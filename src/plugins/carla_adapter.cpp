#include "carla_adapter.h"
#include <CarlaNativePlugin.h>
#include <CarlaHost.h>
#include <vector>
#include <cstring>
#include <string>
#include <cstdlib>
#include <mutex>

#if defined( __x86_64__ ) || defined( __aarch64__ )
#    define BINARY_NATIVE CarlaBackend::BINARY_POSIX64
#else
#    define BINARY_NATIVE CarlaBackend::BINARY_POSIX32
#endif

#include <atomic>

struct JamulusMidiEv
{
    uint8_t  data[4];
    int      length;
    uint32_t offset;
};

struct CarlaAdapterState
{
    int sampleRate;
    int blockSize;
    int numChannels;

    const NativePluginDescriptor* desc;
    NativePluginHandle            pluginHandle;
    CarlaHostHandle               hostHandle;
    NativeHostDescriptor          hostDesc;
    NativeTimeInfo                timeInfo;

    // Deinterleaving and reinterleaving audio buffers
    std::vector<std::vector<float>> inBuffers;
    std::vector<std::vector<float>> outBuffers;

    std::mutex        mutex;
    std::atomic<bool> bProcessingSuspended{ false };
};

// ---------------------------------------------------------------------------
// Carla Native Host Callbacks
// ---------------------------------------------------------------------------
static uint32_t host_get_buffer_size ( NativeHostHandle handle )
{
    auto state = static_cast<CarlaAdapterState*> ( handle );
    return state->blockSize;
}

static double host_get_sample_rate ( NativeHostHandle handle )
{
    auto state = static_cast<CarlaAdapterState*> ( handle );
    return state->sampleRate;
}

static bool host_is_offline ( NativeHostHandle ) { return false; }

static const NativeTimeInfo* host_get_time_info ( NativeHostHandle handle )
{
    auto state = static_cast<CarlaAdapterState*> ( handle );
    return &state->timeInfo;
}

static bool host_write_midi_event ( NativeHostHandle, const NativeMidiEvent* ) { return true; }

static void host_ui_parameter_changed ( NativeHostHandle, uint32_t, float ) {}
static void host_ui_midi_program_changed ( NativeHostHandle, uint8_t, uint32_t, uint32_t ) {}
static void host_ui_custom_data_changed ( NativeHostHandle, const char*, const char* ) {}
static void host_ui_closed ( NativeHostHandle ) {}

static const char* host_ui_open_file ( NativeHostHandle, bool, const char*, const char* ) { return nullptr; }
static const char* host_ui_save_file ( NativeHostHandle, bool, const char*, const char* ) { return nullptr; }

static intptr_t host_dispatcher ( NativeHostHandle, NativeHostDispatcherOpcode, int32_t, intptr_t, void*, float ) { return 0; }

