#include "vst2_adapter.h"

#ifdef Q_OS_WIN

#include <windows.h>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include "vestige/aeffectx.h"

// Define VSTPluginMain signature
typedef AEffect* (VstEntryProc)(audioMasterCallback audioMaster);

struct Vst2Runtime
{
    HMODULE library = nullptr;
    AEffect* effect = nullptr;
    int sampleRate = 48000;
    int blockSize = 128;
    int numChannels = 2;
    bool bEditorVisible = false;

    // Buffer management
    float** inputs = nullptr;
    float** outputs = nullptr;

    ~Vst2Runtime()
    {
        if (effect)
        {
            effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0f);
        }
        if (library)
        {
            FreeLibrary(library);
        }
        if (inputs)
        {
            delete[] inputs[0];
            delete[] inputs;
        }
        if (outputs)
        {
            delete[] outputs[0];
            delete[] outputs;
        }
    }

    static VstIntPtr VSTCALLBACK HostCallback(AEffect* effect, VstInt32 opcode,
                                              VstInt32 index, VstIntPtr value,
                                              void* ptr, float opt)
    {
        switch (opcode)
        {
            case audioMasterVersion:
                return 2400; // VST 2.4
            case audioMasterIdle:
                return 1;
            case audioMasterGetSampleRate:
                // ideally return sampleRate, but we don't have user data here easily
                return 48000; 
            case audioMasterGetBlockSize:
                return 128;
            case audioMasterGetCurrentProcessLevel:
                return 0; // unknown
            case audioMasterGetTime:
                return 0; // null time info
            case audioMasterCanDo:
                if (ptr && strcmp((char*)ptr, "sendVstEvents") == 0) return 1;
                if (ptr && strcmp((char*)ptr, "sendVstMidiEvent") == 0) return 1;
                if (ptr && strcmp((char*)ptr, "receiveVstEvents") == 0) return 1;
                if (ptr && strcmp((char*)ptr, "receiveVstMidiEvent") == 0) return 1;
                return 0;
            default:
                break;
        }
        return 0;
    }

    bool init(const char* path)
    {
        QString wPath = QString::fromUtf8(path);

        // Add the DLL's parent directory to the search path so that
        // sibling dependencies (e.g. libcarla_utils.dll) can be found.
        QFileInfo fi(wPath);
        QString dirPath = fi.absolutePath();
        SetDllDirectoryW((LPCWSTR)dirPath.utf16());
        qDebug() << "vst2_adapter: loading" << wPath << "with DLL dir" << dirPath;

        library = LoadLibraryW((LPCWSTR)wPath.utf16());

        // Restore default DLL search path
        SetDllDirectoryW(nullptr);

        if (!library)
        {
            DWORD err = GetLastError();
            qWarning() << "vst2_adapter: failed to load library" << path << "error code:" << err;
            return false;
        }

        VstEntryProc* mainProc = (VstEntryProc*)GetProcAddress(library, "VSTPluginMain");
        if (!mainProc)
        {
            mainProc = (VstEntryProc*)GetProcAddress(library, "main");
        }

        if (!mainProc)
        {
            qWarning() << "vst2_adapter: could not find VST entry point in" << path;
            return false;
        }

        effect = mainProc(HostCallback);
        if (!effect)
        {
            qWarning() << "vst2_adapter: plugin main returned null";
            return false;
        }

        if (effect->magic != kEffectMagic)
        {
            qWarning() << "vst2_adapter: invalid plugin magic";
            return false;
        }

        effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);
        effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, (float)sampleRate);
        effect->dispatcher(effect, effSetBlockSize, 0, blockSize, nullptr, 0.0f);
        effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f); // Resume (turn on)

        inputs = new float*[numChannels];
        outputs = new float*[numChannels];
        // allocate a reasonable max block size (e.g. 2048)
        inputs[0] = new float[2048 * numChannels]();
        outputs[0] = new float[2048 * numChannels]();
        for (int i = 1; i < numChannels; i++)
        {
            inputs[i] = inputs[0] + i * 2048;
            outputs[i] = outputs[0] + i * 2048;
        }

        return true;
    }

    void process(float* interleaved, int num_frames)
    {
        if (!effect) return;

        // de-interleave
        for (int c = 0; c < numChannels; c++)
        {
            for (int i = 0; i < num_frames; i++)
            {
                inputs[c][i] = interleaved[i * numChannels + c];
            }
        }

        effect->processReplacing(effect, inputs, outputs, num_frames);

        // interleave
        for (int c = 0; c < numChannels; c++)
        {
            for (int i = 0; i < num_frames; i++)
            {
                interleaved[i * numChannels + c] = outputs[c][i];
            }
        }
    }
};

