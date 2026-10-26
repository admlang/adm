// The common item dialog (IFileOpenDialog, IFileSaveDialog) for std.ui's pickFolder,
// pickFiles and saveFile on Windows: shown over the window given, modal to it, from
// whatever thread calls (COM is entered for the call). Included by the native-support
// unit that std/ui/pickers_windows.adm binds; empty elsewhere.
#ifndef ADM_PICKERS_WIN32_H
#define ADM_PICKERS_WIN32_H

#ifdef _WIN32

#define COBJMACROS
#include <windows.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The last call's result: 0 chosen, 1 cancelled, 2 failed (adm_pick_error says why).
static int adm_pick_last_status = 0;
static char adm_pick_last_error[256];
// The paths chosen, joined by '\n', UTF-8; kept until the next call.
static char* adm_pick_last_paths = NULL;

static wchar_t* adm_pick_wide(const char* text) {
    if (!text) text = "";
    int n = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (n <= 0) return NULL;
    wchar_t* out = (wchar_t*)malloc((size_t)n * sizeof(wchar_t));
    if (!out) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, text, -1, out, n);
    return out;
}

static int adm_pick_append_utf8(char** buf, size_t* len, size_t* cap, const wchar_t* wide, int separator) {
    int n = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (n <= 0) return 0;
    size_t need = *len + (size_t)n + 1;
    if (need > *cap) {
        size_t grown = need * 2;
        char* bigger = (char*)realloc(*buf, grown);
        if (!bigger) return 0;
        *buf = bigger;
        *cap = grown;
    }
    if (separator && *len > 0) (*buf)[(*len)++] = '\n';
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, *buf + *len, n, NULL, NULL);
    *len += (size_t)n - 1;
    (*buf)[*len] = 0;
    return 1;
}

static void adm_pick_fail(const char* what, HRESULT hr) {
    adm_pick_last_status = 2;
    snprintf(adm_pick_last_error, sizeof adm_pick_last_error, "%s failed (0x%08lx)", what, (unsigned long)hr);
}

// Counts the filters of "Name|*.a;*.b\nName2|*.c" and fills specs (names and patterns
// as wide strings the caller frees) up to max; the patterns' ';' separators are what
// the dialog takes.
static int adm_pick_filters(const char* text, COMDLG_FILTERSPEC* specs, int max) {
    int count = 0;
    const char* line = text ? text : "";
    while (*line && count < max) {
        const char* end = strchr(line, '\n');
        size_t n = end ? (size_t)(end - line) : strlen(line);
        char* copy = (char*)malloc(n + 1);
        if (!copy) break;
        memcpy(copy, line, n);
        copy[n] = 0;
        char* bar = strchr(copy, '|');
        if (bar) {
            *bar = 0;
            specs[count].pszName = adm_pick_wide(copy);
            specs[count].pszSpec = adm_pick_wide(bar + 1);
            if (specs[count].pszName && specs[count].pszSpec) count++;
        }
        free(copy);
        line = end ? end + 1 : line + n;
    }
    return count;
}

