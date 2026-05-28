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

#include "pluginhost.h"

#ifdef HAVE_CARLA
#include "carla_adapter.h"
#endif

#include <algorithm>
#include <condition_variable>
#include <cstring>
#ifndef Q_OS_WIN
#include <dlfcn.h>
#endif
#include <future>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <queue>
#include <thread>

#ifdef HAVE_LV2
#include "lv2_adapter.h"
#endif

#include "audioequalizer.h"

namespace
{
static bool IsLv2Uri ( const std::string & sPath )
{
	return sPath.find ( "http://" ) == 0 || sPath.find ( "https://" ) == 0;
}
} // namespace

CPluginHost::CPluginHost()
{
	StartLoaderThread();
}

CPluginHost::~CPluginHost()
{
	StopLoaderThread();
	Clear();
}

void CPluginHost::Init ( const int iNSampleRateHz, const int iNBlockSizeFrames )
{
	int iOldSampleRate = iSampleRateHz.exchange ( iNSampleRateHz );
	int iOldBlockSize  = iBlockSizeFrames.exchange ( iNBlockSizeFrames );
#ifdef HAVE_CARLA
	if ( ! carlaAdapterHandle || iNSampleRateHz != iOldSampleRate || iNBlockSizeFrames != iOldBlockSize )
	{
		if ( carlaAdapterHandle )
		{
			carla_adapter_shutdown ( carlaAdapterHandle );
			carlaAdapterHandle = nullptr;
		}
		carlaAdapterHandle = carla_adapter_init ( iNSampleRateHz, iNBlockSizeFrames, 2 );
	}
#endif
}

void CPluginHost::StartLoaderThread()
{
	std::lock_guard<std::mutex> lock ( mtxLoader );
	if ( bLoaderStarted )
		return;

	bStopLoader = false;
	bLoaderStarted = true;
	loaderThread = std::thread ( [this] { LoaderLoop(); } );
}

void CPluginHost::StopLoaderThread()
{
	{
		std::lock_guard<std::mutex> lock ( mtxLoader );
		if ( ! bLoaderStarted )
			return;
		bStopLoader = true;
	}

	cvLoader.notify_all();
	if ( loaderThread.joinable() )
		loaderThread.join();

	std::lock_guard<std::mutex> lock ( mtxLoader );
	bLoaderStarted = false;
	bStopLoader = false;
}

void CPluginHost::LoaderLoop()
{
	for (;;)
	{
		std::function<void()> command;
		{
			std::unique_lock<std::mutex> lock ( mtxLoader );
			cvLoader.wait ( lock, [this] { return bStopLoader || ! loaderCommands.empty(); } );
			if ( bStopLoader && loaderCommands.empty() )
				return;
			command = std::move ( loaderCommands.front() );
			loaderCommands.pop();
		}

		if ( command )
			command();
	}
}

void CPluginHost::Clear()
{
	QWriteLocker lg ( &rwLockPlugins );
	for ( auto & e : vecPlugins )
	{
		if ( e.closeEditor && e.instance )
			e.closeEditor ( e.instance );
		if ( e.destroy && e.instance )
			e.destroy ( e.instance );
#ifndef Q_OS_WIN
		if ( e.handle )
			dlclose ( e.handle );
#endif
		e = PluginEntry();
	}
	vecPlugins.clear();

#ifdef HAVE_CARLA
	if ( carlaAdapterHandle )
	{
		carla_adapter_shutdown ( carlaAdapterHandle );
		carlaAdapterHandle = nullptr;
	}
#endif
}

bool CPluginHost::HasLoadedPlugins()
{
	QWriteLocker lg ( &rwLockPlugins );
#ifdef HAVE_CARLA
	if ( carlaAdapterHandle && carla_adapter_get_plugin_count ( carlaAdapterHandle ) > 0 )
		return true;
#endif
	return !vecPlugins.empty();
}

int CPluginHost::LoadPlugin ( const std::string & sPath )
{
	StartLoaderThread();
	auto promise = std::make_shared<std::promise<int>>();
	auto future = promise->get_future();

	{
		std::lock_guard<std::mutex> lock ( mtxLoader );
		loaderCommands.push ( [this, sPath, promise] { promise->set_value ( LoadPluginImpl ( sPath ) ); } );
	}
	cvLoader.notify_one();
	return future.get();
}

bool CPluginHost::UnloadPlugin ( int iPluginId )
{
	StartLoaderThread();
	auto promise = std::make_shared<std::promise<bool>>();
	auto future = promise->get_future();

	{
		std::lock_guard<std::mutex> lock ( mtxLoader );
		loaderCommands.push ( [this, iPluginId, promise] { promise->set_value ( UnloadPluginImpl ( iPluginId ) ); } );
	}
	cvLoader.notify_one();
	return future.get();
}

