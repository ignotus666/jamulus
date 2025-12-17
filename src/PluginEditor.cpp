#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>

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
            std::cout << "[Editor] Connect clicked." << std::endl;
            audioProcessor.setServerAddress(serverAddressEditor.getText());
            client->SetServerAddr(QString::fromStdString(serverAddressEditor.getText().toStdString()));
            client->Start();
        }
    };

    addAndMakeVisible (disconnectButton);
    disconnectButton.onClick = [this] {
        if (auto* client = audioProcessor.getClient()) {
            std::cout << "[Editor] Disconnect clicked." << std::endl;
            client->DisconnectFromHost();
            // Manually force waiting loop
            for(int i=0; i<50; ++i) QCoreApplication::processEvents();
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
                std::cout << "[Editor] Received Server List Callback. Items: " << list.Size() << std::endl;
                currentServerList.clear();
                for(int i=0; i<list.Size(); ++i) currentServerList.push_back(list[i]);
                updateServerListBox();
            },
            // Client List (Participants)
            [this](const CVector<CChannelInfo>& list) {
                std::cout << "[Editor] Received Client List Callback. Items: " << list.Size() << std::endl;
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
        int id = directoryBox.getSelectedId();
        EDirectoryType type = static_cast<EDirectoryType>(id - 1);

        QString dirAddr = NetworkUtil::GetDirectoryAddress(type, "");
        std::cout << "[Editor] Fetching server list from: " << dirAddr.toStdString() << std::endl;

        CHostAddress hostAddr;
        // Use SrvDiscovery for directory lookups
        if (NetworkUtil::ParseNetworkAddressWithSrvDiscovery(dirAddr, hostAddr, false)) {
             std::cout << "[Editor] Resolved to: " << hostAddr.toString().toStdString() << ". Sending Req." << std::endl;
             client->CreateCLReqServerListMes(hostAddr);
        } else {
             std::cout << "[Editor] Failed to resolve address." << std::endl;
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
    serverListBox.setTextWhenNoChoicesAvailable("Found " + juce::String(currentServerList.size()) + " servers");
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
    // Pump Qt Events
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
