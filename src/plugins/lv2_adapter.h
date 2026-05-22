/* lv2_adapter.h
 * Adapter layer to instantiate/process LV2 plugins and present them via the
 * minimal C ABI used by the plugin host. This file builds even when lilv is
 * not available; in that case functions return failure or no-op.
 * Define HAVE_LV2 to implement real LV2 support.
 */

#pragma once

#include "plugin_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create a wrapped plugin instance from an LV2 plugin URI. Returns
// nullptr on failure. Implemented only when HAVE_LV2 is defined.
plugin_handle_t lv2_create_from_uri ( const char* pluginURI, int sampleRate, int blockSize, int numChannels );

// Destroy a wrapped instance created by lv2_create_from_uri.
void lv2_destroy_handle ( plugin_handle_t h );

// Process interleaved float audio for the wrapped instance.
void lv2_process_handle ( plugin_handle_t handle, float* interleaved_buffer, int num_frames, int num_channels, const void* midi_events = nullptr, int num_midi_events = 0 );

// Show the plugin's external UI window (Carla manages its own window).
bool lv2_show_editor_handle ( plugin_handle_t h );

// Hide/close the plugin's external UI window.
bool lv2_close_editor_handle ( plugin_handle_t h );

// Call the plugin UI's idle function. Must be called periodically (~30Hz)
// while the editor is visible. Returns true if the UI is still open.
bool lv2_idle_editor_handle ( plugin_handle_t h );

// Returns true if the editor is currently visible.
bool lv2_is_editor_visible_handle ( plugin_handle_t h );

// Loads a preset from the given path (if supported).
bool lv2_load_preset_handle ( plugin_handle_t h, const char* presetPath );

// Save plugin state to a newly allocated char array. Caller must free it with free().
char* lv2_save_state_handle(plugin_handle_t inst, int* out_size);

// Restore plugin state from a previously saved char array.
bool lv2_restore_state_handle(plugin_handle_t inst, const char* data, int size);

// Scan for all available LV2 plugins and return their URIs + names.
// The caller owns the returned arrays and must free them.
// Returns the number of plugins found.
struct Lv2PluginInfo
{
    char uri[512];
    char name[256];
};

int lv2_scan_plugins ( struct Lv2PluginInfo** outPlugins );
void lv2_free_scan_results ( struct Lv2PluginInfo* plugins );

#ifdef __cplusplus
}
#endif
