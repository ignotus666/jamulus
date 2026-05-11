/*
 * VST3 adapter for Jamulus plugin hosting.
 *
 * This implementation loads a VST3 bundle on Linux, resolves the exported
 * factory, instantiates the first audio-effect class, configures a simple
 * mono/stereo insert path, and processes interleaved Jamulus audio in-place.
 */

#include "vst3_adapter.h"

#if defined(HAVE_VST3)

#ifndef DEVELOPMENT
#define DEVELOPMENT 1
#endif

#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>

#include <dlfcn.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/smartpointer.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/gui/iplugview.h"
#include "public.sdk/source/vst/vstbus.h"
#include "public.sdk/source/vst/hosting/processdata.h"
#include "public.sdk/source/vst/hosting/eventlist.h"

// forward declaration of the thread-local MIDI events vector (defined later)
extern thread_local std::vector<std::pair<uint8_t, std::vector<uint8_t>>> g_midiEventsForFrame;

namespace
{
using ModuleEntryFunc = bool (PLUGIN_API*) (void*);
using ModuleExitFunc = bool (PLUGIN_API*) ();
using GetPluginFactoryFunc = Steinberg::IPluginFactory* (PLUGIN_API*) ();

class HostApplication final : public Steinberg::Vst::IHostApplication
{
public:
    Steinberg::tresult PLUGIN_API getName ( Steinberg::Vst::String128 name ) override
    {
        if ( !name )
            return Steinberg::kInvalidArgument;

        name[0] = 0;
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API createInstance ( Steinberg::TUID cid, Steinberg::TUID iid, void** obj ) override
    {
        Q_UNUSED ( cid );
        Q_UNUSED ( iid );

        if ( !obj )
            return Steinberg::kInvalidArgument;

        *obj = nullptr;
        return Steinberg::kNoInterface;
    }

    Steinberg::tresult PLUGIN_API queryInterface ( const Steinberg::TUID _iid, void** obj ) override
    {
        if ( !obj )
            return Steinberg::kInvalidArgument;

        *obj = nullptr;
        if ( Steinberg::FUnknownPrivate::iidEqual ( _iid, Steinberg::Vst::IHostApplication::iid ) )
        {
            *obj = static_cast<Steinberg::Vst::IHostApplication*> ( this );
            return Steinberg::kResultOk;
        }

        return Steinberg::kNoInterface;
    }

    Steinberg::uint32 PLUGIN_API addRef() override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; }
};

static std::string FindVst3Binary ( const std::string& path )
{
    QFileInfo info ( QString::fromStdString ( path ) );
    if ( info.isFile() && info.suffix().compare ( QLatin1String ( "so" ), Qt::CaseInsensitive ) == 0 )
        return path;

    if ( !info.isDir() )
        return {};

    QDirIterator it ( QString::fromStdString ( path ), QStringList() << "*.so",
                      QDir::Files, QDirIterator::Subdirectories );
    while ( it.hasNext() )
    {
        const QString candidate = it.next();
        if ( candidate.contains ( QLatin1String ( "/Contents/" ) ) )
            return candidate.toStdString();
    }

    return {};
}

    static bool HasEffectCategory ( const Steinberg::PClassInfo& info )
{
        return std::strncmp ( info.category, kVstAudioEffectClass,
                          sizeof ( info.category ) ) == 0;
}

static bool IsMonoOrStereo ( int32_t channels )
{
    return channels == 1 || channels == 2;
}

class Vst3Runtime
{
public:
    ~Vst3Runtime () { shutdown (); }

