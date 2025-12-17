#pragma once

#include <QObject>
#include <QCoreApplication>
#include "client.h"
#include "global.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ServerListListener : public QObject
{
    Q_OBJECT

public:
    ServerListListener(CClient* client, std::function<void(const CVector<CServerInfo>&)> callback)
        : pClient(client), onListReceived(callback)
    {
        if (pClient)
        {
            connect(pClient, &CClient::CLServerListReceived, this, &ServerListListener::OnCLServerListReceived);
        }
    }

public slots:
    void OnCLServerListReceived(CHostAddress InetAddr, CVector<CServerInfo> vecServerInfo)
    {
        // Forward to JUCE on Message Thread
        if (onListReceived)
        {
            // Copy data to lambda capture safely
            // CVector is compatible with std::vector logic, but let's just pass it
            // We need to be careful about thread safety. This slot is called on Qt thread (Main Thread).
            // JUCE runs on Main Thread too usually.
            onListReceived(vecServerInfo);
        }
    }

private:
    CClient* pClient;
    std::function<void(const CVector<CServerInfo>&)> onListReceived;
};