// Shows the dialog: kind 0 a folder, 1 files (several when multiple), 2 a save. Returns
// the chosen paths joined by '\n' ("" when cancelled or failed; adm_pick_status tells
// which).
static const char* adm_pick_paths(uint64_t hwnd, int kind, const char* title, const char* start, const char* name, const char* filters, int multiple) {
    adm_pick_last_status = 0;
    adm_pick_last_error[0] = 0;
    if (adm_pick_last_paths) adm_pick_last_paths[0] = 0;
    HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    int entered = SUCCEEDED(init) || init == RPC_E_CHANGED_MODE;
    IFileDialog* dialog = NULL;
    HRESULT hr;
    if (kind == 2) {
        hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileSaveDialog, (void**)&dialog);
    } else {
        hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog, (void**)&dialog);
    }
    if (FAILED(hr) || !dialog) {
        adm_pick_fail("CoCreateInstance", hr);
        if (entered && init != RPC_E_CHANGED_MODE) CoUninitialize();
        return "";
    }
    FILEOPENDIALOGOPTIONS options = 0;
    IFileDialog_GetOptions(dialog, &options);
    options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;
    if (kind == 0) options |= FOS_PICKFOLDERS;
    if (kind == 1) options |= FOS_FILEMUSTEXIST;
    if (kind == 1 && multiple) options |= FOS_ALLOWMULTISELECT;
    if (kind == 2) options |= FOS_OVERWRITEPROMPT;
    IFileDialog_SetOptions(dialog, options);
    wchar_t* wtitle = adm_pick_wide(title);
    if (wtitle && wtitle[0]) IFileDialog_SetTitle(dialog, wtitle);
    free(wtitle);
    if (start && start[0]) {
        wchar_t* wstart = adm_pick_wide(start);
        IShellItem* folder = NULL;
        if (wstart && SUCCEEDED(SHCreateItemFromParsingName(wstart, NULL, &IID_IShellItem, (void**)&folder)) && folder) {
            IFileDialog_SetFolder(dialog, folder);
            IShellItem_Release(folder);
        }
        free(wstart);
    }
    if (kind == 2 && name && name[0]) {
        wchar_t* wname = adm_pick_wide(name);
        if (wname) IFileDialog_SetFileName(dialog, wname);
        free(wname);
    }
    COMDLG_FILTERSPEC specs[32];
    int filterCount = kind == 0 ? 0 : adm_pick_filters(filters, specs, 32);
    if (filterCount > 0) {
        IFileDialog_SetFileTypes(dialog, (UINT)filterCount, specs);
        IFileDialog_SetFileTypeIndex(dialog, 1);
    }
    hr = IFileDialog_Show(dialog, (HWND)(uintptr_t)hwnd);
    for (int i = 0; i < filterCount; i++) {
        free((void*)specs[i].pszName);
        free((void*)specs[i].pszSpec);
    }
    size_t len = 0, cap = 0;
    if (adm_pick_last_paths) {
        cap = strlen(adm_pick_last_paths) + 1;
    }
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        adm_pick_last_status = 1;
    } else if (FAILED(hr)) {
        adm_pick_fail("the dialog", hr);
    } else if (kind == 1 && multiple) {
        IShellItemArray* items = NULL;
        hr = IFileOpenDialog_GetResults((IFileOpenDialog*)dialog, &items);
        if (FAILED(hr) || !items) {
            adm_pick_fail("GetResults", hr);
        } else {
            DWORD count = 0;
            IShellItemArray_GetCount(items, &count);
            for (DWORD i = 0; i < count; i++) {
                IShellItem* item = NULL;
                if (FAILED(IShellItemArray_GetItemAt(items, i, &item)) || !item) continue;
                wchar_t* wpath = NULL;
                if (SUCCEEDED(IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &wpath)) && wpath) {
                    adm_pick_append_utf8(&adm_pick_last_paths, &len, &cap, wpath, 1);
                    CoTaskMemFree(wpath);
                }
                IShellItem_Release(item);
            }
            IShellItemArray_Release(items);
        }
    } else {
        IShellItem* item = NULL;
        hr = IFileDialog_GetResult(dialog, &item);
        if (FAILED(hr) || !item) {
            adm_pick_fail("GetResult", hr);
        } else {
            wchar_t* wpath = NULL;
            if (SUCCEEDED(IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &wpath)) && wpath) {
                adm_pick_append_utf8(&adm_pick_last_paths, &len, &cap, wpath, 0);
                CoTaskMemFree(wpath);
            }
            IShellItem_Release(item);
        }
    }
    IFileDialog_Release(dialog);
    if (entered && init != RPC_E_CHANGED_MODE) CoUninitialize();
    return adm_pick_last_paths ? adm_pick_last_paths : "";
}

// The last call's result: 0 chosen, 1 cancelled, 2 failed.
static int adm_pick_status(void) {
    return adm_pick_last_status;
}

// Why the last call failed.
static const char* adm_pick_error(void) {
    return adm_pick_last_error;
}

#endif /* _WIN32 */
#endif /* ADM_PICKERS_WIN32_H */