    bool load ( const std::string& vst3Path, int sampleRateHz, int blockSizeFrames, int hostChannels,
                std::string& errorDescription )
    {
        shutdown ();

        sampleRate = sampleRateHz;
        blockSize = blockSizeFrames;
        channels = hostChannels;

        binaryPath = FindVst3Binary ( vst3Path );
        if ( binaryPath.empty() )
        {
            errorDescription = "No VST3 binary (.so) found inside bundle: " + vst3Path;
            return false;
        }

        moduleHandle = dlopen ( binaryPath.c_str(), RTLD_NOW );
        if ( !moduleHandle )
        {
            errorDescription = "dlopen failed: ";
            errorDescription += dlerror();
            return false;
        }

        moduleEntry = reinterpret_cast<ModuleEntryFunc> ( dlsym ( moduleHandle, "ModuleEntry" ) );
        moduleExit = reinterpret_cast<ModuleExitFunc> ( dlsym ( moduleHandle, "ModuleExit" ) );
        getFactory = reinterpret_cast<GetPluginFactoryFunc> ( dlsym ( moduleHandle, "GetPluginFactory" ) );

        if ( !moduleEntry || !moduleExit || !getFactory )
        {
            errorDescription = "VST3 binary is missing ModuleEntry, ModuleExit, or GetPluginFactory.";
            shutdown ();
            return false;
        }

        if ( !moduleEntry ( moduleHandle ) )
        {
            errorDescription = "ModuleEntry failed.";
            shutdown ();
            return false;
        }

        factory = Steinberg::owned ( getFactory() );
        if ( !factory )
        {
            errorDescription = "GetPluginFactory returned nullptr.";
            shutdown ();
            return false;
        }

        if ( !instantiateProcessor ( errorDescription ) )
        {
            shutdown ();
            return false;
        }

        if ( !configureProcessing ( errorDescription ) )
        {
            shutdown ();
            return false;
        }

        return true;
    }

    void shutdown ()
    {
        if ( plugView )
        {
            qDebug() << "vst3_adapter: shutdown - removing view";
            plugView->removed();
            plugView = nullptr;
            bEditorAttached = false;
        }

        if ( processor )
        {
            processor->setProcessing ( false );
        }
        if ( component )
        {
            component->setActive ( false );
            component->terminate ();
        }

        if ( connectionPointComponent && connectionPointController )
        {
            connectionPointComponent->disconnect ( connectionPointController );
            connectionPointController->disconnect ( connectionPointComponent );
        }

        if ( controller )
        {
            controller->terminate();
        }

        processData.unprepare ();
        processor = nullptr;
        component = nullptr;
        controller = nullptr;
        connectionPointComponent = nullptr;
        connectionPointController = nullptr;
        factory = nullptr;

        if ( moduleHandle )
        {
            if ( moduleExit )
                moduleExit ();
            dlclose ( moduleHandle );
        }

        moduleHandle = nullptr;
        moduleEntry = nullptr;
        moduleExit = nullptr;
        getFactory = nullptr;
        sampleRate = 0;
        blockSize = 0;
        channels = 0;
        pluginInputChannels = 0;
        pluginOutputChannels = 0;
        activeInputBusIndex = -1;
        activeOutputBusIndex = -1;
        inputBusCount = 0;
        outputBusCount = 0;
        symbolicSampleSize = Steinberg::Vst::kSample32;
        bEditorAttached = false;
        binaryPath.clear();
    }

    bool showEditor ( void* parentWindow, std::string& errorDescription )
    {
        if ( !controller )
        {
            errorDescription = "Plugin does not provide an edit controller.";
            return false;
        }

        if ( !parentWindow )
        {
            errorDescription = "Invalid native parent window handle.";
            return false;
        }

        if ( !plugView )
        {
            plugView = controller->createView ( Steinberg::Vst::ViewType::kEditor );
        }

        if ( !plugView )
        {
            errorDescription = "Plugin did not return an editor view.";
            return false;
        }

#if defined( Q_OS_LINUX )
        const char* platformType = Steinberg::kPlatformTypeX11EmbedWindowID;
#elif defined( Q_OS_MACOS )
        const char* platformType = Steinberg::kPlatformTypeNSView;
#elif defined( Q_OS_WIN )
        const char* platformType = Steinberg::kPlatformTypeHWND;
#else
        const char* platformType = Steinberg::kPlatformTypeX11EmbedWindowID;
#endif

        if ( plugView->isPlatformTypeSupported ( platformType ) != Steinberg::kResultTrue )
        {
            errorDescription = "Plugin view does not support this platform type.";
            return false;
        }

        if ( bEditorAttached )
            return true;

        if ( plugView->attached ( parentWindow, platformType ) != Steinberg::kResultTrue )
        {
            errorDescription = "Plugin view attach failed.";
            return false;
        }

        bEditorAttached = true;
        qDebug() << "vst3_adapter: editor attached";
        return true;
    }

