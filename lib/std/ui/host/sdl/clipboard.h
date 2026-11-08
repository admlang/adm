// The system clipboard's data of any MIME type, through SDL3, for
// std.ui.host.sdl.lib. SDL hands the data out through a callback whenever
// another program asks for it, so the bytes are copied into memory SDL
// frees when the clipboard changes hands. Only the sidecar of that module
// includes this header, so a program holds one definition of each function.
#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct adm_sdl_clip_blob {
    void* data;
    size_t size;
} adm_sdl_clip_blob;

static const void* SDLCALL adm_sdl_clip_data(void* userdata, const char* mime, size_t* size) {
    adm_sdl_clip_blob* blob = (adm_sdl_clip_blob*)userdata;
    (void)mime;
    if (blob == NULL) {
        if (size != NULL) *size = 0;
        return NULL;
    }
    if (size != NULL) *size = blob->size;
    return blob->data;
}

static void SDLCALL adm_sdl_clip_cleanup(void* userdata) {
    adm_sdl_clip_blob* blob = (adm_sdl_clip_blob*)userdata;
    if (blob == NULL) return;
    SDL_free(blob->data);
    SDL_free(blob);
}

// Puts a copy of `size` bytes at `data` on the clipboard as `mime`,
// replacing what was there; false when SDL refuses (SDL_GetError says why).
bool adm_sdl_clipboard_set(const char* mime, const uint8_t* data, size_t size) {
    adm_sdl_clip_blob* blob = (adm_sdl_clip_blob*)SDL_malloc(sizeof *blob);
    if (blob == NULL) return false;
    blob->data = SDL_malloc(size > 0 ? size : 1);
    if (blob->data == NULL) {
        SDL_free(blob);
        return false;
    }
    if (size > 0) memcpy(blob->data, data, size);
    blob->size = size;
    const char* mimes[1] = { mime };
    if (!SDL_SetClipboardData(adm_sdl_clip_data, adm_sdl_clip_cleanup, blob, mimes, 1)) {
        adm_sdl_clip_cleanup(blob);
        return false;
    }
    return true;
}

// The clipboard's data of `mime`, `*size` bytes the caller frees with
// SDL_free; NULL when it offers none.
void* adm_sdl_clipboard_get(const char* mime, size_t* size) {
    return SDL_GetClipboardData(mime, size);
}

// The MIME types the clipboard offers: `*count` C strings in one block the
// caller frees with SDL_free; NULL when it offers none.
char** adm_sdl_clipboard_formats(size_t* count) {
    return SDL_GetClipboardMimeTypes(count);
}

// The type at `index` of a list adm_sdl_clipboard_formats gave.
const char* adm_sdl_clipboard_format(char** list, size_t index) {
    return list[index];
}
