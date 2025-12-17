#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <QTimer>
#include "client.h"
#include "ServerListListener.h"

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

    // Server List
    juce::Label directoryLabel { "Directory:", "Directory:" };
    juce::ComboBox directoryBox;
    juce::TextButton fetchListButton { "Get List" };
    juce::ComboBox serverListBox;

    // Manual Address
    juce::Label serverAddressLabel { "Manual Addr:", "Manual Addr:" };
    juce::TextEditor serverAddressEditor;

    // Settings
    juce::Label inputFaderLabel { "Input", "Input" };
    juce::Slider inputFader;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputFaderAttachment;

    juce::Label connectionStatusLabel;

    // Logic
    std::unique_ptr<ServerListListener> serverListListener;
    std::vector<CServerInfo> currentServerList;

    void populateDirectoryBox();
    void fetchServerList();
    void updateServerListBox();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JamulusVSTAudioProcessorEditor)
};