    bool closeEditor ()
    {
        qDebug() << "vst3_adapter: closeEditor called, plugView=" << plugView.get() << "bEditorAttached=" << bEditorAttached;
        
        if ( plugView && bEditorAttached )
        {
            qDebug() << "vst3_adapter: closeEditor - calling removed()";
            try
            {
                   qDebug() << "vst3_adapter: closeEditor - about to call plugView->removed()";
                plugView->removed();
                   qDebug() << "vst3_adapter: closeEditor - removed() succeeded, nullifying plugView";
            }
            catch ( const std::exception& e )
            {
                qWarning() << "vst3_adapter: closeEditor - exception during removed():" << e.what();
            }
            catch ( ... )
            {
                qWarning() << "vst3_adapter: closeEditor - unknown exception during removed()";
            }
            
            plugView = nullptr;
            bEditorAttached = false;
               qDebug() << "vst3_adapter: closeEditor completed successfully";
            return true;
        }

           qDebug() << "vst3_adapter: closeEditor - no active editor to close";
        return true;
    }

    void process ( float* interleaved, int numFrames, int numChannels )
    {
        if ( !processor || !component || !interleaved || numFrames <= 0 || numChannels <= 0 )
            return;

        if ( pluginOutputChannels <= 0 || activeOutputBusIndex < 0 )
            return;

        if ( numChannels != channels )
            return;

        if ( numFrames > blockSize )
        {
            // Reconfigure for a larger block only if needed.
            if ( !processData.prepare ( *component, numFrames, symbolicSampleSize ) )
                return;
            blockSize = numFrames;
        }
        else
        {
            processData.prepare ( *component, blockSize, symbolicSampleSize );
        }

        processData.processMode = Steinberg::Vst::kRealtime;
        processData.numSamples = numFrames;

        // Insert MIDI events collected for this frame into the processData.inputEvents
        if ( !g_midiEventsForFrame.empty() )
        {
            qDebug() << "vst3_adapter::process: converting" << g_midiEventsForFrame.size() << "MIDI events";
            // create an EventList big enough to hold the events
            Steinberg::Vst::EventList* pList = new Steinberg::Vst::EventList ( static_cast<int32_t> ( g_midiEventsForFrame.size() ) );
            Steinberg::Vst::Event e;

            for ( const auto & me : g_midiEventsForFrame )
            {
                if ( me.second.empty() )
                    continue;
                    
                const uint8_t status = me.second[0];
                   qDebug() << "vst3_adapter::process: MIDI status 0x" << Qt::hex << status << Qt::dec << "length" << me.second.size();
                
                   // Set common fields
                e.sampleOffset = static_cast<int32_t>(me.first);
                e.busIndex = 0;
                   e.ppqPosition = 0.0;
                e.flags = 0;
                
                   // Determine event type based on MIDI message type
                   const uint8_t channel = status & 0x0F;
                   const uint8_t messageType = status & 0xF0;
                   if ( me.second.size() >= 3 )
                   {
                           const uint8_t data1 = me.second[1];
                           const uint8_t data2 = me.second[2];
                       
                           if ( messageType == 0x90 )  // Note On
                           {
                               e.type = Steinberg::Vst::Event::kNoteOnEvent;
                              e.noteOn.channel = static_cast<Steinberg::int16>(channel);
                              e.noteOn.pitch = static_cast<Steinberg::int16>(data1);
                               e.noteOn.velocity = static_cast<float>(data2) / 127.0f;
                               e.noteOn.tuning = 0.0f;
                               e.noteOn.length = 0;
                               e.noteOn.noteId = -1;
                               qDebug() << "vst3_adapter::process: NoteOn ch" << channel << "pitch" << data1 << "vel" << data2;
                           }
                           else if ( messageType == 0x80 )  // Note Off
                           {
                               e.type = Steinberg::Vst::Event::kNoteOffEvent;
                              e.noteOff.channel = static_cast<Steinberg::int16>(channel);
                              e.noteOff.pitch = static_cast<Steinberg::int16>(data1);
                               e.noteOff.velocity = static_cast<float>(data2) / 127.0f;
                               e.noteOff.noteId = -1;
                               e.noteOff.tuning = 0.0f;
                               qDebug() << "vst3_adapter::process: NoteOff ch" << channel << "pitch" << data1 << "vel" << data2;
                           }
                           else if ( messageType == 0xA0 )  // Polyphonic aftertouch
                           {
                               e.type = Steinberg::Vst::Event::kPolyPressureEvent;
                               e.polyPressure.channel = static_cast<Steinberg::int16> ( channel );
                               e.polyPressure.pitch = static_cast<Steinberg::int16> ( data1 );
                               e.polyPressure.pressure = static_cast<float> ( data2 ) / 127.0f;
                               e.polyPressure.noteId = -1;
                               qDebug() << "vst3_adapter::process: PolyPressure ch" << channel << "pitch" << data1 << "pressure" << data2;
                           }
                           else if ( messageType == 0xB0 )  // Control Change
                           {
                               // VST3 has no generic short-MIDI CC input event type; many plugins
                               // still accept legacy CC events on the event bus.
                               e.type = Steinberg::Vst::Event::kLegacyMIDICCOutEvent;
                               e.midiCCOut.controlNumber = data1;
                               e.midiCCOut.channel = static_cast<Steinberg::int8> ( channel );
                               e.midiCCOut.value = static_cast<Steinberg::int8> ( data2 );
                               e.midiCCOut.value2 = 0;
                               qDebug() << "vst3_adapter::process: LegacyMIDICC ch" << channel << "cc" << data1 << "value" << data2;
                           }
                           else
                           {
                               // Preserve other MIDI bytes as raw data for plugins that parse legacy MIDI.
                               e.type = Steinberg::Vst::Event::kDataEvent;
                               e.data.type = Steinberg::Vst::DataEvent::kMidiSysEx;
                               e.data.size = static_cast<Steinberg::uint32> ( me.second.size() );
                               e.data.bytes = me.second.data();
                               if ( messageType == 0xD0 )
                                   qDebug() << "vst3_adapter::process: ChannelPressure value" << data1;
                               else
                                   qDebug() << "vst3_adapter::process: DataEvent (other MIDI type)";
                           }
                   }
                   else
                   {
                           // Use raw data for short/unknown MIDI messages as well.
                       e.type = Steinberg::Vst::Event::kDataEvent;
                       e.data.type = Steinberg::Vst::DataEvent::kMidiSysEx;
                       e.data.size = static_cast<Steinberg::uint32> ( me.second.size() );
                       e.data.bytes = me.second.data();
                           qDebug() << "vst3_adapter::process: DataEvent (short MIDI/message length" << me.second.size() << ")";
                   }

                pList->addEvent ( e );
                   qDebug() << "vst3_adapter::process: added event at offset" << e.sampleOffset;
            }

            processData.inputEvents = pList;
        }

        if ( symbolicSampleSize == Steinberg::Vst::kSample32 )
        {
            copyToProcessBuffers32 ( interleaved, numFrames );
            if ( processor->process ( processData ) == Steinberg::kResultTrue )
                copyFromProcessBuffers32 ( interleaved, numFrames );
        }
        else
        {
            copyToProcessBuffers64 ( interleaved, numFrames );
            if ( processor->process ( processData ) == Steinberg::kResultTrue )
                copyFromProcessBuffers64 ( interleaved, numFrames );
        }

        // cleanup inputEvents if we allocated one
        if ( processData.inputEvents )
        {
            delete static_cast<Steinberg::Vst::EventList*> ( processData.inputEvents );
            processData.inputEvents = nullptr;
        }
        // clear the thread-local MIDI event vector after processing
        if ( !g_midiEventsForFrame.empty() )
            g_midiEventsForFrame.clear();
    }

private:
    bool instantiateProcessor ( std::string& errorDescription )
    {
        Steinberg::PClassInfo classInfo {};
        bool found = false;
        const int32_t classCount = factory->countClasses();
        for ( int32_t i = 0; i < classCount; ++i )
        {
            if ( factory->getClassInfo ( i, &classInfo ) == Steinberg::kResultTrue &&
                 HasEffectCategory ( classInfo ) )
            {
                found = true;
                break;
            }
        }

        if ( !found )
        {
            errorDescription = "No audio effect class found in VST3 factory.";
            return false;
        }

        Steinberg::FUID classId ( classInfo.cid );

        Steinberg::Vst::IComponent* rawComponent = nullptr;
        if ( factory->createInstance ( classId.toTUID(), Steinberg::Vst::IComponent::iid.toTUID(),
                                       reinterpret_cast<void**> ( &rawComponent ) ) != Steinberg::kResultTrue ||
             !rawComponent )
        {
            errorDescription = "Failed to create VST3 component instance.";
            return false;
        }

        component = rawComponent;
        processor = Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> ( component.get() );
        if ( !processor )
        {
            errorDescription = "Component does not expose IAudioProcessor.";
            return false;
        }

        if ( component->setIoMode ( Steinberg::Vst::kSimple ) != Steinberg::kResultTrue )
        {
            // Not fatal; many plugins ignore this.
        }

        if ( component->initialize ( &hostApplication ) != Steinberg::kResultTrue )
        {
            errorDescription = "Component initialize() failed.";
            return false;
        }

        Steinberg::Vst::BusInfo inBus {};
        Steinberg::Vst::BusInfo outBus {};
        inputBusCount = component->getBusCount ( Steinberg::Vst::kAudio, Steinberg::Vst::kInput );
        outputBusCount = component->getBusCount ( Steinberg::Vst::kAudio, Steinberg::Vst::kOutput );

        if ( outputBusCount < 1 ||
             component->getBusInfo ( Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, 0, outBus ) != Steinberg::kResultTrue )
        {
            errorDescription = "VST3 plugin must expose at least one audio output bus.";
            return false;
        }

        activeOutputBusIndex = 0;
        pluginOutputChannels = outBus.channelCount;
        if ( pluginOutputChannels < 1 )
        {
            errorDescription = "First VST3 output bus has no channels.";
            return false;
        }

        pluginInputChannels = 0;
        activeInputBusIndex = -1;
        if ( inputBusCount > 0 )
        {
            if ( component->getBusInfo ( Steinberg::Vst::kAudio, Steinberg::Vst::kInput, 0, inBus ) == Steinberg::kResultTrue )
            {
                activeInputBusIndex = 0;
                pluginInputChannels = inBus.channelCount;
            }
        }

        if ( !processData.prepare ( *component, blockSize, symbolicSampleSize ) )
        {
            errorDescription = "HostProcessData preparation failed.";
            return false;
        }

        Steinberg::TUID controllerClassId {};
        if ( component->getControllerClassId ( controllerClassId ) == Steinberg::kResultTrue )
        {
            Steinberg::Vst::IEditController* rawController = nullptr;
            if ( factory->createInstance ( controllerClassId,
                                           Steinberg::Vst::IEditController::iid.toTUID(),
                                           reinterpret_cast<void**> ( &rawController ) ) == Steinberg::kResultTrue &&
                 rawController != nullptr )
            {
                controller = rawController;
                if ( controller->initialize ( &hostApplication ) != Steinberg::kResultTrue )
                {
                    controller = nullptr;
                }
                else
                {
                    connectionPointComponent = Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint> ( component.get() );
                    connectionPointController = Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint> ( controller.get() );
                    if ( connectionPointComponent && connectionPointController )
                    {
                        connectionPointComponent->connect ( connectionPointController );
                        connectionPointController->connect ( connectionPointComponent );
                    }
                }
            }
        }

        return true;
    }

