#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <QCoreApplication>
#include <QThread>

// Static QApp instance
static int argc = 1;
static char* argv[] = { (char*)"JamulusVST", nullptr };
static std::unique_ptr<QCoreApplication> qAppInstance;

JamulusVSTAudioProcessor::JamulusVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // Initialize Qt Core Application if not exists
    if (!QCoreApplication::instance())
    {
        qAppInstance = std::make_unique<QCoreApplication>(argc, argv);
    }

    // Initialize CClient on Main Thread (Constructor)
    // This ensures CClient lives on the Message Thread, so it can receive signals
    // from the Network Thread when we pump the event loop.
    if (!jamulusClient)
    {
        jamulusClient = std::make_unique<CClient>(
            0, // Port
            0, // QoS
            "", // Address
            "", // Midi
            true, // No auto jack
            "JamulusVST", // Name
            false, // IPv6
            false // Mute me
        );
    }
}

JamulusVSTAudioProcessor::~JamulusVSTAudioProcessor()
{
    jamulusClient.reset();
}

juce::AudioProcessorValueTreeState::ParameterLayout JamulusVSTAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "inputLevel",
        "Input Level",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        50.0f));

    return layout;
}

const juce::String JamulusVSTAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JamulusVSTAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JamulusVSTAudioProcessor::producesMidi() const
{
   #if JucePlugin_WantsMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JamulusVSTAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JamulusVSTAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JamulusVSTAudioProcessor::getNumPrograms()
{
    return 1;
}

int JamulusVSTAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JamulusVSTAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String JamulusVSTAudioProcessor::getProgramName (int index)
{
    return {};
}

void JamulusVSTAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void JamulusVSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Start client if not running
    if (jamulusClient && !jamulusClient->IsRunning())
    {
        // CClient::Start() calls Init() which handles buffer sizes
        // We might need to pass sample rate info if Jamulus wasn't fixed to 48k?
        // Jamulus is fixed to 48k. If host is not 48k, pitch will be wrong (Resampling needed).
        // For now, assuming host is 48k or user accepts it.
        jamulusClient->Start();
    }
}

void JamulusVSTAudioProcessor::releaseResources()
{
    // Do NOT stop the client here if we want to keep connection alive across track disable/enable?
    // VST3 releaseResources is called when processing stops.
    // If we stop the client, we disconnect.
    // Ideally we keep it running or reconnect.
    // Let's stop to be safe and save resources.
    if (jamulusClient && jamulusClient->IsRunning())
    {
        jamulusClient->Stop();
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool JamulusVSTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void JamulusVSTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    if (jamulusClient) {
        // Apply Parameters
        float inputLevel = *apvts.getRawParameterValue("inputLevel");
        if (std::abs(inputLevel - jamulusClient->GetAudioInFader()) > 0.1f) {
             jamulusClient->SetAudioInFader(static_cast<int>(inputLevel));
        }

        // Process Audio
        jamulusClient->GetSound().ProcessAudioBlockVST(
           buffer.getArrayOfReadPointers(),
           totalNumInputChannels,
           buffer.getArrayOfWritePointers(),
           totalNumOutputChannels,
           buffer.getNumSamples()
        );
    }
}

bool JamulusVSTAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* JamulusVSTAudioProcessor::createEditor()
{
    return new JamulusVSTAudioProcessorEditor (*this);
}

void JamulusVSTAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Save custom properties (Server Address)
    state.setProperty("serverAddress", serverAddress, nullptr);

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void JamulusVSTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

            // Restore server address
            serverAddress = apvts.state.getProperty("serverAddress", "127.0.0.1");
        }
    }
}

// Instantiate
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JamulusVSTAudioProcessor();
}
