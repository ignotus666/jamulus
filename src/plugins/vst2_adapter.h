#pragma once

#include "plugin_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create a wrapped plugin instance from a VST2 DLL path. Returns
// nullptr on failure. Implemented only when Q_OS_WIN is defined.
plugin_handle_t vst2_create_from_path ( const char* pluginPath, int sampleRate, int blockSize, int numChannels );

// Destroy a wrapped instance created by vst2_create_from_path.
void vst2_destroy_handle ( plugin_handle_t h );

// Process interleaved float audio for the wrapped instance.
void vst2_process_handle ( plugin_handle_t handle, float* interleaved_buffer, int num_frames, int num_channels, const void* midi_events = nullptr, int num_midi_events = 0 );

// Show the plugin's external UI window.
bool vst2_show_editor_handle ( plugin_handle_t h );

// Hide/close the plugin's external UI window.
bool vst2_close_editor_handle ( plugin_handle_t h );

// Call the plugin UI's idle function. Must be called periodically (~30Hz)
// while the editor is visible. Returns true if the UI is still open.
bool vst2_idle_editor_handle ( plugin_handle_t h );

// Load a preset file (.carxp or .fxb etc)
bool vst2_load_preset_handle(plugin_handle_t inst, const char* presetPath);

// Save plugin state to a newly allocated char array. Caller must free it with free().
char* vst2_save_state_handle(plugin_handle_t inst, int* out_size);

// Restore plugin state from a previously saved char array.
bool vst2_restore_state_handle(plugin_handle_t inst, const char* data, int size);

bool vst2_is_editor_visible_handle(plugin_handle_t inst);

#ifdef __cplusplus
}
#endif