    bool configureProcessing ( std::string& errorDescription )
    {
        Steinberg::Vst::ProcessSetup setup {};
        setup.processMode = Steinberg::Vst::kRealtime;
        setup.symbolicSampleSize = symbolicSampleSize;
        setup.maxSamplesPerBlock = blockSize;
        setup.sampleRate = sampleRate;

        if ( processor->setupProcessing ( setup ) != Steinberg::kResultTrue )
        {
            errorDescription = "setupProcessing failed.";
            return false;
        }

        for ( int32_t bus = 0; bus < inputBusCount; ++bus )
        {
            const bool enable = ( bus == activeInputBusIndex );
            component->activateBus ( Steinberg::Vst::kAudio, Steinberg::Vst::kInput, bus, enable );
        }
        for ( int32_t bus = 0; bus < outputBusCount; ++bus )
        {
            const bool enable = ( bus == activeOutputBusIndex );
            component->activateBus ( Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, bus, enable );
        }

        if ( activeOutputBusIndex < 0 )
        {
            errorDescription = "activateBus failed.";
            return false;
        }

        if ( component->setActive ( true ) != Steinberg::kResultTrue ||
             processor->setProcessing ( true ) != Steinberg::kResultTrue )
        {
            errorDescription = "Failed to activate VST3 processor.";
            return false;
        }

        return true;
    }

