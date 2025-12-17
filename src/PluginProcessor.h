#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "client.h"
#include <memory>

class JamulusVSTAudioProcessor : public juce::AudioProcessor
{
public:
    JamulusVSTAudioProcessor();
    ~JamulusVSTAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Custom Jamulus Methods
    CClient* getClient() { return jamulusClient.get(); }
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    void setServerAddress(const juce::String& address) { serverAddress = address; }
    juce::String getServerAddress() const { return serverAddress; }

private:
    std::unique_ptr<CClient> jamulusClient;
    juce::AudioProcessorValueTreeState apvts;
    juce::String serverAddress { "127.0.0.1" };

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JamulusVSTAudioProcessor)
};
