/*
 * LV2 adapter for Jamulus plugin hosting.
 *
 * This implementation uses lilv to discover and instantiate LV2 plugins,
 * and manages the external UI via LV2UI_Show_Interface / LV2UI_Idle_Interface.
 * Designed primarily for loading Carla-Rack or similar host-within-host plugins.
 */

#include "lv2_adapter.h"

#if defined( HAVE_LV2 )

#include <QDebug>
#include <QFile>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <atomic>

#include <lilv/lilv.h>
#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/midi/midi.h>
#include <lv2/options/options.h>
#include <lv2/ui/ui.h>
#include <lv2/urid/urid.h>
#include <lv2/worker/worker.h>
#include <lv2/state/state.h>

// Cross-platform dynamic library loading
#ifdef _WIN32
#include <windows.h>
static void*  lv2_dlopen  ( const char* path, int )     { return (void*)LoadLibraryA ( path ); }
static void*  lv2_dlsym   ( void* h, const char* sym )  { return (void*)GetProcAddress ( (HMODULE)h, sym ); }
static int    lv2_dlclose ( void* h )                    { return FreeLibrary ( (HMODULE)h ) ? 0 : -1; }
static const char* lv2_dlerror()
{
    static char buf[256];
    FormatMessageA ( FORMAT_MESSAGE_FROM_SYSTEM, nullptr, GetLastError(), 0, buf, sizeof(buf), nullptr );
    return buf;
}
#define RTLD_NOW 0 // unused on Windows, just keep the call sites compiling
#else
#include <dlfcn.h>
static void*  lv2_dlopen  ( const char* path, int flags ) { return dlopen ( path, flags ); }
static void*  lv2_dlsym   ( void* h, const char* sym )    { return dlsym ( h, sym ); }
static int    lv2_dlclose ( void* h )                      { return dlclose ( h ); }
static const char* lv2_dlerror()                           { return dlerror(); }
#endif

namespace
{
// ---------------------------------------------------------------------------
// URID map – trivial implementation suitable for a single-plugin host
// ---------------------------------------------------------------------------
static std::unordered_map<std::string, LV2_URID>& UriToId()
{
    static std::unordered_map<std::string, LV2_URID> map;
    return map;
}

static std::unordered_map<LV2_URID, std::string>& IdToUri()
{
    static std::unordered_map<LV2_URID, std::string> map;
    return map;
}

static LV2_URID MapUri ( LV2_URID_Map_Handle, const char* uri )
{
    auto& fwd = UriToId();
    auto it = fwd.find ( uri );
    if ( it != fwd.end() )
        return it->second;

    LV2_URID id = static_cast<LV2_URID> ( fwd.size() + 1 );
    fwd[uri] = id;
    IdToUri()[id] = uri;
    return id;
}

static const char* UnmapUri ( LV2_URID_Unmap_Handle, LV2_URID urid )
{
    auto& rev = IdToUri();
    auto it = rev.find ( urid );
    if ( it != rev.end() )
        return it->second.c_str();

    return nullptr;
}

static LV2_URID_Map  g_uridMap  = { nullptr, MapUri };
static LV2_URID_Unmap g_uridUnmap = { nullptr, UnmapUri };

// Pre-map commonly used URIs
static LV2_URID UridAtomSequence()    { static LV2_URID id = MapUri ( nullptr, LV2_ATOM__Sequence ); return id; }
static LV2_URID UridAtomFloat()       { static LV2_URID id = MapUri ( nullptr, LV2_ATOM__Float ); return id; }
static LV2_URID UridBufSizeMaxLen()   { static LV2_URID id = MapUri ( nullptr, LV2_BUF_SIZE__maxBlockLength ); return id; }
static LV2_URID UridBufSizeNomLen()   { static LV2_URID id = MapUri ( nullptr, LV2_BUF_SIZE__nominalBlockLength ); return id; }
static LV2_URID UridParamSampleRate() { static LV2_URID id = MapUri ( nullptr, "http://lv2plug.in/ns/ext/parameters#sampleRate" ); return id; }
static LV2_URID UridAtomEventTransfer(){ static LV2_URID id = MapUri ( nullptr, "http://lv2plug.in/ns/ext/atom#eventTransfer" ); return id; }
static LV2_URID UridTimeFrame()       { static LV2_URID id = MapUri ( nullptr, "http://lv2plug.in/ns/ext/time#frame" ); return id; }

// ---------------------------------------------------------------------------
// LV2 Runtime – manages a single LV2 plugin instance
// ---------------------------------------------------------------------------
class Lv2Runtime
{
public:
    ~Lv2Runtime() { shutdown(); }

