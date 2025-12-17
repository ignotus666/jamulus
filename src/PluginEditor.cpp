#include "PluginProcessor.h"
#include "PluginEditor.h"

JamulusVSTAudioProcessorEditor::JamulusVSTAudioProcessorEditor (JamulusVSTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);

    // Connect Button
    addAndMakeVisible (connectButton);
    connectButton.onClick = [this] {
        if (auto* client = audioProcessor.getClient()) {
            // Update address from editor to processor state
            audioProcessor.setServerAddress(serverAddressEditor.getText());

            client->SetServerAddr(QString::fromStdString(serverAddressEditor.getText().toStdString()));
            client->Start();
        }
    };

    // Disconnect Button
    addAndMakeVisible (disconnectButton);
    disconnectButton.onClick = [this] {
        if (auto* client = audioProcessor.getClient()) {
            client->Stop();
        }
    };

    // Address
    addAndMakeVisible (serverAddressLabel);
    addAndMakeVisible (serverAddressEditor);
    serverAddressEditor.setText(audioProcessor.getServerAddress()); // Load from state

    // Fader
    addAndMakeVisible (inputFader);
    addAndMakeVisible (inputFaderLabel);
    inputFader.setSliderStyle (juce::Slider::LinearVertical);

    // Attach to APVTS
    inputFaderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "inputLevel", inputFader);

    addAndMakeVisible(connectionStatusLabel);
    connectionStatusLabel.setText("Disconnected", juce::dontSendNotification);

    startTimer(30);
}

JamulusVSTAudioProcessorEditor::~JamulusVSTAudioProcessorEditor()
{
}

void JamulusVSTAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void JamulusVSTAudioProcessorEditor::resized()
{
    serverAddressLabel.setBounds(10, 10, 60, 20);
    serverAddressEditor.setBounds(80, 10, 200, 20);
    connectButton.setBounds(290, 10, 80, 20);
    disconnectButton.setBounds(290, 40, 80, 20);

    inputFaderLabel.setBounds(10, 50, 60, 20);
    inputFader.setBounds(10, 70, 40, 150);

    connectionStatusLabel.setBounds(10, 230, 300, 20);
}

void JamulusVSTAudioProcessorEditor::timerCallback()
{
    // Pump Qt Events
    QCoreApplication::processEvents();

    // Update Status
    if (auto* client = audioProcessor.getClient()) {
        if (client->IsConnected()) {
            connectionStatusLabel.setText("Connected", juce::dontSendNotification);
        } else {
            connectionStatusLabel.setText("Disconnected", juce::dontSendNotification);
        }
    }
}