    void copyToProcessBuffers32 ( float* interleaved, int numFrames )
    {
        auto* inputBus = processData.inputs;
        if ( !inputBus || pluginInputChannels <= 0 || activeInputBusIndex < 0 )
            return;

        auto& bus = inputBus[activeInputBusIndex];

        for ( int frame = 0; frame < numFrames; ++frame )
        {
            const int srcIndex = frame * channels;
            const float left = interleaved[srcIndex];
            const float right = channels > 1 ? interleaved[srcIndex + 1] : left;
            if ( pluginInputChannels == 1 )
                bus.channelBuffers32[0][frame] = 0.5f * ( left + right );
            else
            {
                bus.channelBuffers32[0][frame] = left;
                bus.channelBuffers32[1][frame] = right;
                for ( int ch = 2; ch < pluginInputChannels; ++ch )
                    bus.channelBuffers32[ch][frame] = 0.0f;
            }
        }
    }

    void copyFromProcessBuffers32 ( float* interleaved, int numFrames )
    {
        auto* outputBus = processData.outputs;
        if ( !outputBus || pluginOutputChannels <= 0 || activeOutputBusIndex < 0 )
            return;

        auto& bus = outputBus[activeOutputBusIndex];

        for ( int frame = 0; frame < numFrames; ++frame )
        {
            const int dstIndex = frame * channels;
            if ( pluginOutputChannels == 1 )
            {
                const float sample = bus.channelBuffers32[0][frame];
                interleaved[dstIndex] = sample;
                if ( channels > 1 )
                    interleaved[dstIndex + 1] = sample;
            }
            else
            {
                interleaved[dstIndex] = bus.channelBuffers32[0][frame];
                if ( channels > 1 )
                    interleaved[dstIndex + 1] = bus.channelBuffers32[1][frame];
            }
        }
    }