    bool load ( const char* pluginURI, int sampleRateHz, int blockSizeFrames, int hostChannels,
                std::string& errorDescription )
    {
        shutdown();

        sampleRate = sampleRateHz;
        blockSize  = blockSizeFrames;
        channels   = hostChannels;

        world = lilv_world_new();
        if ( !world )
        {
            errorDescription = "Failed to create LilvWorld.";
            return false;
        }

        lilv_world_load_all ( world );

        const LilvPlugins* plugins = lilv_world_get_all_plugins ( world );
        LilvNode* uriNode = lilv_new_uri ( world, pluginURI );
        plugin = lilv_plugins_get_by_uri ( plugins, uriNode );
        lilv_node_free ( uriNode );

        if ( !plugin )
        {
            errorDescription = std::string ( "LV2 plugin not found: " ) + pluginURI;
            return false;
        }

        // Prepare LV2 options for the plugin
        float fSampleRate = static_cast<float> ( sampleRate );
        int32_t iBlockSize = blockSize;

        optionSampleRate = { LV2_OPTIONS_INSTANCE, 0,
                             UridParamSampleRate(), sizeof ( float ),
                             UridAtomFloat(), &fSampleRate };
        optionMaxBlockLength = { LV2_OPTIONS_INSTANCE, 0,
                                 UridBufSizeMaxLen(), sizeof ( int32_t ),
                                 MapUri ( nullptr, LV2_ATOM__Int ), &iBlockSize };
        optionNomBlockLength = { LV2_OPTIONS_INSTANCE, 0,
                                 UridBufSizeNomLen(), sizeof ( int32_t ),
                                 MapUri ( nullptr, LV2_ATOM__Int ), &iBlockSize };
        optionTerminator = { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr };

        options[0] = optionSampleRate;
        options[1] = optionMaxBlockLength;
        options[2] = optionNomBlockLength;
        options[3] = optionTerminator;

        // Worker Feature
        featureWorkerScheduleData = { this, ScheduleWorkTrampoline };

        // Build features array
        featureMap     = { LV2_URID__map, &g_uridMap };
        featureUnmap   = { LV2_URID__unmap, &g_uridUnmap };
        featureOptions = { LV2_OPTIONS__options, options };
        featureBufSizeBounded = { LV2_BUF_SIZE__boundedBlockLength, nullptr };
        featureWorkerSchedule = { LV2_WORKER__schedule, &featureWorkerScheduleData };

        features[0] = &featureMap;
        features[1] = &featureUnmap;
        features[2] = &featureOptions;
        features[3] = &featureBufSizeBounded;
        features[4] = &featureWorkerSchedule;
        features[5] = nullptr;

        // Start worker thread
        workerQuit = false;
        workerThread = std::thread(&Lv2Runtime::workerLoop, this);

        // Instantiate the plugin
        instance = lilv_plugin_instantiate ( plugin, sampleRate, features );
        if ( !instance )
        {
            errorDescription = std::string ( "Failed to instantiate LV2 plugin: " ) + pluginURI;
            return false;
        }

        workerInterface = static_cast<const LV2_Worker_Interface*> (
            lilv_instance_get_extension_data ( instance, LV2_WORKER__interface ) );

        // Discover and connect ports
        if ( !setupPorts ( errorDescription ) )
        {
            shutdown();
            return false;
        }

        // Activate
        lilv_instance_activate ( instance );
        bActive = true;

        savedPluginURI = pluginURI;
        return true;
    }

    void shutdown()
    {
        closeEditor();

        if ( instance )
        {
            if ( bActive )
                lilv_instance_deactivate ( instance );
        }

        // Stop worker thread safely before freeing instance
        if ( workerThread.joinable() )
        {
            {
                std::lock_guard<std::mutex> lg(workerMutex);
                workerQuit = true;
            }
            workerCv.notify_one();
            workerThread.join();
        }

        if ( instance )
        {
            lilv_instance_free ( instance );
        }

        if ( world )
            lilv_world_free ( world );

        instance = nullptr;
        world    = nullptr;
        plugin   = nullptr;
        workerInterface = nullptr;
        bActive  = false;

        audioInLeft.clear();
        audioInRight.clear();
        audioOutLeft.clear();
        audioOutRight.clear();
        atomInBuffer.clear();
        atomOutBuffer.clear();

        audioInLeftIdx  = -1;
        audioInRightIdx = -1;
        audioOutLeftIdx = -1;
        audioOutRightIdx = -1;
        atomInIdx       = -1;
        atomOutIdx      = -1;
        freewheelIdx    = -1;

        std::queue<WorkerJob> emptyWorker;
        std::swap(workerJobs, emptyWorker);

        std::queue<WorkerJob> emptyResponse;
        std::swap(responseJobs, emptyResponse);

        uiEvents.clear();
    }

