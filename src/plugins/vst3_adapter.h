/* vst3_adapter.h
 * Adapter layer to instantiate/process VST3 plugins and present them via the
 * minimal C ABI used by the plugin host. This file builds even when the
 * Steinberg VST3 SDK is not available; in that case functions return failure
 * or no-op and log a warning. Define HAVE_VST3 to implement real VST3 support.
 */

#pragma once

#include <vector>
#include "plugin_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create a wrapped plugin instance from a VST3 plugin bundle path. Returns
// nullptr on failure. Implemented only when HAVE_VST3 is defined.
plugin_handle_t vst3_create_from_path ( const char* sPath, int sampleRate, int blockSize, int numChannels );

// Destroy a wrapped instance created by vst3_create_from_path.
void vst3_destroy_handle ( plugin_handle_t h );

// Process interleaved float audio for the wrapped instance.
void vst3_process_handle ( plugin_handle_t h, float* interleaved, int numFrames, int numChannels );

// Show plugin editor UI by attaching to a native parent window handle.
// On Linux this expects an X11 Window ID pointer value.
bool vst3_show_editor_handle ( plugin_handle_t h, void* parentWindow );

// Detach the plugin editor from its native parent window, if attached.
bool vst3_close_editor_handle ( plugin_handle_t h );

// Set MIDI events for the current audio frame processing (internal use by plugin host)
void vst3_set_midi_events ( const std::vector<std::pair<uint8_t, std::vector<uint8_t>>>& midiEvents );

#ifdef __cplusplus
}
#endif
