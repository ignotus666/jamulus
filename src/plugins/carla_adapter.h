#pragma once
#include "plugin_api.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Initialize the Carla-Rack engine. Returns an opaque handle.
    // sampleRate and blockSize configure the audio engine.
    void* carla_adapter_init ( int sampleRate, int blockSize, int numChannels );

    // Shutdown and free the Carla engine.
    void carla_adapter_shutdown ( void* handle );

    // Process interleaved stereo audio through the Carla plugin chain.
    void carla_adapter_process ( void* handle, float* interleaved, int numFrames, int numChannels, const void* midiEvents, int numMidiEvents );

    // Plugin management via Carla Host API
    // Returns carla plugin ID (>=0) on success, -1 on failure.
    int carla_adapter_add_plugin ( void*       handle,
                                   int         binaryType,
                                   int         pluginType,
                                   const char* filename,
                                   const char* name,
                                   const char* label,
                                   int64_t     uniqueId );

    bool carla_adapter_remove_plugin ( void* handle, int pluginId );
    bool carla_adapter_remove_all_plugins ( void* handle );

    // UI management
    void carla_adapter_show_plugin_ui ( void* handle, int pluginId, bool show );
    void carla_adapter_idle ( void* handle );

    // Plugin enable/disable/parameters
    void carla_adapter_set_active ( void* handle, int pluginId, bool active );
    bool carla_adapter_get_active ( void* handle, int pluginId );
    void carla_adapter_set_drywet ( void* handle, int pluginId, float value );
    void carla_adapter_set_volume ( void* handle, int pluginId, float value );

    // State management
    char* carla_adapter_save_state ( void* handle ); // caller must free() with free()
    bool  carla_adapter_restore_state ( void* handle, const char* state );

    // Plugin count
    int carla_adapter_get_plugin_count ( void* handle );

    // Get info about a loaded plugin (name)
    const char* carla_adapter_get_plugin_name ( void* handle, int pluginId );

#ifdef __cplusplus
}
#endif