    void process ( float* interleaved, int numFrames, int numChannels, const void* midiEvents = nullptr, int numMidiEvents = 0 )
    {
        if ( !instance || !bActive || !interleaved || numFrames <= 0 || numChannels <= 0 )
            return;

        const int iFrames = std::min ( numFrames, blockSize );

        // Process worker responses
        if ( workerInterface && workerInterface->work_response )
        {
            std::lock_guard<std::mutex> lg(responseMutex);
            while ( !responseJobs.empty() )
            {
                const WorkerJob& job = responseJobs.front();
                workerInterface->work_response(instance, job.data.size(), job.data.data());
                responseJobs.pop();
            }
        }

        // Ensure buffers are large enough
        if ( static_cast<int> ( audioInLeft.size() ) < iFrames )
        {
            audioInLeft.resize ( iFrames, 0.0f );
            audioInRight.resize ( iFrames, 0.0f );
            audioOutLeft.resize ( iFrames, 0.0f );
            audioOutRight.resize ( iFrames, 0.0f );
        }

        // Deinterleave input
        for ( int i = 0; i < iFrames; ++i )
        {
            audioInLeft[i]  = interleaved[i * numChannels];
            audioInRight[i] = ( numChannels > 1 ) ? interleaved[i * numChannels + 1] : interleaved[i * numChannels];
        }

        // Clear output buffers
        std::fill ( audioOutLeft.begin(), audioOutLeft.begin() + iFrames, 0.0f );
        std::fill ( audioOutRight.begin(), audioOutRight.begin() + iFrames, 0.0f );

        // Connect audio ports
        if ( audioInLeftIdx >= 0 )
            lilv_instance_connect_port ( instance, audioInLeftIdx, audioInLeft.data() );
        if ( audioInRightIdx >= 0 )
            lilv_instance_connect_port ( instance, audioInRightIdx, audioInRight.data() );
        if ( audioOutLeftIdx >= 0 )
            lilv_instance_connect_port ( instance, audioOutLeftIdx, audioOutLeft.data() );
        if ( audioOutRightIdx >= 0 )
            lilv_instance_connect_port ( instance, audioOutRightIdx, audioOutRight.data() );

        // Handle atom inputs and UI events
        if ( atomInIdx >= 0 )
        {
            LV2_Atom_Sequence* seq = reinterpret_cast<LV2_Atom_Sequence*> ( atomInBuffer.data() );
            seq->atom.type = UridAtomSequence();
            seq->atom.size = sizeof ( LV2_Atom_Sequence_Body );
            seq->body.unit = UridTimeFrame();
            seq->body.pad  = 0;
            
            // Append UI events to the atom sequence
            uint32_t capacity = atomInBuffer.size() - sizeof(LV2_Atom);
            
            // Append MIDI events
            if ( midiEvents && numMidiEvents > 0 )
            {
                struct MidiEv { uint8_t data[4]; int length; uint32_t offset; };
                const MidiEv* evs = static_cast<const MidiEv*>(midiEvents);
                for ( int i = 0; i < numMidiEvents; ++i )
                {
                    const MidiEv& ev = evs[i];
                    uint32_t ev_size = sizeof(LV2_Atom_Event) + ev.length;
                    if ( seq->atom.size + ev_size <= capacity )
                    {
                        LV2_Atom_Event* aev = reinterpret_cast<LV2_Atom_Event*>(
                            reinterpret_cast<uint8_t*>(&seq->body) + seq->atom.size
                        );
                        aev->time.frames = ev.offset;
                        aev->body.type = MapUri(nullptr, LV2_MIDI__MidiEvent);
                        aev->body.size = ev.length;
                        std::memcpy(LV2_ATOM_BODY(&aev->body), ev.data, ev.length);
                        
                        uint32_t padded_size = lv2_atom_pad_size(ev.length);
                        seq->atom.size += sizeof(LV2_Atom_Event) + padded_size;
                    }
                }
            }
            
            std::lock_guard<std::mutex> lg(uiEventMutex);
            for ( const auto& ev : uiEvents )
            {
                if ( ev.port_protocol == UridAtomEventTransfer() && ev.port_index == static_cast<uint32_t>(atomInIdx) )
                {
                    // For Atom events, the UI passes the atom itself
                    const uint32_t ev_size = sizeof(LV2_Atom_Event) + ev.data.size();
                    if ( seq->atom.size + ev_size <= capacity )
                    {
                        LV2_Atom_Event* aev = reinterpret_cast<LV2_Atom_Event*>(
                            reinterpret_cast<uint8_t*>(&seq->body) + seq->atom.size
                        );
                        aev->time.frames = 0;
                        std::memcpy(&aev->body, ev.data.data(), ev.data.size());
                        // Pad to 64-bit alignment
                        uint32_t padded_size = lv2_atom_pad_size(ev.data.size());
                        seq->atom.size += sizeof(LV2_Atom_Event) + padded_size;
                    }
                }
                else if ( ev.port_protocol == 0 )
                {
                    // Float control updates are handled by directly pushing to the control port,
                    // or in the case of some plugins, wrapped in atom sequences. We aren't fully 
                    // supporting non-atom UI writes yet.
                }
            }
            uiEvents.clear();

            lilv_instance_connect_port ( instance, atomInIdx, atomInBuffer.data() );
        }

        if ( atomOutIdx >= 0 )
        {
            LV2_Atom_Sequence* seq = reinterpret_cast<LV2_Atom_Sequence*> ( atomOutBuffer.data() );
            seq->atom.type = UridAtomSequence();
            seq->atom.size = atomOutBuffer.size() - sizeof ( LV2_Atom );
            seq->body.unit = 0;
            seq->body.pad  = 0;
            lilv_instance_connect_port ( instance, atomOutIdx, atomOutBuffer.data() );
        }

        // Connect freewheel port (always 0 = realtime)
        if ( freewheelIdx >= 0 )
            lilv_instance_connect_port ( instance, freewheelIdx, &freewheelValue );

        // Run the plugin
        lilv_instance_run ( instance, iFrames );

        // Post-run: copy any outgoing events to the DSP->UI queue
        if ( atomOutIdx >= 0 )
        {
            LV2_Atom_Sequence* seq = reinterpret_cast<LV2_Atom_Sequence*> ( atomOutBuffer.data() );
            if ( seq->atom.size > sizeof(LV2_Atom_Sequence_Body) )
            {
                size_t eventsLen = seq->atom.size - sizeof(LV2_Atom_Sequence_Body);
                uint8_t* eventsStart = reinterpret_cast<uint8_t*>(&seq->body);
                
                std::lock_guard<std::mutex> lg(dspToUiMutex);
                size_t oldSize = dspToUiEvents.size();
                dspToUiEvents.resize(oldSize + eventsLen);
                std::memcpy(dspToUiEvents.data() + oldSize, eventsStart, eventsLen);
            }
        }
        
        if ( workerInterface && workerInterface->end_run )
            workerInterface->end_run(instance);

        // Reinterleave output
        for ( int i = 0; i < iFrames; ++i )
        {
            interleaved[i * numChannels] = audioOutLeft[i];
            if ( numChannels > 1 )
                interleaved[i * numChannels + 1] = audioOutRight[i];
        }
    }

