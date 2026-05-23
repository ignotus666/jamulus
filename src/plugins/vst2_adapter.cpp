#include "vst2_adapter.h"

#ifdef _WIN32

#include <windows.h>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include "vestige/aeffectx.h"

// Define VSTPluginMain signature
typedef AEffect* (VstEntryProc)(audioMasterCallback audioMaster);
// Shell VST2 opcode not in vestige header
constexpr int effShellGetNextPlugin = 70;

// Thread-local storage for the shell plugin ID that the host callback should return
static thread_local intptr_t s_shellCurrentId = 0;

struct Vst2Runtime
{
    HMODULE library = nullptr;
    AEffect* effect = nullptr;
    int sampleRate = 48000;
    int blockSize = 128;
    int numChannels = 2;
    bool bEditorVisible = false;
    HWND editorWindow = nullptr;
    QString pluginDir; // directory containing the DLL and its resources

    // Buffer management
    float** inputs = nullptr;
    float** outputs = nullptr;

    ~Vst2Runtime()
    {
        if (editorWindow)
        {
            if (effect)
                effect->dispatcher(effect, effEditClose, 0, 0, nullptr, 0.0f);
            DestroyWindow(editorWindow);
            editorWindow = nullptr;
        }
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

    static intptr_t VST_CALL_CONV HostCallback(AEffect* effect, int32_t opcode,
                                              int32_t index, intptr_t value,
                                              void* ptr, float opt)
    {
        switch (opcode)
        {
            case audioMasterVersion:
                return 2400; // VST 2.4
            case audioMasterCurrentId:
                return s_shellCurrentId;
            case audioMasterIdle:
                return 1;
            case audioMasterGetSampleRate:
                return 48000; 
            case audioMasterGetBlockSize:
                return 128;
            case audioMasterGetCurrentProcessLevel:
                return 0; // unknown
            case audioMasterGetTime:
                return 0; // null time info
            case audioMasterIOChanged:
                return 1;
            case audioMasterSizeWindow:
                // index = width, value = height
                if (effect && effect->user)
                {
                    auto* rt = static_cast<Vst2Runtime*>(effect->user);
                    if (rt->editorWindow)
                    {
                        RECT rc = {0, 0, index, (int)value};
                        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
                        SetWindowPos(rt->editorWindow, nullptr, 0, 0,
                                     rc.right - rc.left, rc.bottom - rc.top,
                                     SWP_NOMOVE | SWP_NOZORDER);
                    }
                }
                return 1;
            case audioMasterCanDo:
                if (ptr && strcmp((char*)ptr, "shellCategory") == 0) return 1;
                if (ptr && strcmp((char*)ptr, "sizeWindow") == 0) return 1;
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

    static LRESULT CALLBACK EditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return DefWindowProc(hwnd, msg, wParam, lParam);
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

        // For shell plugins (like Carla), we must enumerate sub-plugins and pick one.
        // First, try to get a shell instance to enumerate from.
        qDebug() << "vst2_adapter: enumerating shell sub-plugins...";
        s_shellCurrentId = 0;
        AEffect* probeEffect = mainProc(HostCallback);

        if (probeEffect)
        {
            // Check if this is a shell by trying to enumerate sub-plugins
            char name[256] = {0};
            intptr_t subId = probeEffect->dispatcher(probeEffect, effShellGetNextPlugin, 0, 0, name, 0.0f);

            if (subId != 0)
            {
                // It IS a shell plugin. Close this probe and re-instantiate properly.
                qDebug() << "vst2_adapter: detected shell plugin, first sub-plugin:" << name << "id:" << subId;
                intptr_t firstId = subId;

                // Log remaining sub-plugins
                while (subId != 0)
                {
                    char nextName[256] = {0};
                    subId = probeEffect->dispatcher(probeEffect, effShellGetNextPlugin, 0, 0, nextName, 0.0f);
                    if (subId != 0)
                    {
                        qDebug() << "vst2_adapter: shell sub-plugin:" << nextName << "id:" << subId;
                    }
                }

                probeEffect->dispatcher(probeEffect, effClose, 0, 0, nullptr, 0.0f);
                probeEffect = nullptr;

                // Re-instantiate with the first sub-plugin ID
                qDebug() << "vst2_adapter: instantiating sub-plugin with ID:" << firstId;
                s_shellCurrentId = firstId;
                effect = mainProc(HostCallback);
            }
            else
            {
                // Not a shell plugin — use the probe effect directly
                qDebug() << "vst2_adapter: not a shell plugin, using directly";
                effect = probeEffect;
            }
        }

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

        // Store back-pointer so HostCallback can find us
        effect->user = this;

        // Store the plugin directory for resource lookups
        pluginDir = fi.absolutePath();

        qDebug() << "vst2_adapter: plugin loaded successfully, uniqueID:" << effect->uniqueID
                 << "inputs:" << effect->numInputs << "outputs:" << effect->numOutputs
                 << "hasEditor:" << (bool)(effect->flags & effFlagsHasEditor);

        // Set DLL dir and PATH during effOpen so Carla can find its resources
        SetDllDirectoryW((LPCWSTR)pluginDir.utf16());
        // Also prepend to PATH so child processes (carla-bridge-native.exe etc.) can be found
        QString currentPath = QString::fromWCharArray(_wgetenv(L"PATH"));
        QString newPath = pluginDir + ";" + pluginDir + "/resources;" + currentPath;
        _wputenv_s(L"PATH", (LPCWSTR)newPath.utf16());

        effect->dispatcher(effect, effOpen, 0, 0, nullptr, 0.0f);
        effect->dispatcher(effect, effSetSampleRate, 0, 0, nullptr, (float)sampleRate);
        effect->dispatcher(effect, effSetBlockSize, 0, blockSize, nullptr, 0.0f);
        effect->dispatcher(effect, effMainsChanged, 0, 1, nullptr, 0.0f); // Resume (turn on)

        SetDllDirectoryW(nullptr); // restore default

        // Re-check I/O after effOpen
        qDebug() << "vst2_adapter: after effOpen - inputs:" << effect->numInputs
                 << "outputs:" << effect->numOutputs;

        inputs = new float*[numChannels];
        outputs = new float*[numChannels];
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
    auto* rt = static_cast<Vst2Runtime*>(h);
    if (!rt || !rt->effect)
        return false;

    if (!(rt->effect->flags & effFlagsHasEditor))
    {
        qWarning() << "vst2_adapter: plugin does not have an editor";
        return false;
    }

    if (rt->editorWindow)
    {
        // Already open, just bring to front
        ShowWindow(rt->editorWindow, SW_SHOW);
        SetForegroundWindow(rt->editorWindow);
        rt->bEditorVisible = true;
        return true;
    }

    // Register window class (once)
    static bool classRegistered = false;
    if (!classRegistered)
    {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = Vst2Runtime::EditorWndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"JamulusVST2Editor";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClassW(&wc);
        classRegistered = true;
    }

    // Query editor size
    struct ERect { int16_t top; int16_t left; int16_t bottom; int16_t right; };
    ERect* rect = nullptr;
    rt->effect->dispatcher(rt->effect, effEditGetRect, 0, 0, &rect, 0.0f);

    int width = 800, height = 600;
    if (rect)
    {
        width = rect->right - rect->left;
        height = rect->bottom - rect->top;
        qDebug() << "vst2_adapter: editor size:" << width << "x" << height;
    }

    // Create the host window with WS_CLIPCHILDREN so child windows render properly
    RECT rc = {0, 0, width, height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, FALSE);

    rt->editorWindow = CreateWindowW(
        L"JamulusVST2Editor",
        L"Carla",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    if (!rt->editorWindow)
    {
        qWarning() << "vst2_adapter: failed to create editor window";
        return false;
    }

    // Show parent window BEFORE effEditOpen — child windows need a visible parent
    ShowWindow(rt->editorWindow, SW_SHOW);
    UpdateWindow(rt->editorWindow);

    // Set DLL directory during effEditOpen so Carla can find its UI resources
    SetDllDirectoryW((LPCWSTR)rt->pluginDir.utf16());

    // Open editor inside our window
    rt->effect->dispatcher(rt->effect, effEditOpen, 0, 0, (void*)rt->editorWindow, 0.0f);

    SetDllDirectoryW(nullptr);

    // Ensure child windows are visible and force repaint
    HWND child = GetWindow(rt->editorWindow, GW_CHILD);
    int childCount = 0;
    while (child)
    {
        childCount++;
        ShowWindow(child, SW_SHOW);
        InvalidateRect(child, nullptr, TRUE);
        child = GetWindow(child, GW_HWNDNEXT);
    }
    qDebug() << "vst2_adapter: showed" << childCount << "child windows after effEditOpen";

    // Re-query size after open (some plugins update it)
    rect = nullptr;
    rt->effect->dispatcher(rt->effect, effEditGetRect, 0, 0, &rect, 0.0f);
    if (rect)
    {
        int newW = rect->right - rect->left;
        int newH = rect->bottom - rect->top;
        if (newW > 0 && newH > 0 && (newW != width || newH != height))
        {
            RECT rc2 = {0, 0, newW, newH};
            AdjustWindowRect(&rc2, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, FALSE);
            SetWindowPos(rt->editorWindow, nullptr, 0, 0,
                         rc2.right - rc2.left, rc2.bottom - rc2.top,
                         SWP_NOMOVE | SWP_NOZORDER);
        }
    }

    // Force full repaint
    InvalidateRect(rt->editorWindow, nullptr, TRUE);
    UpdateWindow(rt->editorWindow);
    rt->bEditorVisible = true;
    qDebug() << "vst2_adapter: editor window opened";
    return true;
}

bool vst2_close_editor_handle(plugin_handle_t h)
{
    if (auto* rt = static_cast<Vst2Runtime*>(h))
    {
        if (rt->effect && rt->editorWindow)
        {
            rt->effect->dispatcher(rt->effect, effEditClose, 0, 0, nullptr, 0.0f);
            DestroyWindow(rt->editorWindow);
            rt->editorWindow = nullptr;
        }
        rt->bEditorVisible = false;
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
            // Pump Win32 messages for the editor window and its children
            if (rt->editorWindow)
            {
                MSG msg;
                while (PeekMessage(&msg, rt->editorWindow, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                // Also pump messages for child windows
                HWND child = GetWindow(rt->editorWindow, GW_CHILD);
                while (child)
                {
                    while (PeekMessage(&msg, child, 0, 0, PM_REMOVE))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                    child = GetWindow(child, GW_HWNDNEXT);
                }
            }

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
        intptr_t size = rt->effect->dispatcher(rt->effect, effGetChunk, 0, 0, &chunk, 0.0f);
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

#endif // _WIN32
