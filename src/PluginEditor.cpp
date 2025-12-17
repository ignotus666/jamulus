#include "PluginProcessor.h"
#include "PluginEditor.h"

JamulusVSTAudioProcessorEditor::JamulusVSTAudioProcessorEditor (JamulusVSTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (800, 500);

    // Directory Box
    addAndMakeVisible(directoryLabel);
    addAndMakeVisible(directoryBox);
    populateDirectoryBox();

    // Fetch Button
    addAndMakeVisible(fetchListButton);
    fetchListButton.onClick = [this] { fetchServerList(); };

    // Server List Box
    addAndMakeVisible(serverListBox);
    serverListBox.onChange = [this] {
        if (serverListBox.getSelectedId() > 0) {
            int idx = serverListBox.getSelectedId() - 1;
            if (idx >= 0 && idx < currentServerList.size()) {
                serverAddressEditor.setText(currentServerList[idx].HostAddr.toString().toStdString());
            }
        }
    };

    // Manual Address
    addAndMakeVisible (serverAddressLabel);
    addAndMakeVisible (serverAddressEditor);
    serverAddressEditor.setText(audioProcessor.getServerAddress());

    // Connect/Disconnect
    addAndMakeVisible (connectButton);
    connectButton.onClick = [this] {
        if (auto* client = audioProcessor.getClient()) {
            audioProcessor.setServerAddress(serverAddressEditor.getText());
            client->SetServerAddr(QString::fromStdString(serverAddressEditor.getText().toStdString()));
            client->Start();
        }
    };

    addAndMakeVisible (disconnectButton);
    disconnectButton.onClick = [this] {
        if (auto* client = audioProcessor.getClient()) {
            client->DisconnectFromHost();

            // Force pump to send packet
            for(int i=0; i<10; ++i) QCoreApplication::processEvents();
        }
    };

    // My Fader
    addAndMakeVisible (inputFaderLabel);
    addAndMakeVisible (inputFader);
    inputFader.setSliderStyle (juce::Slider::LinearVertical);
    inputFaderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "inputLevel", inputFader);

    addAndMakeVisible(connectionStatusLabel);
    connectionStatusLabel.setText("Disconnected", juce::dontSendNotification);

    // Mixer
    addAndMakeVisible(mixerViewport);
    mixerViewport.setViewedComponent(&mixerContent, false);

    // Initialize Bridge
    if (auto* client = audioProcessor.getClient()) {
        bridge = std::make_unique<JamulusBridge>(client,
            // Server List
            [this](const CVector<CServerInfo>& list) {
                currentServerList.clear();
                for(int i=0; i<list.Size(); ++i) currentServerList.push_back(list[i]);
                updateServerListBox();
            },
            // Client List (Participants)
            [this](const CVector<CChannelInfo>& list) {
                currentClientList.clear();
                for(int i=0; i<list.Size(); ++i) currentClientList.push_back(list[i]);
                updateMixerLayout();
            },
            // Levels
            [this](const CVector<uint16_t>& levels) {
                updateLevels(levels);
            }
        );
    }

    startTimer(30);
}

JamulusVSTAudioProcessorEditor::~JamulusVSTAudioProcessorEditor()
{
    bridge.reset();
}

void JamulusVSTAudioProcessorEditor::populateDirectoryBox()
{
    // See src/global.h EDirectoryType
    directoryBox.addItem("Any Genre 1", 1); // AT_DEFAULT = 0
    directoryBox.addItem("Any Genre 2", 2); // AT_ANY_GENRE2 = 1
    directoryBox.addItem("Any Genre 3", 3);
    directoryBox.addItem("Rock", 4);
    directoryBox.addItem("Jazz", 5);
    directoryBox.addItem("Classical/Folk", 6);
    directoryBox.addItem("Choral", 7);
    directoryBox.setSelectedId(1);
}