    bool showEditor ( std::string& errorDescription )
    {
        if ( !instance || !plugin )
        {
            errorDescription = "Plugin not loaded.";
            return false;
        }

        // If UI is already open, just call show again
        if ( uiInstance && showInterface )
        {
            showInterface->show ( uiInstance );
            bEditorVisible = true;
            return true;
        }

        // Find the UI descriptor from the plugin's library
        LilvUIs* uis = lilv_plugin_get_uis ( plugin );
        if ( !uis )
        {
            errorDescription = "Plugin does not provide any UI.";
            return false;
        }

        const LilvUI* selectedUI = nullptr;
        LilvNode* externalUIType = lilv_new_uri ( world, "http://kxstudio.sf.net/ns/lv2ext/external-ui#Widget" );

        LILV_FOREACH ( uis, iter, uis )
        {
            const LilvUI* ui = lilv_uis_get ( uis, iter );
            if ( lilv_ui_is_a ( ui, externalUIType ) )
            {
                selectedUI = ui;
                break;
            }
        }

        lilv_node_free ( externalUIType );

        if ( !selectedUI )
        {
            lilv_uis_free ( uis );
            errorDescription = "No external UI found for this plugin.";
            return false;
        }

        // Get the UI library path
        const LilvNode* uiBinary = lilv_ui_get_binary_uri ( selectedUI );
        char* uiBinaryPath = lilv_file_uri_parse ( lilv_node_as_uri ( uiBinary ), nullptr );
        if ( !uiBinaryPath )
        {
            lilv_uis_free ( uis );
            errorDescription = "Failed to resolve UI binary path.";
            return false;
        }

        // Load the UI library
        void* uiLib = lv2_dlopen ( uiBinaryPath, RTLD_NOW );
        lilv_free ( uiBinaryPath );

        if ( !uiLib )
        {
            lilv_uis_free ( uis );
            errorDescription = std::string ( "Failed to load UI library: " ) + lv2_dlerror();
            return false;
        }

        // Get the UI descriptor function
        auto uiDescFunc = reinterpret_cast<LV2UI_DescriptorFunction> ( lv2_dlsym ( uiLib, "lv2ui_descriptor" ) );
        if ( !uiDescFunc )
        {
            lv2_dlclose ( uiLib );
            lilv_uis_free ( uis );
            errorDescription = "UI library does not export lv2ui_descriptor.";
            return false;
        }

        // Find the matching UI descriptor by URI
        const char* selectedUIUri = lilv_node_as_uri ( lilv_ui_get_uri ( selectedUI ) );
        const LV2UI_Descriptor* uiDesc = nullptr;
        for ( uint32_t i = 0; ; ++i )
        {
            const LV2UI_Descriptor* d = uiDescFunc ( i );
            if ( !d )
                break;
            if ( std::strcmp ( d->URI, selectedUIUri ) == 0 )
            {
                uiDesc = d;
                break;
            }
        }

        if ( !uiDesc )
        {
            lv2_dlclose ( uiLib );
            lilv_uis_free ( uis );
            errorDescription = "UI descriptor not found for URI.";
            return false;
        }

        uiLibHandle = uiLib;

        // Prepare features for the UI, including instance-access
        LV2_Feature featureInstanceAccess = { "http://lv2plug.in/ns/ext/instance-access",
                                              const_cast<void*> ( static_cast<const void*> (
                                                  lilv_instance_get_handle ( instance ) ) ) };

        const LV2_Feature* uiFeatures[] = {
            &featureMap,
            &featureUnmap,
            &featureOptions,
            &featureInstanceAccess,
            nullptr
        };

        // Get the bundle path for the UI
        const LilvNode* uiBundleUri = lilv_ui_get_bundle_uri ( selectedUI );
        char* uiBundlePath = lilv_file_uri_parse ( lilv_node_as_uri ( uiBundleUri ), nullptr );

        LV2UI_Widget widget = nullptr;
        qDebug() << "lv2_adapter: instantiating UI via uiDesc->instantiate for URI:" << savedPluginURI.c_str();
        uiInstance = uiDesc->instantiate ( uiDesc,
                                           savedPluginURI.c_str(),
                                           uiBundlePath ? uiBundlePath : "",
                                           UiWriteTrampoline,
                                           this,
                                           &widget,
                                           uiFeatures );
        qDebug() << "lv2_adapter: UI instantiate finished, pointer:" << uiInstance;
        uiDescriptor = uiDesc;

        if ( uiBundlePath )
            lilv_free ( uiBundlePath );

        lilv_uis_free ( uis );

        if ( !uiInstance )
        {
            errorDescription = "Failed to instantiate UI.";
            return false;
        }

        // Get the show/hide/idle interfaces
        if ( uiDesc->extension_data )
        {
            showInterface = static_cast<const LV2UI_Show_Interface*> (
                uiDesc->extension_data ( LV2_UI__showInterface ) );
            idleInterface = static_cast<const LV2UI_Idle_Interface*> (
                uiDesc->extension_data ( LV2_UI__idleInterface ) );
        }

        // Call show
        if ( showInterface )
        {
            bShowCompleted = false;
#ifdef _WIN32
            std::thread([this]() {
                qDebug() << "lv2_adapter: showing UI via showInterface->show in worker thread...";
                showInterface->show ( uiInstance );
                qDebug() << "lv2_adapter: UI show completed in worker thread";
                bShowCompleted = true;
            }).detach();
#else
            qDebug() << "lv2_adapter: showing UI via showInterface->show...";
            showInterface->show ( uiInstance );
            qDebug() << "lv2_adapter: UI show completed";
            bShowCompleted = true;
#endif
            bEditorVisible = true;
        }
        else
        {
            errorDescription = "UI does not provide show interface.";
            closeEditor();
            return false;
        }

        return true;
    }

    bool closeEditor()
    {
        // Safely wait for show thread if it is still running to prevent concurrent cleanup crashes
        int waitCount = 0;
        while ( !bShowCompleted && waitCount < 50 )
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            waitCount++;
        }

        if ( uiInstance )
        {
            if ( showInterface && bEditorVisible && bShowCompleted )
                showInterface->hide ( uiInstance );

            if ( uiDescriptor && uiDescriptor->cleanup && bShowCompleted )
                uiDescriptor->cleanup ( uiInstance );

            uiInstance = nullptr;
        }

        if ( uiLibHandle )
        {
            lv2_dlclose ( uiLibHandle );
            uiLibHandle = nullptr;
        }

        uiDescriptor  = nullptr;
        showInterface  = nullptr;
        idleInterface  = nullptr;
        bEditorVisible = false;
        return true;
    }

