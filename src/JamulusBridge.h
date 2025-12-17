#pragma once

#include <QObject>
#include <QCoreApplication>
#include "client.h"
#include "global.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <iostream>

class JamulusBridge : public QObject
{
    Q_OBJECT

public:
    JamulusBridge(CClient* client,
                  std::function<void(const CVector<CServerInfo>&)> onServerList,
                  std::function<void(const CVector<CChannelInfo>&)> onClientList,
                  std::function<void(const CVector<uint16_t>&)> onLevels)
        : pClient(client),
          serverListCb(onServerList),
          clientListCb(onClientList),
          levelsCb(onLevels)
    {
        if (pClient)
        {
            // Server List (Directory)
            connect(pClient, &CClient::CLServerListReceived, this, &JamulusBridge::OnCLServerListReceived);

            // Client List (Connected participants)
            connect(pClient, &CClient::ConClientListMesReceived, this, &JamulusBridge::OnConClientListMesReceived);

            // Levels
            connect(pClient, &CClient::CLChannelLevelListReceived, this, &JamulusBridge::OnCLChannelLevelListReceived);

            std::cout << "[JamulusBridge] Connected to CClient signals." << std::endl;
        }
    }

public slots:
    void OnCLServerListReceived(CHostAddress InetAddr, CVector<CServerInfo> vecServerInfo)
    {
        std::cout << "[JamulusBridge] OnCLServerListReceived: " << vecServerInfo.Size() << " servers." << std::endl;
        if (serverListCb) serverListCb(vecServerInfo);
    }

    void OnConClientListMesReceived(CVector<CChannelInfo> vecChanInfo)
    {
        std::cout << "[JamulusBridge] OnConClientListMesReceived: " << vecChanInfo.Size() << " clients." << std::endl;
        if (clientListCb) clientListCb(vecChanInfo);
    }

    void OnCLChannelLevelListReceived(CHostAddress InetAddr, CVector<uint16_t> vecLevelList)
    {
        // Level updates are frequent, uncomment if needed
        // std::cout << "[JamulusBridge] Levels received." << std::endl;
        if (levelsCb) levelsCb(vecLevelList);
    }

private:
    CClient* pClient;
    std::function<void(const CVector<CServerInfo>&)> serverListCb;
    std::function<void(const CVector<CChannelInfo>&)> clientListCb;
    std::function<void(const CVector<uint16_t>&)> levelsCb;
};