// ---------------------------------------------------------------------------
// C Public API Implementation
// ---------------------------------------------------------------------------
extern "C"
{

    void* carla_adapter_init ( int sampleRate, int blockSize, int numChannels )
    {
        CarlaAdapterState* state = new CarlaAdapterState();
        state->sampleRate        = sampleRate;
        state->blockSize         = blockSize;
        state->numChannels       = numChannels;

        // Initialize time info structure safely to avoid nullptr dereference in Carla process
        std::memset ( &state->timeInfo, 0, sizeof ( NativeTimeInfo ) );
        state->timeInfo.playing   = false;
        state->timeInfo.frame     = 0;
        state->timeInfo.usecs     = 0;
        state->timeInfo.bbt.valid = false;

        // Resolve the native rack plugin descriptor
        state->desc = carla_get_native_rack_plugin();
        if ( !state->desc )
        {
            delete state;
            return nullptr;
        }

        // Allocate audio buffers matching exact Carla-Rack port counts!
        state->inBuffers.resize ( state->desc->audioIns );
        for ( uint32_t i = 0; i < state->desc->audioIns; ++i )
        {
            state->inBuffers[i].resize ( blockSize, 0.0f );
        }

        state->outBuffers.resize ( state->desc->audioOuts );
        for ( uint32_t i = 0; i < state->desc->audioOuts; ++i )
        {
            state->outBuffers[i].resize ( blockSize, 0.0f );
        }

        // Setup Host Descriptor
        state->hostDesc.handle      = state;
        state->hostDesc.resourceDir = "/usr/share/carla";
        state->hostDesc.uiName      = "Jamulus Plugins";
        state->hostDesc.uiParentId  = 0;

        state->hostDesc.get_buffer_size  = host_get_buffer_size;
        state->hostDesc.get_sample_rate  = host_get_sample_rate;
        state->hostDesc.is_offline       = host_is_offline;
        state->hostDesc.get_time_info    = host_get_time_info;
        state->hostDesc.write_midi_event = host_write_midi_event;

        state->hostDesc.ui_parameter_changed    = host_ui_parameter_changed;
        state->hostDesc.ui_midi_program_changed = host_ui_midi_program_changed;
        state->hostDesc.ui_custom_data_changed  = host_ui_custom_data_changed;
        state->hostDesc.ui_closed               = host_ui_closed;

        state->hostDesc.ui_open_file = host_ui_open_file;
        state->hostDesc.ui_save_file = host_ui_save_file;
        state->hostDesc.dispatcher   = host_dispatcher;

        // Instantiate
        state->pluginHandle = state->desc->instantiate ( &state->hostDesc );
        if ( !state->pluginHandle )
        {
            delete state;
            return nullptr;
        }

        // Get CarlaHostHandle (must be done before activation to set engine options)
        state->hostHandle = carla_create_native_plugin_host_handle ( state->desc, state->pluginHandle );
        if ( !state->hostHandle )
        {
            state->desc->cleanup ( state->pluginHandle );
            delete state;
            return nullptr;
        }

        // Prefer plugin bridges (out-of-process loading) for maximum safety and stability.
        // This isolates plugin threads (e.g. Wine/yabridge threads) and prevents crashes in the main host.
        carla_set_engine_option ( state->hostHandle, CarlaBackend::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, 1, nullptr );

        // Activate the plugin descriptor (starts the engine properly)
        state->desc->activate ( state->pluginHandle );

        return state;
    }

    void carla_adapter_shutdown ( void* handle )
    {
        if ( !handle )
            return;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        {
            std::lock_guard<std::mutex> lock ( state->mutex );
            if ( state->desc )
            {
                state->desc->deactivate ( state->pluginHandle );
                if ( state->hostHandle )
                {
                    carla_host_handle_free ( state->hostHandle );
                }
                state->desc->cleanup ( state->pluginHandle );
            }
        }

        delete state;
    }

    void carla_adapter_process ( void* handle, float* interleaved, int numFrames, int numChannels, const void* midiEvents, int numMidiEvents )
    {
        if ( !handle || !interleaved || numFrames <= 0 )
            return;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        // If processing is temporarily suspended, bypass Carla immediately.
        // This prevents race conditions and crashes during synchronous plugin additions/removals.
        if ( state->bProcessingSuspended.load ( std::memory_order_relaxed ) )
        {
            return;
        }

        if ( !state->desc )
            return;

        const int frames = numFrames;

        // Keep host time moving without forcing transport playback state.
        state->timeInfo.playing = false;
        state->timeInfo.frame += frames;
        if ( state->sampleRate > 0 )
        {
            state->timeInfo.usecs +=
                static_cast<uint64_t> ( ( static_cast<double> ( frames ) / static_cast<double> ( state->sampleRate ) ) * 1000000.0 );
        }
        state->timeInfo.bbt.valid = false;

        // Safety check / dynamic resize
        if ( state->inBuffers.empty() || state->inBuffers[0].size() < (size_t) frames )
        {
            state->blockSize = frames;
            for ( uint32_t i = 0; i < state->desc->audioIns; ++i )
            {
                state->inBuffers[i].resize ( frames, 0.0f );
            }
            for ( uint32_t i = 0; i < state->desc->audioOuts; ++i )
            {
                state->outBuffers[i].resize ( frames, 0.0f );
            }
        }

        // Pre-clear inputs
        for ( uint32_t i = 0; i < state->desc->audioIns; ++i )
        {
            std::memset ( state->inBuffers[i].data(), 0, frames * sizeof ( float ) );
        }

        // Deinterleave input
        if ( numChannels == 2 && state->desc->audioIns >= 2 )
        {
            for ( int i = 0; i < frames; ++i )
            {
                state->inBuffers[0][i] = interleaved[2 * i];
                state->inBuffers[1][i] = interleaved[2 * i + 1];
            }
        }
        else if ( state->desc->audioIns >= 1 )
        {
            for ( int i = 0; i < frames; ++i )
            {
                state->inBuffers[0][i] = interleaved[i];
                if ( state->desc->audioIns >= 2 )
                {
                    state->inBuffers[1][i] = interleaved[i];
                }
            }
        }

        // Pre-clear outputs
        for ( uint32_t i = 0; i < state->desc->audioOuts; ++i )
        {
            std::memset ( state->outBuffers[i].data(), 0, frames * sizeof ( float ) );
        }

        // Map MIDI Events
        std::vector<NativeMidiEvent> carlaMidiEvs;
        if ( midiEvents && numMidiEvents > 0 )
        {
            carlaMidiEvs.resize ( numMidiEvents );
            const JamulusMidiEv* jamEvs = static_cast<const JamulusMidiEv*> ( midiEvents );
            for ( int i = 0; i < numMidiEvents; ++i )
            {
                const uint32_t clampedTime =
                    jamEvs[i].offset >= static_cast<uint32_t> ( frames ) ? static_cast<uint32_t> ( frames - 1 ) : jamEvs[i].offset;
                int eventSize = jamEvs[i].length;
                if ( eventSize < 1 )
                    eventSize = 1;
                if ( eventSize > 4 )
                    eventSize = 4;

                carlaMidiEvs[i].time = clampedTime;
                carlaMidiEvs[i].port = 0;
                carlaMidiEvs[i].size = static_cast<uint32_t> ( eventSize );
                std::memcpy ( carlaMidiEvs[i].data, jamEvs[i].data, static_cast<size_t> ( eventSize ) );
            }

            // Preserve arrival order, but avoid dumping many controller messages onto the
            // exact same sample. Carla instruments tend to behave better when bursts are
            // serialized within the current block instead of landing simultaneously.
            std::stable_sort ( carlaMidiEvs.begin(), carlaMidiEvs.end(), [] ( const NativeMidiEvent& a, const NativeMidiEvent& b ) {
                return a.time < b.time;
            } );

            uint32_t previousTime = 0;
            for ( size_t i = 0; i < carlaMidiEvs.size(); ++i )
            {
                if ( i == 0 )
                {
                    previousTime = carlaMidiEvs[i].time;
                    continue;
                }

                if ( carlaMidiEvs[i].time <= previousTime )
                {
                    carlaMidiEvs[i].time =
                        ( previousTime < static_cast<uint32_t> ( frames - 1 ) ) ? ( previousTime + 1 ) : static_cast<uint32_t> ( frames - 1 );
                }

                previousTime = carlaMidiEvs[i].time;
            }
        }

        // Build pointer arrays matching exact Carla-Rack port counts!
        std::vector<float*> inPtrs ( state->desc->audioIns );
        for ( uint32_t i = 0; i < state->desc->audioIns; ++i )
        {
            inPtrs[i] = state->inBuffers[i].data();
        }

        std::vector<float*> outPtrs ( state->desc->audioOuts );
        for ( uint32_t i = 0; i < state->desc->audioOuts; ++i )
        {
            outPtrs[i] = state->outBuffers[i].data();
        }

        // Process
        state->desc->process ( state->pluginHandle, inPtrs.data(), outPtrs.data(), frames, carlaMidiEvs.data(), carlaMidiEvs.size() );

        // Reinterleave output
        if ( numChannels == 2 && state->desc->audioOuts >= 2 )
        {
            for ( int i = 0; i < frames; ++i )
            {
                interleaved[2 * i]     = state->outBuffers[0][i];
                interleaved[2 * i + 1] = state->outBuffers[1][i];
            }
        }
        else if ( state->desc->audioOuts >= 1 )
        {
            for ( int i = 0; i < frames; ++i )
            {
                interleaved[i] = state->outBuffers[0][i];
            }
        }
    }

    int carla_adapter_add_plugin ( void*       handle,
                                   int         binaryType,
                                   int         pluginType,
                                   const char* filename,
                                   const char* name,
                                   const char* label,
                                   int64_t     uniqueId )
    {
        if ( !handle )
            return -1;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        // Suspend processing during plugin load to avoid race conditions
        state->bProcessingSuspended.store ( true, std::memory_order_relaxed );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( !state->hostHandle )
        {
            state->bProcessingSuspended.store ( false, std::memory_order_relaxed );
            return -1;
        }

        // Find next available plugin ID (usually sequentially allocated, but we want to know what it is)
        uint32_t currentCount = carla_get_current_plugin_count ( state->hostHandle );

        uint32_t pluginOptions = CarlaBackend::PLUGIN_OPTION_SEND_CONTROL_CHANGES | CarlaBackend::PLUGIN_OPTION_SEND_CHANNEL_PRESSURE |
                                 CarlaBackend::PLUGIN_OPTION_SEND_NOTE_AFTERTOUCH | CarlaBackend::PLUGIN_OPTION_SEND_PITCHBEND |
                                 CarlaBackend::PLUGIN_OPTION_SEND_ALL_SOUND_OFF;

        bool ok = carla_add_plugin ( state->hostHandle,
                                     static_cast<BinaryType> ( binaryType ),
                                     static_cast<PluginType> ( pluginType ),
                                     filename,
                                     name,
                                     label,
                                     uniqueId,
                                     nullptr,
                                     pluginOptions );

        int result = -1;
        if ( ok )
        {
            // Return the index of the newly added plugin
            uint32_t newCount = carla_get_current_plugin_count ( state->hostHandle );
            if ( newCount > currentCount )
            {
                result = (int) ( newCount - 1 );
            }
            else
            {
                result = (int) currentCount;
            }
        }

        // Resume audio processing
        state->bProcessingSuspended.store ( false, std::memory_order_relaxed );

        return result;
    }

    bool carla_adapter_remove_plugin ( void* handle, int pluginId )
    {
        if ( !handle || pluginId < 0 )
            return false;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        state->bProcessingSuspended.store ( true, std::memory_order_relaxed );
        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( !state->hostHandle )
        {
            state->bProcessingSuspended.store ( false, std::memory_order_relaxed );
            return false;
        }

        bool result = carla_remove_plugin ( state->hostHandle, (uint) pluginId );
        state->bProcessingSuspended.store ( false, std::memory_order_relaxed );
        return result;
    }

    bool carla_adapter_remove_all_plugins ( void* handle )
    {
        if ( !handle )
            return false;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        state->bProcessingSuspended.store ( true, std::memory_order_relaxed );
        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( !state->hostHandle )
        {
            state->bProcessingSuspended.store ( false, std::memory_order_relaxed );
            return false;
        }

        bool result = carla_remove_all_plugins ( state->hostHandle );
        state->bProcessingSuspended.store ( false, std::memory_order_relaxed );
        return result;
    }

    void carla_adapter_show_plugin_ui ( void* handle, int pluginId, bool show )
    {
        if ( !handle || pluginId < 0 )
            return;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( !state->hostHandle )
            return;

        carla_show_custom_ui ( state->hostHandle, (uint) pluginId, show );
    }

    void carla_adapter_idle ( void* handle )
    {
        if ( !handle )
            return;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( state->desc )
        {
            if ( state->desc->ui_idle )
            {
                state->desc->ui_idle ( state->pluginHandle );
            }
            if ( state->desc->dispatcher )
            {
                state->desc->dispatcher ( state->pluginHandle, NATIVE_PLUGIN_OPCODE_IDLE, 0, 0, nullptr, 0.0f );
            }
        }
    }

    void carla_adapter_set_active ( void* handle, int pluginId, bool active )
    {
        if ( !handle || pluginId < 0 )
            return;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( !state->hostHandle )
            return;

        carla_set_active ( state->hostHandle, (uint) pluginId, active );
    }

    bool carla_adapter_get_active ( void* handle, int pluginId )
    {
        if ( !handle || pluginId < 0 )
            return false;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( !state->hostHandle )
            return false;

        const float activeValue =
            carla_get_internal_parameter_value ( state->hostHandle, (uint) pluginId, CarlaBackend::PARAMETER_ACTIVE );
        return activeValue >= 0.5f;
    }

    void carla_adapter_set_drywet ( void* handle, int pluginId, float value )
    {
        if ( !handle || pluginId < 0 )
            return;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( !state->hostHandle )
            return;

        carla_set_drywet ( state->hostHandle, (uint) pluginId, value );
    }

    void carla_adapter_set_volume ( void* handle, int pluginId, float value )
    {
        if ( !handle || pluginId < 0 )
            return;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( !state->hostHandle )
            return;

        carla_set_volume ( state->hostHandle, (uint) pluginId, value );
    }

    char* carla_adapter_save_state ( void* handle )
    {
        if ( !handle )
            return nullptr;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( state->desc && state->desc->get_state )
        {
            char* rawState = state->desc->get_state ( state->pluginHandle );
            if ( rawState )
            {
                char* copied = strdup ( rawState );
                return copied;
            }
        }
        return nullptr;
    }

    bool carla_adapter_restore_state ( void* handle, const char* stateData )
    {
        if ( !handle || !stateData )
            return false;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( state->desc && state->desc->set_state )
        {
            state->desc->set_state ( state->pluginHandle, stateData );
            return true;
        }
        return false;
    }

    int carla_adapter_get_plugin_count ( void* handle )
    {
        if ( !handle )
            return 0;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( !state->hostHandle )
            return 0;

        return (int) carla_get_current_plugin_count ( state->hostHandle );
    }

    const char* carla_adapter_get_plugin_name ( void* handle, int pluginId )
    {
        if ( !handle || pluginId < 0 )
            return nullptr;
        auto state = static_cast<CarlaAdapterState*> ( handle );

        std::lock_guard<std::mutex> lock ( state->mutex );
        if ( !state->hostHandle )
            return nullptr;

        const CarlaPluginInfo* info = carla_get_plugin_info ( state->hostHandle, (uint) pluginId );
        return info ? info->name : nullptr;
    }

} // extern "C"