plugin_handle_t vst2_create_from_path(const char* pluginPath, int sampleRate, int blockSize, int numChannels)
{
    Vst2Runtime* rt = new Vst2Runtime();
    rt->sampleRate = sampleRate;
    rt->blockSize = blockSize;
    rt->numChannels = numChannels;

    if (!rt->init(pluginPath))
    {
        delete rt;
        return nullptr;
    }
    return rt;
}

void vst2_destroy_handle(plugin_handle_t h)
{
    if (auto* rt = static_cast<Vst2Runtime*>(h))
    {
        delete rt;
    }
}

void vst2_process_handle(plugin_handle_t handle, float* interleaved_buffer, int num_frames, int num_channels, const void* midi_events, int num_midi_events)
{
    if (auto* rt = static_cast<Vst2Runtime*>(handle))
    {
        rt->process(interleaved_buffer, num_frames);
    }
}

bool vst2_show_editor_handle(plugin_handle_t h)
{
    if (auto* rt = static_cast<Vst2Runtime*>(h))
    {
        if (rt->effect && (rt->effect->flags & effFlagsHasEditor))
        {
            rt->effect->dispatcher(rt->effect, effEditOpen, 0, 0, nullptr, 0.0f);
            rt->bEditorVisible = true;
            return true;
        }
    }
    return false;
}

bool vst2_close_editor_handle(plugin_handle_t h)
{
    if (auto* rt = static_cast<Vst2Runtime*>(h))
    {
        if (rt->effect && rt->bEditorVisible)
        {
            rt->effect->dispatcher(rt->effect, effEditClose, 0, 0, nullptr, 0.0f);
            rt->bEditorVisible = false;
        }
        return true;
    }
    return false;
}

bool vst2_idle_editor_handle(plugin_handle_t h)
{
    if (auto* rt = static_cast<Vst2Runtime*>(h))
    {
        if (rt->effect && rt->bEditorVisible)
        {
            rt->effect->dispatcher(rt->effect, effEditIdle, 0, 0, nullptr, 0.0f);
            return true;
        }
    }
    return false;
}

bool vst2_is_editor_visible_handle(plugin_handle_t h)
{
    if (auto* rt = static_cast<Vst2Runtime*>(h))
    {
        return rt->bEditorVisible;
    }
    return false;
}

bool vst2_load_preset_handle(plugin_handle_t inst, const char* presetPath)
{
    if (auto* rt = static_cast<Vst2Runtime*>(inst))
    {
        // Carla specific: we could read the file and pass it as a chunk if it's a carxp,
        // but Carla also implements effSetChunk natively.
        // Actually for Carla VST2, we might have to pass the raw XML as the chunk.
        QFile file(QString::fromUtf8(presetPath));
        if (file.open(QIODevice::ReadOnly))
        {
            QByteArray xml = file.readAll();
            file.close();
            // Carla expects null terminated string in chunk
            xml.append('\0');
            rt->effect->dispatcher(rt->effect, effSetChunk, 0, xml.size(), xml.data(), 0.0f);
            return true;
        }
    }
    return false;
}

char* vst2_save_state_handle(plugin_handle_t inst, int* out_size)
{
    if (auto* rt = static_cast<Vst2Runtime*>(inst))
    {
        void* chunk = nullptr;
        VstIntPtr size = rt->effect->dispatcher(rt->effect, effGetChunk, 0, 0, &chunk, 0.0f);
        if (size > 0 && chunk)
        {
            char* copy = (char*)malloc(size);
            memcpy(copy, chunk, size);
            if (out_size) *out_size = size;
            return copy;
        }
    }
    if (out_size) *out_size = 0;
    return nullptr;
}

bool vst2_restore_state_handle(plugin_handle_t inst, const char* data, int size)
{
    if (auto* rt = static_cast<Vst2Runtime*>(inst))
    {
        rt->effect->dispatcher(rt->effect, effSetChunk, 0, size, (void*)data, 0.0f);
        return true;
    }
    return false;
}

#else

// Dummy implementations for non-Windows platforms (e.g. Linux/macOS)
plugin_handle_t vst2_create_from_path(const char*, int, int, int) { return nullptr; }
void vst2_destroy_handle(plugin_handle_t) {}
void vst2_process_handle(plugin_handle_t, float*, int, int, const void*, int) {}
bool vst2_show_editor_handle(plugin_handle_t) { return false; }
bool vst2_close_editor_handle(plugin_handle_t) { return false; }
bool vst2_idle_editor_handle(plugin_handle_t) { return false; }
bool vst2_is_editor_visible_handle(plugin_handle_t) { return false; }
bool vst2_load_preset_handle(plugin_handle_t, const char*) { return false; }
char* vst2_save_state_handle(plugin_handle_t, int*) { return nullptr; }
bool vst2_restore_state_handle(plugin_handle_t, const char*, int) { return false; }

#endif