bool CPluginHost::ShowPluginEditor ( int iPluginId, void* parentWindow )
{
	QWriteLocker lg ( &rwLockPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
							 [iPluginId] ( const PluginEntry& e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() )
		return false;

	if ( !it->showEditor || !it->instance )
		return false;

	return it->showEditor ( it->instance, parentWindow );
}

bool CPluginHost::ClosePluginEditor ( int iPluginId )
{
	QWriteLocker lg ( &rwLockPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
							[iPluginId] ( const PluginEntry& e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() )
		return false;

	if ( it->closeEditor && it->instance )
	{
		// Defer the actual closeEditor call to the loader thread to avoid
		// running potentially unsafe plugin teardown on the GUI thread.
		StartLoaderThread();
		std::lock_guard<std::mutex> lock ( mtxLoader );
		// capture the instance pointer; the loader thread will call the host adapter
		// close function in a safe context.
		plugin_handle_t inst = it->instance;
		loaderCommands.push ( [inst, this] {
			try {
				// find matching plugin entry by instance and call its closeEditor callback
				QWriteLocker lg2 ( &rwLockPlugins );
				auto jt = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
										 [inst] ( const PluginEntry& e ) { return e.instance == inst; } );
				if ( jt != vecPlugins.end() && jt->closeEditor )
				{
					jt->closeEditor ( jt->instance );
				}
			}
			catch ( const std::exception& ex ) {
				qWarning() << "pluginhost (loader): exception during deferred closeEditor:" << ex.what();
			}
			catch ( ... ) {
				qWarning() << "pluginhost (loader): unknown exception during deferred closeEditor";
			}
		} );
		cvLoader.notify_one();
	}

	return true;
}

bool CPluginHost::GetPluginEditorSize ( int iPluginId, int& width, int& height )
{
	QReadLocker lg ( &rwLockPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
							 [iPluginId] ( const PluginEntry& e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() || !it->getEditorSize || !it->instance )
		return false;

	return it->getEditorSize ( it->instance, width, height );
}

bool CPluginHost::ResizePluginEditor ( int iPluginId, int width, int height )
{
	QReadLocker lg ( &rwLockPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
							 [iPluginId] ( const PluginEntry& e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() || !it->resizeEditor || !it->instance )
		return false;

	return it->resizeEditor ( it->instance, width, height );
}

bool CPluginHost::ResizePluginEditorFromPlugin ( int iPluginId, int width, int height )
{
	QReadLocker lg ( &rwLockPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
							 [iPluginId] ( const PluginEntry& e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() || !it->resizeEditorFromPlugin || !it->instance )
		return false;

	return it->resizeEditorFromPlugin ( it->instance, width, height );
}

void CPluginHost::HostResizeTrampoline ( void* context, int width, int height )
{
	if ( !context )
		return;

	auto* ctx = static_cast<HostResizeContext*> ( context );
	if ( ctx->callback )
		ctx->callback ( width, height );
}

bool CPluginHost::SetPluginEditorHostResizeCallback ( int iPluginId, std::function<void ( int, int )> callback )
{
	QReadLocker lg ( &rwLockPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
							 [iPluginId] ( const PluginEntry& e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() || !it->instance )
		return false;

	auto ctx = std::make_shared<HostResizeContext>();
	ctx->callback = std::move ( callback );
	it->hostResizeContext = ctx;

	// LV2 external UI manages its own window; host resize callbacks are not
	// applicable for external-UI plugins like Carla.
	return false;
}

std::vector<CPluginHost::LoadedPluginInfo> CPluginHost::GetLoadedPluginsSnapshot()
{
	QReadLocker lg ( &rwLockPlugins );
	std::vector<LoadedPluginInfo> result;
	result.reserve ( vecPlugins.size() );
	for ( const auto & e : vecPlugins )
	{
		LoadedPluginInfo info;
		info.id = e.id;
		info.path = e.path;
		info.bEditorVisible = e.isEditorVisible ? e.isEditorVisible ( e.instance ) : false;
		result.push_back ( std::move ( info ) );
	}
	return result;
}

bool CPluginHost::LoadPluginPreset ( int iPluginId, const std::string& presetPath )
{
	QReadLocker lg ( &rwLockPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
	                        [iPluginId] ( const PluginEntry & e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() || !it->loadPreset || !it->instance )
		return false;

	return it->loadPreset ( it->instance, presetPath.c_str() );
}

QByteArray CPluginHost::SavePluginState ( int iPluginId )
{
	QReadLocker lg ( &rwLockPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
	                        [iPluginId] ( const PluginEntry & e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() || !it->saveState || !it->instance )
		return QByteArray();

    int size = 0;
    char* data = it->saveState(it->instance, &size);
    if (!data || size <= 0)
        return QByteArray();

    QByteArray result(data, size);
    free(data);
    return result;
}

bool CPluginHost::RestorePluginState ( int iPluginId, const QByteArray& stateData )
{
	QReadLocker lg ( &rwLockPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
	                        [iPluginId] ( const PluginEntry & e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() || !it->restoreState || !it->instance )
		return false;

	return it->restoreState ( it->instance, stateData.constData(), stateData.size() );
}

void CPluginHost::IdlePluginEditors()
{
#ifdef HAVE_CARLA
	if ( carlaAdapterHandle )
	{
		carla_adapter_idle ( carlaAdapterHandle );
	}
#endif
	QReadLocker lg ( &rwLockPlugins );
	for ( auto& p : vecPlugins )
	{
		if ( p.idleEditor )
		{
			if ( !p.idleEditor ( p.instance ) )
			{
				p.closeEditor ( p.instance );
			}
		}
	}
}

void CPluginHost::QueueMIDIEvent ( const uint8_t* pData, int iLength, uint32_t iSampleOffset )
{
	if ( !pData || iLength <= 0 || iLength > 4 )
	{
		if ( pData && iLength > 0 )
			qWarning() << "CPluginHost::QueueMIDIEvent - invalid length" << iLength;
		return;
	}

	// Only forward channel voice messages to instrument plugins.
	// Realtime/system messages (clock, active sensing, sysex, etc.)
	// can arrive at very high rates from hardware controllers and
	// cause unnecessary processing load in the audio callback.
	const uint8_t iStatusByte = pData[0];
	if ( iStatusByte < 0x80 || iStatusByte >= 0xF0 )
		return;

	std::lock_guard<std::mutex> lock ( mtxMIDI );
	MidiEventData evt;
	evt.length = iLength;
	evt.offset = iSampleOffset;
	std::memcpy ( evt.data, pData, iLength );
	vecMIDIEvents.push_back ( evt );
}

std::vector<CPluginHost::MidiEventData> CPluginHost::GetAndClearMIDIEvents()
{
	std::lock_guard<std::mutex> lock ( mtxMIDI );
	auto result = std::move ( vecMIDIEvents );
	vecMIDIEvents.clear();
	return result;
}

int CPluginHost::LoadPluginImpl ( const std::string & sPath )
{
	// First try a plain C-ABI plugin shared library.
#ifndef Q_OS_WIN
	void * h = dlopen ( sPath.c_str(), RTLD_NOW );
	if ( h )
	{
		auto create = (plugin_create_t)dlsym ( h, "plugin_create" );
		auto destroy = (plugin_destroy_t)dlsym ( h, "plugin_destroy" );
		auto process = (plugin_process_t)dlsym ( h, "plugin_process" );

		if ( create && destroy && process )
		{
			plugin_handle_t inst = create ( GetSampleRateHz(), GetBlockSizeFrames(), 2 );
			if ( ! inst )
			{
				qWarning() << "pluginhost: plugin_create failed:" << sPath.c_str();
				dlclose ( h );
				return -1;
			}

			PluginEntry e;
			{
				QWriteLocker lg ( &rwLockPlugins );
				e.id = iNextPluginId++;
				e.handle = h;
				e.instance = inst;
				e.create = create;
				e.destroy = destroy;
				e.process = process;
					e.closeEditor = nullptr;
				e.path = sPath;
				e.showEditor = nullptr;
				vecPlugins.push_back ( e );
			}

			return e.id;
		}

		dlclose ( h );
	}
#endif

#ifdef HAVE_LV2
	// Fall back to LV2 plugin loading (by URI).
	if ( ! IsLv2Uri ( sPath ) )
	{
		qWarning() << "pluginhost: path is neither a C-ABI library nor an LV2 URI:" << sPath.c_str();
		return -1;
	}

	const int iHostChannels = 2;
	plugin_handle_t inst = lv2_create_from_uri ( sPath.c_str(), GetSampleRateHz(),
	                                              GetBlockSizeFrames(), iHostChannels );
	if ( ! inst )
	{
		qWarning() << "pluginhost: failed to load plugin via LV2 adapter:" << sPath.c_str();
		return -1;
	}

	PluginEntry e;
	{
		QWriteLocker lg ( &rwLockPlugins );
		e.id = iNextPluginId++;
		e.handle = nullptr;
		e.instance = inst;
		e.create = nullptr;
		e.destroy = []( plugin_handle_t x ) { lv2_destroy_handle ( x ); };
		e.process = []( plugin_handle_t x, float * buf, int nframes, int nch, const void* midi, int nmidi ) {
			lv2_process_handle ( x, buf, nframes, nch, midi, nmidi );
		};
		e.closeEditor = []( plugin_handle_t x ) { lv2_close_editor_handle ( x ); };
		e.showEditor = []( plugin_handle_t x, void* /* parentWindow */ ) {
			// LV2 external UI manages its own window; parentWindow is unused.
			return lv2_show_editor_handle ( x );
		};
		e.idleEditor = []( plugin_handle_t x ) { return lv2_idle_editor_handle ( x ); };
		e.isEditorVisible = []( plugin_handle_t x ) { return lv2_is_editor_visible_handle ( x ); };
		e.loadPreset = []( plugin_handle_t x, const char* path ) { return lv2_load_preset_handle ( x, path ); };
		e.saveState = []( plugin_handle_t x, int* outSize ) { return lv2_save_state_handle ( x, outSize ); };
		e.restoreState = []( plugin_handle_t x, const char* data, int size ) { return lv2_restore_state_handle ( x, data, size ); };
		e.getEditorSize = nullptr;
		e.resizeEditor = nullptr;
		e.resizeEditorFromPlugin = nullptr;
		e.path = sPath;
		vecPlugins.push_back ( e );
	}

	return e.id;
#else
	qWarning() << "pluginhost: LV2 URI loading not supported in this build:" << sPath.c_str();
	return -1;
#endif
}

bool CPluginHost::UnloadPluginImpl ( int iPluginId )
{
	QWriteLocker lg ( &rwLockPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
	                        [iPluginId] ( const PluginEntry & e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() )
	{
		qDebug() << "pluginhost: UnloadPluginImpl - plugin id" << iPluginId << "not found";
		return false;
	}

	if ( it->closeEditor && it->instance )
	{
		qDebug() << "pluginhost: UnloadPluginImpl - closing editor for id" << iPluginId;
		try
		{
			it->closeEditor ( it->instance );
			qDebug() << "pluginhost: UnloadPluginImpl - editor closed successfully";
		}
		catch ( const std::exception& e )
		{
			qWarning() << "pluginhost: UnloadPluginImpl - exception during closeEditor:" << e.what();
		}
		catch ( ... )
		{
			qWarning() << "pluginhost: UnloadPluginImpl - unknown exception during closeEditor";
		}
	}

	if ( it->destroy && it->instance )
	{
		qDebug() << "pluginhost: UnloadPluginImpl - destroying instance for id" << iPluginId;
		it->destroy ( it->instance );
	}
#ifndef Q_OS_WIN
	if ( it->handle )
		dlclose ( it->handle );
#endif

	vecPlugins.erase ( it );
	return true;
}

void CPluginHost::Process ( CVector<int16_t>& vecsStereoInOut, const int iBlockSizeSam )
{
	const int iFrames = std::min ( iBlockSizeSam, static_cast<int> ( vecsStereoInOut.size() / 2 ) );
	if ( iFrames <= 0 )
		return;

	if ( ! rwLockPlugins.tryLockForRead() )
		return;

	constexpr int iChannels = 2;
	std::vector<float> buffer ( iFrames * iChannels );

	for ( int iFrame = 0; iFrame < iFrames; ++iFrame )
	{
		const int iIndex = iFrame * iChannels;
		buffer[iIndex] = static_cast<float> ( vecsStereoInOut[iIndex] ) / 32768.0f;
		buffer[iIndex + 1] = static_cast<float> ( vecsStereoInOut[iIndex + 1] ) / 32768.0f;
	}

	std::vector<MidiEventData> localMidiEvents;
	{
		std::lock_guard<std::mutex> lockMIDI ( mtxMIDI );
		localMidiEvents = std::move(vecMIDIEvents);
		vecMIDIEvents.clear();
	}

#ifdef HAVE_CARLA
	if ( carlaAdapterHandle )
	{
		carla_adapter_process ( carlaAdapterHandle, buffer.data(), iFrames, iChannels, localMidiEvents.data(), static_cast<int>(localMidiEvents.size()) );
	}
	else
#endif
	{
		for ( const auto & e : vecPlugins )
		{
			if ( e.process && e.instance )
				e.process ( e.instance, buffer.data(), iFrames, iChannels, localMidiEvents.data(), static_cast<int>(localMidiEvents.size()) );
		}
	}

	for ( int iFrame = 0; iFrame < iFrames; ++iFrame )
	{
		const int iIndex = iFrame * iChannels;
		float fL = buffer[iIndex];
		float fR = buffer[iIndex + 1];
		if ( fL > 1.0f ) fL = 1.0f;
		if ( fL < -1.0f ) fL = -1.0f;
		if ( fR > 1.0f ) fR = 1.0f;
		if ( fR < -1.0f ) fR = -1.0f;
		vecsStereoInOut[iIndex] = static_cast<int16_t> ( fL * 32767.0f );
		vecsStereoInOut[iIndex + 1] = static_cast<int16_t> ( fR * 32767.0f );
	}

	rwLockPlugins.unlock();
}