    bool idleEditor()
    {
        if ( !bShowCompleted )
            return false;

        // Copy queued DSP -> UI events safely
        std::vector<uint8_t> localDspToUiEvents;
        {
            std::lock_guard<std::mutex> lg(dspToUiMutex);
            localDspToUiEvents = std::move(dspToUiEvents);
            dspToUiEvents.clear();
        }

        // Forward atom events from DSP -> UI 
        if ( uiInstance && uiDescriptor && uiDescriptor->port_event && !localDspToUiEvents.empty() )
        {
            size_t offset = 0;
            while ( offset < localDspToUiEvents.size() )
            {
                LV2_Atom_Event* ev = reinterpret_cast<LV2_Atom_Event*>(localDspToUiEvents.data() + offset);
                uiDescriptor->port_event(uiInstance, atomOutIdx, ev->body.size,
                    UridAtomEventTransfer(), &ev->body);
                offset += sizeof(LV2_Atom_Event) + lv2_atom_pad_size(ev->body.size);
            }
        }
        
        if ( !uiInstance || !idleInterface || !bEditorVisible )
            return false;

        static bool firstIdle = true;
        if ( firstIdle )
        {
            qDebug() << "lv2_adapter: first call to idleInterface->idle...";
            firstIdle = false;
        }
        int result = idleInterface->idle ( uiInstance );
        static bool firstIdleDone = true;
        if ( firstIdleDone )
        {
            qDebug() << "lv2_adapter: first call to idle completed successfully, result:" << result;
            firstIdleDone = false;
        }
        if ( result != 0 )
        {
            // UI requested close
            bEditorVisible = false;
            return false;
        }

        return true;
    }

    bool isEditorVisible() const { return bEditorVisible; }

    // --- Direct LV2 State restore for Carla preset loading ---
    // Instead of serializing XML into Turtle (fragile with large files),
    // we call the plugin's LV2 State restore interface directly with a
    // custom retrieve callback.

    struct PresetRestoreData {
        const void* chunkData;
        size_t      chunkSize;
        LV2_URID    chunkKeyUrid;
        LV2_URID    atomStringUrid;
    };

    static const void* presetRetrieveCallback(
        LV2_State_Handle handle,
        uint32_t         key,
        size_t*          size,
        uint32_t*        type,
        uint32_t*        flags)
    {
        auto* data = static_cast<PresetRestoreData*>(handle);
        if (key == data->chunkKeyUrid)
        {
            *size  = data->chunkSize;
            *type  = data->atomStringUrid;
            *flags = LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE;
            return data->chunkData;
        }
        *size = 0;
        *type = 0;
        *flags = 0;
        return nullptr;
    }

    bool loadPreset(const char* presetPath)
    {
        if (!instance) return false;

        // Read the .carxp file
        QFile file(QString::fromUtf8(presetPath));
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "loadPreset: cannot open file:" << presetPath;
            return false;
        }
        QByteArray xmlData = file.readAll();
        file.close();

        if (xmlData.isEmpty())
        {
            qWarning() << "loadPreset: file is empty:" << presetPath;
            return false;
        }



        // Get the LV2 State interface from the plugin
        const LV2_State_Interface* stateIface =
            static_cast<const LV2_State_Interface*>(
                lilv_instance_get_extension_data(instance, LV2_STATE__interface));

        if (!stateIface || !stateIface->restore)
        {
            qWarning() << "loadPreset: plugin does not support LV2 State interface";
            return false;
        }

        // Map the URIs that Carla expects
        LV2_URID chunkKeyUrid    = MapUri(nullptr, "http://kxstudio.sf.net/ns/carla/chunk");
        LV2_URID atomStringUrid  = MapUri(nullptr, LV2_ATOM__String);

        // Prepare restore data - Carla expects the raw XML as a null-terminated string
        PresetRestoreData restoreData;
        restoreData.chunkData      = xmlData.constData();
        restoreData.chunkSize      = xmlData.size() + 1; // include null terminator
        restoreData.chunkKeyUrid   = chunkKeyUrid;
        restoreData.atomStringUrid = atomStringUrid;

        // Call the plugin's restore function directly
        LV2_Handle lv2Handle = lilv_instance_get_handle(instance);
        LV2_State_Status status = stateIface->restore(
            lv2Handle,
            presetRetrieveCallback,
            &restoreData,
            0,       // flags
            features // pass our features (includes URID map etc.)
        );


        return (status == LV2_STATE_SUCCESS);
    }

    struct PresetStoreData {
        QByteArray* outData;
        LV2_URID chunkKeyUrid;
        LV2_URID atomStringUrid;
    };

    static LV2_State_Status presetStoreCallback(
        LV2_State_Handle handle,
        uint32_t         key,
        const void*      value,
        size_t           size,
        uint32_t         type,
        uint32_t         flags)
    {
        auto* data = static_cast<PresetStoreData*>(handle);
        if (key == data->chunkKeyUrid && type == data->atomStringUrid)
        {
            if (value && size > 0)
            {
                // size might include null terminator
                data->outData->append(static_cast<const char*>(value), size);
            }
            return LV2_STATE_SUCCESS;
        }
        return LV2_STATE_SUCCESS; // Ignore other properties
    }

    QByteArray saveState()
    {
        QByteArray result;
        if (!instance) return result;

        const LV2_State_Interface* stateIface =
            static_cast<const LV2_State_Interface*>(
                lilv_instance_get_extension_data(instance, LV2_STATE__interface));

        if (!stateIface || !stateIface->save)
        {
            qWarning() << "saveState: plugin does not support LV2 State interface";
            return result;
        }

        PresetStoreData storeData;
        storeData.outData = &result;
        storeData.chunkKeyUrid = MapUri(nullptr, "http://kxstudio.sf.net/ns/carla/chunk");
        storeData.atomStringUrid = MapUri(nullptr, LV2_ATOM__String);

        LV2_Handle lv2Handle = lilv_instance_get_handle(instance);
        LV2_State_Status status = stateIface->save(
            lv2Handle,
            presetStoreCallback,
            &storeData,
            LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE,
            features
        );

        if (status != LV2_STATE_SUCCESS)
        {
            qWarning() << "saveState: save returned status" << status;
        }
        
        return result;
    }

    bool restoreState(const char* data, int size)
    {
        if (!instance || !data || size <= 0) return false;

        const LV2_State_Interface* stateIface =
            static_cast<const LV2_State_Interface*>(
                lilv_instance_get_extension_data(instance, LV2_STATE__interface));

        if (!stateIface || !stateIface->restore)
        {
            qWarning() << "restoreState: plugin does not support LV2 State interface";
            return false;
        }

        LV2_URID chunkKeyUrid    = MapUri(nullptr, "http://kxstudio.sf.net/ns/carla/chunk");
        LV2_URID atomStringUrid  = MapUri(nullptr, LV2_ATOM__String);

        PresetRestoreData restoreData;
        restoreData.chunkData      = data;
        restoreData.chunkSize      = size;
        restoreData.chunkKeyUrid   = chunkKeyUrid;
        restoreData.atomStringUrid = atomStringUrid;

        LV2_Handle lv2Handle = lilv_instance_get_handle(instance);
        LV2_State_Status status = stateIface->restore(
            lv2Handle,
            presetRetrieveCallback,
            &restoreData,
            0,
            features
        );

        return (status == LV2_STATE_SUCCESS);
    }