void JamulusVSTAudioProcessorEditor::fetchServerList()
{
    if (auto* client = audioProcessor.getClient()) {
        // ID 1 is AT_DEFAULT (0). ID 8 is AT_CUSTOM (7).
        int id = directoryBox.getSelectedId();
        EDirectoryType type = static_cast<EDirectoryType>(id - 1);

        QString dirAddr = NetworkUtil::GetDirectoryAddress(type, "");

        // Debug
        // std::cout << "Fetching from: " << dirAddr.toStdString() << std::endl;

        CHostAddress hostAddr;
        if (NetworkUtil::ParseNetworkAddress(dirAddr, hostAddr, false)) {
             client->CreateCLReqServerListMes(hostAddr);
        }
    }
}

void JamulusVSTAudioProcessorEditor::updateServerListBox()
{
    serverListBox.clear();
    for (int i = 0; i < currentServerList.size(); ++i) {
        juce::String label = juce::String(currentServerList[i].strName.toStdString()) + " [" +
                             juce::String(currentServerList[i].strCity.toStdString()) + "]";
        serverListBox.addItem(label, i + 1);
    }
}

void JamulusVSTAudioProcessorEditor::updateMixerLayout()
{
    channelStrips.clear();
    mixerContent.removeAllChildren();

    int x = 0;
    int w = 60;

    for (const auto& client : currentClientList) {
        auto strip = std::make_unique<ChannelStrip>();
        strip->setBounds(x, 0, w, 200);
        strip->setName(client.strName.toStdString());

        mixerContent.addAndMakeVisible(strip.get());
        channelStrips.push_back(std::move(strip));
        x += w + 5;
    }

    mixerContent.setBounds(0, 0, x, 200);
    resized(); // refresh viewport
}

void JamulusVSTAudioProcessorEditor::updateLevels(const CVector<uint16_t>& levels)
{
    // Map levels to channel strips
    for (int i = 0; i < levels.Size(); ++i) {
        if (i < channelStrips.size()) {
            // Level is roughly 0..??? Jamulus uses uint16.
            // Need to calibrate. Assuming 0-32768 roughly?
            // Actually CClient uses levels for LED meter.
            float norm = static_cast<float>(levels[i]) / 5000.0f; // Guesswork calibration
            if (norm > 1.0f) norm = 1.0f;
            channelStrips[i]->setLevel(norm);
        }
    }
}

void JamulusVSTAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void JamulusVSTAudioProcessorEditor::resized()
{
    int margin = 10;
    int y = margin;

    // Header
    directoryLabel.setBounds(margin, y, 80, 24);
    directoryBox.setBounds(margin + 80, y, 200, 24);
    fetchListButton.setBounds(margin + 290, y, 80, 24);
    y += 30;

    serverListBox.setBounds(margin, y, 380, 24);
    y += 30;

    serverAddressLabel.setBounds(margin, y, 80, 24);
    serverAddressEditor.setBounds(margin + 80, y, 200, 24);
    connectButton.setBounds(margin + 290, y, 80, 24);
    disconnectButton.setBounds(margin + 380, y, 80, 24);
    y += 40;

    // Main Area
    // Left: My Settings
    inputFaderLabel.setBounds(margin, y, 60, 20);
    inputFader.setBounds(margin, y + 20, 60, 200);

    // Right: Mixer
    mixerViewport.setBounds(margin + 80, y, getWidth() - (margin + 90), 220);

    // Bottom
    connectionStatusLabel.setBounds(margin, getHeight() - 30, getWidth() - 2*margin, 24);
}

void JamulusVSTAudioProcessorEditor::timerCallback()
{
    QCoreApplication::processEvents();

    if (auto* client = audioProcessor.getClient()) {
        if (client->IsConnected()) {
            juce::String addr = client->GetServerAddress().toStdString();
            connectionStatusLabel.setText("Connected: " + addr, juce::dontSendNotification);
        } else {
            connectionStatusLabel.setText("Disconnected", juce::dontSendNotification);
        }
    }
}
