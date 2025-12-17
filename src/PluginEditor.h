#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <QTimer>
#include "client.h"
#include "JamulusBridge.h"

// Component for a single channel strip
class ChannelStrip : public juce::Component
{
public:
    ChannelStrip()
    {
        addAndMakeVisible(fader);
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        fader.setRange(0.0, 100.0);

        addAndMakeVisible(nameLabel);
        nameLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(levelMeter);
        levelMeter.setColour(juce::ProgressBar::foregroundColourId, juce::Colours::green);
    }

    void resized() override
    {
        nameLabel.setBounds(0, 0, getWidth(), 20);
        fader.setBounds(0, 20, getWidth(), getHeight() - 30);
        levelMeter.setBounds(0, getHeight() - 10, getWidth(), 10);
    }

    void setName(const juce::String& name) { nameLabel.setText(name, juce::dontSendNotification); }

    // Fix: Update the double variable directly, ProgressBar reads from it reference
    void setLevel(float level) { levelVal = static_cast<double>(level); levelMeter.repaint(); }

    juce::Slider fader;
    juce::Label nameLabel;
    double levelVal = 0.0;
    juce::ProgressBar levelMeter { levelVal };
};

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

    // Connect/Disconnect
    juce::TextButton connectButton { "Connect" };
    juce::TextButton disconnectButton { "Disconnect" };
    juce::Label connectionStatusLabel;

    // Server List
    juce::Label directoryLabel { "Directory:", "Directory:" };
    juce::ComboBox directoryBox;
    juce::TextButton fetchListButton { "Get List" };
    juce::ComboBox serverListBox;

    // Manual Address
    juce::Label serverAddressLabel { "Address:", "Address:" };
    juce::TextEditor serverAddressEditor;

    // My Settings
    juce::Label inputFaderLabel { "My Input", "My Input" };
    juce::Slider inputFader;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputFaderAttachment;

    // Mixer View
    juce::Viewport mixerViewport;
    juce::Component mixerContent;
    std::vector<std::unique_ptr<ChannelStrip>> channelStrips;

    // Logic
    std::unique_ptr<JamulusBridge> bridge;
    std::vector<CServerInfo> currentServerList;
    std::vector<CChannelInfo> currentClientList;

    void populateDirectoryBox();
    void fetchServerList();
    void updateServerListBox();
    void updateMixerLayout();
    void updateLevels(const CVector<uint16_t>& levels);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JamulusVSTAudioProcessorEditor)
};
