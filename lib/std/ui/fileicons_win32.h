// The system image lists for std.ui's systemFileIcon on Windows: a kind of file's icon by
// its name alone (SHGFI_USEFILEATTRIBUTES), so the file need not exist, read out as RGBA
// from whatever thread calls (COM is entered for the call). Included by the native-support
// unit that std/ui/fileicons_windows.adm binds, whose compiled code calls the functions it
// names: those have external linkage. Empty elsewhere.
#ifndef ADM_FILEICONS_WIN32_H
#define ADM_FILEICONS_WIN32_H

#ifdef _WIN32

#define COBJMACROS
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <stdint.h>
#include <stdlib.h>

// IID_IImageList, spelled out: the library that defines it is not always linked.
static const GUID adm_file_icon_iid = {0x46EB5926, 0x582E, 0x4017, {0x9F, 0xDF, 0xE8, 0x99, 0x8D, 0xAA, 0x09, 0x50}};

// The lists from small to large; their sides follow the system's scale.
static const int adm_file_icon_lists[] = {SHIL_SMALL, SHIL_LARGE, SHIL_EXTRALARGE, SHIL_JUMBO};

static IImageList* adm_file_icon_list(int which) {
    IImageList* list = NULL;
    if (FAILED(SHGetImageList(adm_file_icon_lists[which], &adm_file_icon_iid, (void**)&list))) return NULL;
    return list;
}

static int64_t adm_file_icon_list_side(IImageList* list) {
    int wide = 0, high = 0;
    if (FAILED(IImageList_GetIconSize(list, &wide, &high))) return 0;
    return wide;
}

// Enters COM for a call; says whether the call has to leave it.
static int adm_file_icon_enter(void) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    return hr == S_OK || hr == S_FALSE;
}

// The smallest list that holds `want` pixels, else the largest. 0: no list.
int64_t adm_file_icon_side(int64_t want) {
    int entered = adm_file_icon_enter();
    int64_t found = 0;
    for (int which = 0; which < 4; which++) {
        IImageList* list = adm_file_icon_list(which);
        if (!list) continue;
        int64_t side = adm_file_icon_list_side(list);
        IImageList_Release(list);
        if (side <= 0) continue;
        found = side;
        if (side >= want) break;
    }
    if (entered) CoUninitialize();
    return found;
}

// A bitmap as top-down 32-bit pixels, blue first.
static int adm_file_icon_bits(HDC dc, HBITMAP bitmap, int64_t side, uint8_t* out) {
    BITMAPINFO info;
    ZeroMemory(&info, sizeof(info));
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = (LONG)side;
    info.bmiHeader.biHeight = -(LONG)side;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    return GetDIBits(dc, bitmap, 0, (UINT)side, out, &info, DIB_RGB_COLORS) == (int)side;
}

// An icon as RGBA in a square `side` on a side. An icon of the old kind has no alpha at
// all and a mask instead, in which a set pixel is one that does not show.
static int adm_file_icon_pixels(HICON icon, int64_t side, uint8_t* out) {
    ICONINFO parts;
    if (!GetIconInfo(icon, &parts)) return 0;
    int ok = 0;
    HDC dc = GetDC(NULL);
    if (dc && parts.hbmColor && adm_file_icon_bits(dc, parts.hbmColor, side, out)) {
        int64_t count = side * side;
        int anyAlpha = 0;
        for (int64_t i = 0; i < count; i++) {
            uint8_t* px = out + i * 4;
            uint8_t blue = px[0];
            px[0] = px[2];
            px[2] = blue;
            if (px[3] != 0) anyAlpha = 1;
        }
        ok = 1;
        if (!anyAlpha) {
            uint8_t* mask = (uint8_t*)malloc((size_t)(count * 4));
            if (mask && parts.hbmMask && adm_file_icon_bits(dc, parts.hbmMask, side, mask)) {
                for (int64_t i = 0; i < count; i++) out[i * 4 + 3] = mask[i * 4] ? 0 : 255;
            } else {
                for (int64_t i = 0; i < count; i++) out[i * 4 + 3] = 255;
            }
            free(mask);
        }
    }
    if (dc) ReleaseDC(NULL, dc);
    if (parts.hbmColor) DeleteObject(parts.hbmColor);
    if (parts.hbmMask) DeleteObject(parts.hbmMask);
    return ok;
}

int32_t adm_file_icon_read(const char* name, int32_t folder, int64_t side, uint8_t* out) {
    wchar_t wide[MAX_PATH];
    if (MultiByteToWideChar(CP_UTF8, 0, name ? name : "", -1, wide, MAX_PATH) <= 0) return 0;
    int entered = adm_file_icon_enter();
    int32_t ok = 0;
    SHFILEINFOW shown;
    ZeroMemory(&shown, sizeof(shown));
    DWORD attributes = folder ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    if (SHGetFileInfoW(wide, attributes, &shown, sizeof(shown), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES)) {
        for (int which = 0; which < 4 && !ok; which++) {
            IImageList* list = adm_file_icon_list(which);
            if (!list) continue;
            if (adm_file_icon_list_side(list) == side) {
                HICON icon = NULL;
                if (SUCCEEDED(IImageList_GetIcon(list, shown.iIcon, ILD_TRANSPARENT, &icon)) && icon) {
                    ok = adm_file_icon_pixels(icon, side, out);
                    DestroyIcon(icon);
                }
            }
            IImageList_Release(list);
        }
    }
    if (entered) CoUninitialize();
    return ok;
}

#endif // _WIN32

#endif // ADM_FILEICONS_WIN32_H
