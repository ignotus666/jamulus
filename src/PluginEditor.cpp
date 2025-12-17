#include "PluginProcessor.h"
#include "PluginEditor.h"

JamulusVSTAudioProcessorEditor::JamulusVSTAudioProcessorEditor (JamulusVSTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (500, 400); // Increased size

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
            // ID is index + 1
            int idx = serverListBox.getSelectedId() - 1;
            if (idx >= 0 && idx < currentServerList.size()) {
                serverAddressEditor.setText(currentServerList[idx].HostAddr.toString());
            }
        }
    };

    // Manual Address
    addAndMakeVisible (serverAddressLabel);
    addAndMakeVisible (serverAddressEditor);
    serverAddressEditor.setText(audioProcessor.getServerAddress());

    // Connect Button
    addAndMakeVisible (connectButton);
    connectButton.onClick = [this] {
        if (auto* client = audioProcessor.getClient()) {
            audioProcessor.setServerAddress(serverAddressEditor.getText());
            client->SetServerAddr(QString::fromStdString(serverAddressEditor.getText().toStdString()));
            client->Start();
        }
    };

    // Disconnect Button
    addAndMakeVisible (disconnectButton);
    disconnectButton.onClick = [this] {
        if (auto* client = audioProcessor.getClient()) {
            // Explicitly disconnect logic
            // 1. Send Disconnect Message manually to ensure it goes out
            // client->getConnLessProtocol()->CreateCLDisconnection(client->Channel.GetAddress()); // Access issue?
            // client->CreateCLDisconnection() is not public in CClient?
            // client->Stop() handles it, but let's make sure we process events AFTER stop.

            client->Stop();

            // Pump event loop a bit to flush the packet
            for(int i=0; i<5; ++i) QCoreApplication::processEvents();
        }
    };

    // Fader
    addAndMakeVisible (inputFaderLabel);
    addAndMakeVisible (inputFader);
    inputFader.setSliderStyle (juce::Slider::LinearVertical);
    inputFaderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "inputLevel", inputFader);

    addAndMakeVisible(connectionStatusLabel);
    connectionStatusLabel.setText("Disconnected", juce::dontSendNotification);

    // Initialize Listener
    if (auto* client = audioProcessor.getClient()) {
        serverListListener = std::make_unique<ServerListListener>(client,
            [this](const CVector<CServerInfo>& list) {
                // On Main Thread
                currentServerList.clear();
                for(int i=0; i<list.Size(); ++i) {
                    currentServerList.push_back(list[i]);
                }
                updateServerListBox();
            });
    }

    startTimer(30);
}

JamulusVSTAudioProcessorEditor::~JamulusVSTAudioProcessorEditor()
{
    // Ensure listener is deleted before client if possible, though client is in processor
    serverListListener.reset();
}

void JamulusVSTAudioProcessorEditor::populateDirectoryBox()
{
    directoryBox.addItem("Any Genre 1 (Default)", 1);
    directoryBox.addItem("Any Genre 2", 2);
    directoryBox.addItem("Any Genre 3", 3);
    directoryBox.addItem("Rock", 4);
    directoryBox.addItem("Jazz", 5);
    directoryBox.addItem("Classical/Folk", 6);
    directoryBox.addItem("Choral", 7);
    directoryBox.addItem("Custom", 8);
    directoryBox.setSelectedId(1);
}

void JamulusVSTAudioProcessorEditor::fetchServerList()
{
    if (auto* client = audioProcessor.getClient()) {
        // Determine Directory Address
        QString dirAddr = NetworkUtil::GetDirectoryAddress(
            static_cast<EDirectoryType>(directoryBox.getSelectedId() - 1),
            "" // Custom address not yet supported in UI
        );

        // Resolve address
        CHostAddress hostAddr;
        if (NetworkUtil::ParseNetworkAddress(dirAddr, hostAddr, false)) { // IPv4 for now
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

void JamulusVSTAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void JamulusVSTAudioProcessorEditor::resized()
{
    int margin = 10;
    int y = margin;

    // Directory Row
    directoryLabel.setBounds(margin, y, 80, 24);
    directoryBox.setBounds(margin + 80, y, 200, 24);
    fetchListButton.setBounds(margin + 290, y, 80, 24);
    y += 30;

    // Server List Row
    serverListBox.setBounds(margin, y, 380, 24);
    y += 30;

    // Manual Address Row
    serverAddressLabel.setBounds(margin, y, 80, 24);
    serverAddressEditor.setBounds(margin + 80, y, 200, 24);
    connectButton.setBounds(margin + 290, y, 80, 24);
    y += 30;

    disconnectButton.setBounds(margin + 290, y, 80, 24);

    // Fader Area
    y += 10;
    inputFaderLabel.setBounds(margin, y, 60, 20);
    inputFader.setBounds(margin, y + 20, 60, 200);

    // Status
    connectionStatusLabel.setBounds(margin + 80, y + 20, 300, 24);
}

void JamulusVSTAudioProcessorEditor::timerCallback()
{
    // Pump Qt Events
    QCoreApplication::processEvents();

    // Update Status
    if (auto* client = audioProcessor.getClient()) {
        if (client->IsConnected()) {
            connectionStatusLabel.setText("Connected: " + juce::String(client->Channel.GetAddress().toString().toStdString()), juce::dontSendNotification);
        } else {
            connectionStatusLabel.setText("Disconnected", juce::dontSendNotification);
        }
    }
}