    void copyToProcessBuffers64 ( float* interleaved, int numFrames )
    {
        auto* inputBus = processData.inputs;
        if ( !inputBus || pluginInputChannels <= 0 || activeInputBusIndex < 0 )
            return;

        auto& bus = inputBus[activeInputBusIndex];

        for ( int frame = 0; frame < numFrames; ++frame )
        {
            const int srcIndex = frame * channels;
            const double left = interleaved[srcIndex];
            const double right = channels > 1 ? interleaved[srcIndex + 1] : left;
            if ( pluginInputChannels == 1 )
                bus.channelBuffers64[0][frame] = 0.5 * ( left + right );
            else
            {
                bus.channelBuffers64[0][frame] = left;
                bus.channelBuffers64[1][frame] = right;
                for ( int ch = 2; ch < pluginInputChannels; ++ch )
                    bus.channelBuffers64[ch][frame] = 0.0;
            }
        }
    }

    void copyFromProcessBuffers64 ( float* interleaved, int numFrames )
    {
        auto* outputBus = processData.outputs;
        if ( !outputBus || pluginOutputChannels <= 0 || activeOutputBusIndex < 0 )
            return;

        auto& bus = outputBus[activeOutputBusIndex];

        for ( int frame = 0; frame < numFrames; ++frame )
        {
            const int dstIndex = frame * channels;
            if ( pluginOutputChannels == 1 )
            {
                const float sample = static_cast<float> ( bus.channelBuffers64[0][frame] );
                interleaved[dstIndex] = sample;
                if ( channels > 1 )
                    interleaved[dstIndex + 1] = sample;
            }
            else
            {
                interleaved[dstIndex] = static_cast<float> ( bus.channelBuffers64[0][frame] );
                if ( channels > 1 )
                    interleaved[dstIndex + 1] = static_cast<float> ( bus.channelBuffers64[1][frame] );
            }
        }
    }

    void* moduleHandle {nullptr};
    ModuleEntryFunc moduleEntry {nullptr};
    ModuleExitFunc moduleExit {nullptr};
    GetPluginFactoryFunc getFactory {nullptr};

    Steinberg::IPtr<Steinberg::IPluginFactory> factory;
    Steinberg::OPtr<Steinberg::Vst::IComponent> component;
    Steinberg::OPtr<Steinberg::Vst::IEditController> controller;
    Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint> connectionPointComponent;
    Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint> connectionPointController;
    Steinberg::OPtr<Steinberg::IPlugView> plugView;
    Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> processor;
    Steinberg::Vst::HostProcessData processData;

