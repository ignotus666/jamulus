/******************************************************************************\
 * Copyright (c) 2026
 *
 * Author(s):
 *  Daryl Hanlon
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <string>
#include <queue>
#include <vector>
#include <mutex>
#include <thread>
#include <stdint.h>
#include "util.h"
#include "plugin_api.h"

class CPluginHost
{
public:
    struct LoadedPluginInfo
    {
        int id { -1 };
        std::string path;
    };

    CPluginHost();
    ~CPluginHost();

    void Init ( const int iNSampleRateHz, const int iNStereoBlockSizeSam )
    {
        iSampleRateHz.store ( iNSampleRateHz );
        iStereoBlockSizeSam.store ( iNStereoBlockSizeSam );
    }

    void Clear();

    bool HasLoadedPlugins();

    // Queue raw MIDI bytes for the next audio block. Called from audio/MIDI callback.
    // Safe for concurrent calls from MIDI thread. vst3_adapter will forward these into
    // the VST3 processor's inputEvents buffer during the Process call.
    void QueueMIDIEvent ( const uint8_t* pData, int iLength, uint32_t iSampleOffset = 0 );

    // Process in-place interleaved stereo (int16_t) buffer. Non-blocking: will try to lock the
    // plugin mutex with try_lock; if the lock cannot be acquired the processing step is skipped
    // for this buffer to preserve audio thread real-time behavior.
    void Process ( CVector<int16_t>& vecsStereoInOut, const int iBlockSizeSam );

    int  GetSampleRateHz() const { return iSampleRateHz.load(); }
    int  GetStereoBlockSizeSam() const { return iStereoBlockSizeSam.load(); }
    bool IsInitialized() const { return ( iSampleRateHz.load() > 0 ) && ( iStereoBlockSizeSam.load() > 0 ); }

    // Plugin management (non-RT functions)
    // Returns an integer id for the loaded plugin, or -1 on error.
    int LoadPlugin ( const std::string & sPath );
    bool UnloadPlugin ( int iPluginId );
    bool ShowPluginEditor ( int iPluginId, void* parentWindow );
    bool ClosePluginEditor ( int iPluginId );
    std::vector<LoadedPluginInfo> GetLoadedPluginsSnapshot();
    // Retrieve and consume MIDI events queued for the audio block (called by vst3_adapter)
    struct MidiEventData { uint8_t data[4]; int length; uint32_t offset; };
    std::vector<MidiEventData> GetAndClearMIDIEvents();
private:
    struct PluginEntry
    {
        int id{ -1 };
        void* handle{ nullptr };
        plugin_handle_t instance{ nullptr };
        plugin_create_t create{ nullptr };
        plugin_destroy_t destroy{ nullptr };
        plugin_process_t process{ nullptr };
        std::function<void ( plugin_handle_t )> closeEditor;
        std::function<bool ( plugin_handle_t, void* )> showEditor;
        std::string path;
    };

    std::atomic<int> iSampleRateHz{ 0 };
    std::atomic<int> iStereoBlockSizeSam{ 0 };

    std::mutex mtxPlugins;
    std::vector<PluginEntry> vecPlugins;
    int iNextPluginId{ 1 };

    std::mutex mtxMIDI;
    std::vector<MidiEventData> vecMIDIEvents;

    std::thread loaderThread;
    std::mutex mtxLoader;
    std::condition_variable cvLoader;
    std::queue<std::function<void()>> loaderCommands;
    bool bStopLoader{ false };
    bool bLoaderStarted{ false };

    void StartLoaderThread();
    void StopLoaderThread();
    void LoaderLoop();
    int LoadPluginImpl ( const std::string & sPath );
    bool UnloadPluginImpl ( int iPluginId );
};
