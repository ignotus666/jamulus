/*
 * Minimal C ABI for hostable audio plugins.
 * Plugins implementing this API should export the three symbols below:
 *   - plugin_create(int sampleRate, int blockSize, int channels) -> void*
 *   - plugin_destroy(void* instance)
 *   - plugin_process(void* instance, float* interleaved, int numFrames, int numChannels)
 *
 * This header is intentionally simple so it can be implemented by small example plugins
 * and later adapted to wrap VST3/LV2 instances in a compatibility layer.
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* plugin_handle_t;

typedef plugin_handle_t (*plugin_create_t)(int sampleRate, int blockSize, int numChannels);
typedef void (*plugin_destroy_t)(plugin_handle_t);
typedef void (*plugin_process_t)(plugin_handle_t, float* interleaved, int numFrames, int numChannels, const void* midi_events, int num_midi_events);
typedef bool (*plugin_is_editor_visible_t)(plugin_handle_t);
typedef bool (*plugin_load_preset_t)(plugin_handle_t, const char* presetPath);
typedef char* (*plugin_save_state_t)(plugin_handle_t, int* out_size);
typedef bool (*plugin_restore_state_t)(plugin_handle_t, const char* data, int size);

// End of plugin API definitions

#ifdef __cplusplus
}
#endif