    std::string binaryPath;
    int sampleRate {0};
    int blockSize {0};
    int channels {0};
    int pluginInputChannels {0};
    int pluginOutputChannels {0};
    int activeInputBusIndex {-1};
    int activeOutputBusIndex {-1};
    int32_t inputBusCount {0};
    int32_t outputBusCount {0};
    Steinberg::Vst::SymbolicSampleSizes symbolicSampleSize {Steinberg::Vst::kSample32};
    bool bEditorAttached {false};
    HostApplication hostApplication;
};
} // namespace

// Thread-local storage for MIDI events to be processed in the current frame
thread_local std::vector<std::pair<uint8_t, std::vector<uint8_t>>> g_midiEventsForFrame;

void vst3_set_midi_events ( const std::vector<std::pair<uint8_t, std::vector<uint8_t>>>& midiEvents )
{
    qDebug() << "vst3_set_midi_events: received" << midiEvents.size() << "events";
    for ( size_t i = 0; i < midiEvents.size(); ++i )
    {
        const auto& evt = midiEvents[i];
        if ( !evt.second.empty() )
            qDebug() << "  event[" << i << "]: offset=" << evt.first << "status=" << evt.second[0] << "length=" << evt.second.size();
    }
    g_midiEventsForFrame = midiEvents;
}

plugin_handle_t vst3_create_from_path ( const char* sPath, int sampleRate, int blockSize, int numChannels )
{
    if ( !sPath || sampleRate <= 0 || blockSize <= 0 || !IsMonoOrStereo ( numChannels ) )
        return nullptr;

    std::unique_ptr<Vst3Runtime> runtime ( new Vst3Runtime );
    std::string errorDescription;
    if ( !runtime->load ( sPath, sampleRate, blockSize, numChannels, errorDescription ) )
    {
        qWarning() << "vst3_adapter:" << QString::fromStdString ( errorDescription );
        return nullptr;
    }

    return runtime.release();
}

void vst3_destroy_handle ( plugin_handle_t h )
{
    delete static_cast<Vst3Runtime*> ( h );
}

void vst3_process_handle ( plugin_handle_t h, float* interleaved, int numFrames, int numChannels )
{
    if ( auto* runtime = static_cast<Vst3Runtime*> ( h ) )
        runtime->process ( interleaved, numFrames, numChannels );
}

bool vst3_show_editor_handle ( plugin_handle_t h, void* parentWindow )
{
    if ( auto* runtime = static_cast<Vst3Runtime*> ( h ) )
    {
        std::string errorDescription;
        if ( runtime->showEditor ( parentWindow, errorDescription ) )
            return true;

        qWarning() << "vst3_adapter:" << QString::fromStdString ( errorDescription );
    }
    return false;
}

bool vst3_close_editor_handle ( plugin_handle_t h )
{
    if ( auto* runtime = static_cast<Vst3Runtime*> ( h ) )
        return runtime->closeEditor();

    return false;
}

#else

plugin_handle_t vst3_create_from_path ( const char* sPath, int sampleRate, int blockSize,
                                        int numChannels )
{
    Q_UNUSED ( sPath );
    Q_UNUSED ( sampleRate );
    Q_UNUSED ( blockSize );
    Q_UNUSED ( numChannels );
    qWarning() << "vst3_adapter: VST3 support not compiled in.";
    return nullptr;
}

void vst3_destroy_handle ( plugin_handle_t h )
{
    Q_UNUSED ( h );
}

void vst3_process_handle ( plugin_handle_t h, float* interleaved, int numFrames, int numChannels )
{
    Q_UNUSED ( h );
    Q_UNUSED ( interleaved );
    Q_UNUSED ( numFrames );
    Q_UNUSED ( numChannels );
}

bool vst3_show_editor_handle ( plugin_handle_t h, void* parentWindow )
{
    Q_UNUSED ( h );
    Q_UNUSED ( parentWindow );
    qWarning() << "vst3_adapter: VST3 support not compiled in.";
    return false;
}

bool vst3_close_editor_handle ( plugin_handle_t h )
{
    Q_UNUSED ( h );
    return true;
}

#endif