private:
    struct WorkerJob {
        std::vector<uint8_t> data;
    };
    
    struct UiEvent {
        uint32_t port_index;
        uint32_t port_protocol;
        std::vector<uint8_t> data;
    };

    static void UiWriteTrampoline ( LV2UI_Controller controller,
                                  uint32_t         port_index,
                                  uint32_t         buffer_size,
                                  uint32_t         port_protocol,
                                  const void*      buffer )
    {
        auto* self = static_cast<Lv2Runtime*>(controller);
        UiEvent ev;
        ev.port_index = port_index;
        ev.port_protocol = port_protocol;
        if ( buffer_size > 0 && buffer )
        {
            const uint8_t* b = static_cast<const uint8_t*>(buffer);
            ev.data.assign(b, b + buffer_size);
        }
        
        std::lock_guard<std::mutex> lg(self->uiEventMutex);
        self->uiEvents.push_back(std::move(ev));
    }
    
    static LV2_Worker_Status ScheduleWorkTrampoline(LV2_Worker_Schedule_Handle handle,
                                                    uint32_t                   size,
                                                    const void*                data)
    {
        auto* self = static_cast<Lv2Runtime*>(handle);
        WorkerJob job;
        if ( size > 0 && data )
        {
            const uint8_t* b = static_cast<const uint8_t*>(data);
            job.data.assign(b, b + size);
        }
        
        {
            std::lock_guard<std::mutex> lg(self->workerMutex);
            self->workerJobs.push(std::move(job));
        }
        self->workerCv.notify_one();
        return LV2_WORKER_SUCCESS;
    }
    
    static LV2_Worker_Status RespondWorkTrampoline(LV2_Worker_Respond_Handle handle,
                                                   uint32_t                  size,
                                                   const void*               data)
    {
        auto* self = static_cast<Lv2Runtime*>(handle);
        WorkerJob job;
        if ( size > 0 && data )
        {
            const uint8_t* b = static_cast<const uint8_t*>(data);
            job.data.assign(b, b + size);
        }
        
        std::lock_guard<std::mutex> lg(self->responseMutex);
        self->responseJobs.push(std::move(job));
        return LV2_WORKER_SUCCESS;
    }

    void workerLoop()
    {
        while (true)
        {
            WorkerJob job;
            {
                std::unique_lock<std::mutex> lock(workerMutex);
                workerCv.wait(lock, [this]{ return workerQuit || !workerJobs.empty(); });
                
                if ( workerQuit && workerJobs.empty() )
                    break;
                    
                job = std::move(workerJobs.front());
                workerJobs.pop();
            }
            
            if ( workerInterface && workerInterface->work )
            {
                workerInterface->work(instance, RespondWorkTrampoline, this,
                                      job.data.size(), job.data.data());
            }
        }
    }

    bool setupPorts ( std::string& errorDescription )
    {
        const uint32_t numPorts = lilv_plugin_get_num_ports ( plugin );
        if ( numPorts == 0 )
        {
            errorDescription = "Plugin has no ports.";
            return false;
        }

        LilvNode* audioClass   = lilv_new_uri ( world, LILV_URI_AUDIO_PORT );
        LilvNode* controlClass = lilv_new_uri ( world, LILV_URI_CONTROL_PORT );
        LilvNode* atomClass    = lilv_new_uri ( world, LV2_ATOM__AtomPort );
        LilvNode* inputClass   = lilv_new_uri ( world, LILV_URI_INPUT_PORT );
        LilvNode* outputClass  = lilv_new_uri ( world, LILV_URI_OUTPUT_PORT );

        int audioInCount  = 0;
        int audioOutCount = 0;

        for ( uint32_t i = 0; i < numPorts; ++i )
        {
            const LilvPort* port = lilv_plugin_get_port_by_index ( plugin, i );

            if ( lilv_port_is_a ( plugin, port, audioClass ) )
            {
                if ( lilv_port_is_a ( plugin, port, inputClass ) )
                {
                    if ( audioInCount == 0 )
                        audioInLeftIdx = static_cast<int> ( i );
                    else if ( audioInCount == 1 )
                        audioInRightIdx = static_cast<int> ( i );
                    ++audioInCount;
                }
                else if ( lilv_port_is_a ( plugin, port, outputClass ) )
                {
                    if ( audioOutCount == 0 )
                        audioOutLeftIdx = static_cast<int> ( i );
                    else if ( audioOutCount == 1 )
                        audioOutRightIdx = static_cast<int> ( i );
                    ++audioOutCount;
                }
            }
            else if ( lilv_port_is_a ( plugin, port, atomClass ) )
            {
                if ( lilv_port_is_a ( plugin, port, inputClass ) && atomInIdx < 0 )
                    atomInIdx = static_cast<int> ( i );
                else if ( lilv_port_is_a ( plugin, port, outputClass ) && atomOutIdx < 0 )
                    atomOutIdx = static_cast<int> ( i );
            }
            else if ( lilv_port_is_a ( plugin, port, controlClass ) )
            {
                // Check if this is the freewheel port
                LilvNode* designation = lilv_port_get ( plugin, port, lilv_new_uri ( world, LV2_CORE__designation ) );
                if ( designation )
                {
                    if ( std::strcmp ( lilv_node_as_uri ( designation ), LV2_CORE__freeWheeling ) == 0 )
                        freewheelIdx = static_cast<int> ( i );
                    lilv_node_free ( designation );
                }
            }
        }

        lilv_node_free ( audioClass );
        lilv_node_free ( controlClass );
        lilv_node_free ( atomClass );
        lilv_node_free ( inputClass );
        lilv_node_free ( outputClass );

        if ( audioOutLeftIdx < 0 )
        {
            errorDescription = "Plugin has no audio output ports.";
            return false;
        }

        // Allocate audio buffers
        audioInLeft.resize ( blockSize, 0.0f );
        audioInRight.resize ( blockSize, 0.0f );
        audioOutLeft.resize ( blockSize, 0.0f );
        audioOutRight.resize ( blockSize, 0.0f );

        // Allocate atom event buffers (64KB should be plenty for MIDI events + UI events)
        constexpr size_t kAtomBufferSize = 65536;
        atomInBuffer.resize ( kAtomBufferSize, 0 );
        atomOutBuffer.resize ( kAtomBufferSize, 0 );

        return true;
    }

    LilvWorld*    world    { nullptr };
    const LilvPlugin* plugin { nullptr };
    LilvInstance* instance { nullptr };
    const LV2_Worker_Interface* workerInterface { nullptr };
    bool          bActive  { false };

    int sampleRate { 0 };
    int blockSize  { 0 };
    int channels   { 0 };
    std::string savedPluginURI;

    // Port indices
    int audioInLeftIdx   { -1 };
    int audioInRightIdx  { -1 };
    int audioOutLeftIdx  { -1 };
    int audioOutRightIdx { -1 };
    int atomInIdx        { -1 };
    int atomOutIdx       { -1 };
    int freewheelIdx     { -1 };

    // Audio buffers (deinterleaved)
    std::vector<float> audioInLeft;
    std::vector<float> audioInRight;
    std::vector<float> audioOutLeft;
    std::vector<float> audioOutRight;

    // Atom event buffers
    std::vector<uint8_t> atomInBuffer;
    std::vector<uint8_t> atomOutBuffer;

    // Control port values
    float freewheelValue { 0.0f };

    // LV2 options
    LV2_Options_Option optionSampleRate {};
    LV2_Options_Option optionMaxBlockLength {};
    LV2_Options_Option optionNomBlockLength {};
    LV2_Options_Option optionTerminator {};
    LV2_Options_Option options[4] {};

    // LV2 features
    LV2_Feature featureMap {};
    LV2_Feature featureUnmap {};
    LV2_Feature featureOptions {};
    LV2_Feature featureBufSizeBounded {};
    LV2_Feature featureWorkerSchedule {};
    LV2_Worker_Schedule featureWorkerScheduleData {};
    const LV2_Feature* features[6] {};

    // UI state
    LV2UI_Handle              uiInstance    { nullptr };
    const LV2UI_Descriptor*   uiDescriptor  { nullptr };
    const LV2UI_Show_Interface* showInterface { nullptr };
    const LV2UI_Idle_Interface* idleInterface { nullptr };
    void*                     uiLibHandle   { nullptr };
    bool                      bEditorVisible { false };
    std::atomic<bool>         bShowCompleted { true };
    
    // Concurrency
    std::mutex workerMutex;
    std::condition_variable workerCv;
    std::queue<WorkerJob> workerJobs;
    bool workerQuit { false };
    std::thread workerThread;

    std::mutex responseMutex;
    std::queue<WorkerJob> responseJobs;

    std::mutex uiEventMutex;
    std::vector<UiEvent> uiEvents;

    // DSP -> UI communication
    std::mutex dspToUiMutex;
    std::vector<uint8_t> dspToUiEvents;
};

} // namespace

