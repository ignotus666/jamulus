#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <QTimer>
#include "client.h"

class JamulusVSTAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    JamulusVSTAudioProcessorEditor (JamulusVSTAudioProcessor&);
    ~JamulusVSTAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    JamulusVSTAudioProcessor& audioProcessor;

    // GUI Components
    juce::TextButton connectButton { "Connect" };
    juce::TextButton disconnectButton { "Disconnect" };
    juce::Label serverAddressLabel { "Server:", "Server:" };
    juce::TextEditor serverAddressEditor;

    // Faders
    juce::Slider inputFader;
    juce::Label inputFaderLabel { "Input", "Input" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputFaderAttachment;

    juce::Label connectionStatusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JamulusVSTAudioProcessorEditor)
};
