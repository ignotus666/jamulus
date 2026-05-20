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

#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <dlfcn.h>
#include <future>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <queue>
#include <thread>

#include "vst3_adapter.h"

namespace
{
static std::string TrimToVst3Binary ( const std::string & sPath )
{
	QFileInfo info ( QString::fromStdString ( sPath ) );
	if ( info.isFile() && info.suffix().compare ( QLatin1String ( "so" ), Qt::CaseInsensitive ) == 0 )
		return sPath;

	if ( ! info.isDir() )
		return {};

	QDirIterator it ( QString::fromStdString ( sPath ), QStringList() << "*.so", QDir::Files,
	                  QDirIterator::Subdirectories );
	while ( it.hasNext() )
	{
		const QString candidate = it.next();
		if ( candidate.contains ( QLatin1String ( "/Contents/" ) ) )
			return candidate.toStdString();
	}

	return {};
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
	std::lock_guard<std::mutex> lg ( mtxPlugins );
	for ( auto & e : vecPlugins )
	{
		if ( e.closeEditor && e.instance )
			e.closeEditor ( e.instance );
		if ( e.destroy && e.instance )
			e.destroy ( e.instance );
		if ( e.handle )
			dlclose ( e.handle );
		e = PluginEntry();
	}
	vecPlugins.clear();
}

bool CPluginHost::HasLoadedPlugins()
{
	std::lock_guard<std::mutex> lg ( mtxPlugins );
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
	std::lock_guard<std::mutex> lg ( mtxPlugins );
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
	std::lock_guard<std::mutex> lg ( mtxPlugins );
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
				std::lock_guard<std::mutex> lg2 ( mtxPlugins );
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
	std::lock_guard<std::mutex> lg ( mtxPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
							 [iPluginId] ( const PluginEntry& e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() || !it->getEditorSize || !it->instance )
		return false;

	return it->getEditorSize ( it->instance, width, height );
}

bool CPluginHost::ResizePluginEditor ( int iPluginId, int width, int height )
{
	std::lock_guard<std::mutex> lg ( mtxPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
							 [iPluginId] ( const PluginEntry& e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() || !it->resizeEditor || !it->instance )
		return false;

	return it->resizeEditor ( it->instance, width, height );
}

bool CPluginHost::ResizePluginEditorFromPlugin ( int iPluginId, int width, int height )
{
	std::lock_guard<std::mutex> lg ( mtxPlugins );
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
	std::lock_guard<std::mutex> lg ( mtxPlugins );
	auto it = std::find_if ( vecPlugins.begin(), vecPlugins.end(),
							 [iPluginId] ( const PluginEntry& e ) { return e.id == iPluginId; } );
	if ( it == vecPlugins.end() || !it->instance )
		return false;

	auto ctx = std::make_shared<HostResizeContext>();
	ctx->callback = std::move ( callback );
	it->hostResizeContext = ctx;

	if ( it->handle == nullptr )
		return vst3_set_host_resize_callback_handle ( it->instance, ctx.get(), &CPluginHost::HostResizeTrampoline );

	return false;
}

std::vector<CPluginHost::LoadedPluginInfo> CPluginHost::GetLoadedPluginsSnapshot()
{
	std::lock_guard<std::mutex> lg ( mtxPlugins );
	std::vector<LoadedPluginInfo> result;
	result.reserve ( vecPlugins.size() );
	for ( const auto & e : vecPlugins )
	{
		LoadedPluginInfo info;
		info.id = e.id;
		info.path = e.path;
		result.push_back ( std::move ( info ) );
	}
	return result;
}

void CPluginHost::QueueMIDIEvent ( const uint8_t* pData, int iLength, uint32_t iSampleOffset )
{
	if ( !pData || iLength <= 0 || iLength > 4 )
	{
		if ( pData && iLength > 0 )
			qWarning() << "CPluginHost::QueueMIDIEvent - invalid length" << iLength;
		return;
	}

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
	void * h = dlopen ( sPath.c_str(), RTLD_NOW );
	if ( h )
	{
		auto create = (plugin_create_t)dlsym ( h, "plugin_create" );
		auto destroy = (plugin_destroy_t)dlsym ( h, "plugin_destroy" );
		auto process = (plugin_process_t)dlsym ( h, "plugin_process" );

		if ( create && destroy && process )
		{
			plugin_handle_t inst = create ( GetSampleRateHz(), GetStereoBlockSizeSam(), 2 );
			if ( ! inst )
			{
				qWarning() << "pluginhost: plugin_create failed:" << sPath.c_str();
				dlclose ( h );
				return -1;
			}

			PluginEntry e;
			{
				std::lock_guard<std::mutex> lg ( mtxPlugins );
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

	// Fall back to a VST3 bundle or direct Linux VST3 binary.
	const int iHostChannels = 2;
	const std::string binaryPath = TrimToVst3Binary ( sPath );
	const std::string loadPath = binaryPath.empty() ? sPath : binaryPath;
	plugin_handle_t inst = vst3_create_from_path ( loadPath.c_str(), GetSampleRateHz(),
	                                              GetStereoBlockSizeSam(), iHostChannels );
	if ( ! inst )
	{
		qWarning() << "pluginhost: failed to load plugin via VST3 adapter:" << sPath.c_str();
		return -1;
	}

	PluginEntry e;
	{
		std::lock_guard<std::mutex> lg ( mtxPlugins );
		e.id = iNextPluginId++;
		e.handle = nullptr;
		e.instance = inst;
		e.create = nullptr;
		e.destroy = []( plugin_handle_t x ) { vst3_destroy_handle ( x ); };
		e.process = []( plugin_handle_t x, float * buf, int nframes, int nch ) {
			vst3_process_handle ( x, buf, nframes, nch );
		};
		e.closeEditor = []( plugin_handle_t x ) { vst3_close_editor_handle ( x ); };
		e.showEditor = []( plugin_handle_t x, void* parentWindow ) {
			return vst3_show_editor_handle ( x, parentWindow );
		};
		e.getEditorSize = []( plugin_handle_t x, int& w, int& h ) {
			return vst3_get_editor_size_handle ( x, &w, &h );
		};
		e.resizeEditor = []( plugin_handle_t x, int w, int h ) {
			return vst3_resize_editor_handle ( x, w, h );
		};
		e.resizeEditorFromPlugin = []( plugin_handle_t x, int w, int h ) {
			return vst3_resize_editor_from_plugin_handle ( x, w, h );
		};
		e.path = sPath;
		vecPlugins.push_back ( e );
	}

	return e.id;
}

bool CPluginHost::UnloadPluginImpl ( int iPluginId )
{
	std::lock_guard<std::mutex> lg ( mtxPlugins );
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
	if ( it->handle )
		dlclose ( it->handle );

	vecPlugins.erase ( it );
	return true;
}

void CPluginHost::Process ( CVector<int16_t>& vecsStereoInOut, const int iBlockSizeSam )
{
	const int iFrames = std::min ( iBlockSizeSam, static_cast<int> ( vecsStereoInOut.size() / 2 ) );
	if ( iFrames <= 0 )
		return;

	if ( ! mtxPlugins.try_lock() )
		return;

	constexpr int iChannels = 2;
	std::vector<float> buffer ( iFrames * iChannels );

	for ( int iFrame = 0; iFrame < iFrames; ++iFrame )
	{
		const int iIndex = iFrame * iChannels;
		buffer[iIndex] = static_cast<float> ( vecsStereoInOut[iIndex] ) / 32768.0f;
		buffer[iIndex + 1] = static_cast<float> ( vecsStereoInOut[iIndex + 1] ) / 32768.0f;
	}

	// Convert queued MIDI events for VST3 processing and set them on adapter
	{
		std::lock_guard<std::mutex> lockMIDI ( mtxMIDI );
		std::vector<std::pair<uint8_t, std::vector<uint8_t>>> midiEventsForAdapter;
		for ( const auto& evt : vecMIDIEvents )
		{
			std::vector<uint8_t> data ( evt.data, evt.data + evt.length );
			uint8_t sampleOffset = static_cast<uint8_t>( evt.offset % iFrames );  // clamp to frame size
			if ( !data.empty() )
				qDebug() << "CPluginHost::Process: queuing MIDI" << data[0] << "at offset" << sampleOffset << "length" << evt.length;
			midiEventsForAdapter.push_back ( { sampleOffset, data } );
		}
		if ( !midiEventsForAdapter.empty() )
		{
			qDebug() << "CPluginHost::Process: setting" << midiEventsForAdapter.size() << "MIDI events for frame";
			vst3_set_midi_events ( midiEventsForAdapter );
		}
		vecMIDIEvents.clear();
	}

	for ( const auto & e : vecPlugins )
	{
		if ( e.process && e.instance )
			e.process ( e.instance, buffer.data(), iFrames, iChannels );
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

	mtxPlugins.unlock();
}