// ---------------------------------------------------------------------------
// Public C API
// ---------------------------------------------------------------------------

plugin_handle_t lv2_create_from_uri ( const char* pluginURI, int sampleRate, int blockSize, int numChannels )
{
    if ( !pluginURI || sampleRate <= 0 || blockSize <= 0 || ( numChannels != 1 && numChannels != 2 ) )
        return nullptr;

    std::unique_ptr<Lv2Runtime> runtime(new Lv2Runtime());
    std::string errorDescription;
    if ( !runtime->load ( pluginURI, sampleRate, blockSize, numChannels, errorDescription ) )
    {
        qWarning() << "lv2_adapter:" << QString::fromStdString ( errorDescription );
        return nullptr;
    }

    return runtime.release();
}

void lv2_destroy_handle ( plugin_handle_t h )
{
    delete static_cast<Lv2Runtime*> ( h );
}

void lv2_process_handle ( plugin_handle_t h, float* interleaved, int numFrames, int numChannels, const void* midiEvents, int numMidiEvents )
{
    if ( h )
    {
        auto* runtime = static_cast<Lv2Runtime*> ( h );
        runtime->process ( interleaved, numFrames, numChannels, midiEvents, numMidiEvents );
    }
}

bool lv2_show_editor_handle ( plugin_handle_t h )
{
    if ( auto* runtime = static_cast<Lv2Runtime*> ( h ) )
    {
        std::string errorDescription;
        if ( runtime->showEditor ( errorDescription ) )
            return true;

        qWarning() << "lv2_adapter:" << QString::fromStdString ( errorDescription );
    }
    return false;
}

bool lv2_close_editor_handle ( plugin_handle_t h )
{
    if ( auto* runtime = static_cast<Lv2Runtime*> ( h ) )
        return runtime->closeEditor();

    return false;
}

bool lv2_idle_editor_handle ( plugin_handle_t h )
{
    if ( auto* runtime = static_cast<Lv2Runtime*> ( h ) )
        return runtime->idleEditor();

    return false;
}

bool lv2_is_editor_visible_handle ( plugin_handle_t h )
{
    if ( auto* runtime = static_cast<Lv2Runtime*> ( h ) )
        return runtime->isEditorVisible();

    return false;
}

bool lv2_load_preset_handle ( plugin_handle_t h, const char* presetPath )
{
    if ( auto* runtime = static_cast<Lv2Runtime*> ( h ) )
        return runtime->loadPreset(presetPath);

    return false;
}

char* lv2_save_state_handle(plugin_handle_t inst, int* out_size)
{
    if (auto* rt = static_cast<Lv2Runtime*>(inst))
    {
        QByteArray data = rt->saveState();
        if (!data.isEmpty())
        {
            char* copy = static_cast<char*>(malloc(data.size()));
            if (copy)
            {
                memcpy(copy, data.constData(), data.size());
                if (out_size) *out_size = data.size();
                return copy;
            }
        }
    }
    if (out_size) *out_size = 0;
    return nullptr;
}

bool lv2_restore_state_handle(plugin_handle_t inst, const char* data, int size)
{
    if (auto* rt = static_cast<Lv2Runtime*>(inst))
    {
        return rt->restoreState(data, size);
    }
    return false;
}

int lv2_scan_plugins ( struct Lv2PluginInfo** outPlugins )
{
    if ( !outPlugins )
        return 0;

    *outPlugins = nullptr;

    LilvWorld* world = lilv_world_new();
    if ( !world )
        return 0;

    lilv_world_load_all ( world );

    const LilvPlugins* plugins = lilv_world_get_all_plugins ( world );
    int count = lilv_plugins_size ( plugins );

    if ( count <= 0 )
    {
        lilv_world_free ( world );
        return 0;
    }

    auto* results = new Lv2PluginInfo[count];
    std::memset ( results, 0, sizeof ( Lv2PluginInfo ) * count );

    int idx = 0;
    LILV_FOREACH ( plugins, iter, plugins )
    {
        const LilvPlugin* p = lilv_plugins_get ( plugins, iter );
        const LilvNode* uriNode = lilv_plugin_get_uri ( p );
        LilvNode* nameNode = lilv_plugin_get_name ( p );

        if ( uriNode )
        {
            const char* uri = lilv_node_as_uri ( uriNode );
            if ( uri )
                std::strncpy ( results[idx].uri, uri, sizeof ( results[idx].uri ) - 1 );
        }

        if ( nameNode )
        {
            const char* name = lilv_node_as_string ( nameNode );
            if ( name )
                std::strncpy ( results[idx].name, name, sizeof ( results[idx].name ) - 1 );
            lilv_node_free ( nameNode );
        }

        ++idx;
    }

    lilv_world_free ( world );

    *outPlugins = results;
    return count;
}

void lv2_free_scan_results ( struct Lv2PluginInfo* plugins )
{
    delete[] plugins;
}

#else // !HAVE_LV2

#include <QDebug>

plugin_handle_t lv2_create_from_uri ( const char* pluginURI, int sampleRate, int blockSize, int numChannels )
{
    Q_UNUSED ( pluginURI );
    Q_UNUSED ( sampleRate );
    Q_UNUSED ( blockSize );
    Q_UNUSED ( numChannels );
    qWarning() << "lv2_adapter: LV2 support not compiled in.";
    return nullptr;
}

void lv2_destroy_handle ( plugin_handle_t h ) { Q_UNUSED ( h ); }

void lv2_process_handle ( plugin_handle_t h, float* interleaved, int numFrames, int numChannels, const void* midiEvents, int numMidiEvents )
{
    (void)h;
    (void)interleaved;
    (void)numFrames;
    (void)numChannels;
    (void)midiEvents;
    (void)numMidiEvents;
}

bool lv2_show_editor_handle ( plugin_handle_t h )
{
    Q_UNUSED ( h );
    qWarning() << "lv2_adapter: LV2 support not compiled in.";
    return false;
}

bool lv2_close_editor_handle ( plugin_handle_t h )
{
    Q_UNUSED ( h );
    return true;
}

bool lv2_idle_editor_handle ( plugin_handle_t h )
{
    Q_UNUSED ( h );
    return false;
}

bool lv2_is_editor_visible_handle ( plugin_handle_t h )
{
    Q_UNUSED ( h );
    return false;
}

bool lv2_load_preset_handle ( plugin_handle_t h, const char* presetPath )
{
    Q_UNUSED ( h );
    Q_UNUSED ( presetPath );
    return false;
}

char* lv2_save_state_handle(plugin_handle_t inst, int* out_size)
{
    Q_UNUSED ( inst );
    if (out_size) *out_size = 0;
    return nullptr;
}

bool lv2_restore_state_handle(plugin_handle_t inst, const char* data, int size)
{
    Q_UNUSED ( inst );
    Q_UNUSED ( data );
    Q_UNUSED ( size );
    return false;
}

int lv2_scan_plugins ( struct Lv2PluginInfo** outPlugins )
{
    if ( outPlugins )
        *outPlugins = nullptr;
    return 0;
}

void lv2_free_scan_results ( struct Lv2PluginInfo* plugins )
{
    Q_UNUSED ( plugins );
}

#endif // HAVE_LV2
