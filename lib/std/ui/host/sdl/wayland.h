// Where a Wayland window is there for the pointer, for std.ui.host.sdl.lib: SDL
// shapes windows under X11 only, so the surface's input region is set here,
// through the connection and the surface SDL holds. libwayland-client is the
// one SDL loaded, found by name at the first call (a program never links it).
// Only the sidecar of that module includes this header, so a program holds one
// definition of each function.
#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Requests by their number in the protocol (wayland.xml).
#define ADM_WL_DISPLAY_GET_REGISTRY 1
#define ADM_WL_REGISTRY_BIND 0
#define ADM_WL_COMPOSITOR_CREATE_REGION 1
#define ADM_WL_REGION_DESTROY 0
#define ADM_WL_REGION_ADD 1
#define ADM_WL_SURFACE_SET_INPUT_REGION 5
#define ADM_WL_SURFACE_COMMIT 6
// wl_proxy_marshal_flags: the request destroys the object it is sent to.
#define ADM_WL_MARSHAL_DESTROY 1

typedef struct adm_wl_registry_listener {
    void (*global)(void* data, void* registry, uint32_t name, const char* interface, uint32_t version);
    void (*global_remove)(void* data, void* registry, uint32_t name);
} adm_wl_registry_listener;

typedef struct adm_wl_client {
    bool loaded;
    bool usable;
    void* (*marshal)(void* proxy, uint32_t opcode, const void* interface, uint32_t version, uint32_t flags, ...);
    int (*add_listener)(void* proxy, void (**implementation)(void), void* data);
    void (*destroy)(void* proxy);
    uint32_t (*version)(void* proxy);
    void* (*create_wrapper)(void* proxy);
    void (*wrapper_destroy)(void* wrapper);
    void (*set_queue)(void* proxy, void* queue);
    void* (*create_queue)(void* display);
    void (*queue_destroy)(void* queue);
    int (*roundtrip_queue)(void* display, void* queue);
    int (*flush)(void* display);
    const void* registry_interface;
    const void* compositor_interface;
    const void* region_interface;
    // The compositor bound for `display`, with the queue its registry answered on.
    void* display;
    void* queue;
    void* compositor;
} adm_wl_client;

static adm_wl_client adm_wl;

static bool adm_wl_load(void) {
    if (adm_wl.loaded) return adm_wl.usable;
    adm_wl.loaded = true;
    void* lib = dlopen("libwayland-client.so.0", RTLD_NOW | RTLD_GLOBAL);
    if (lib == NULL) return false;
    *(void**)&adm_wl.marshal = dlsym(lib, "wl_proxy_marshal_flags");
    *(void**)&adm_wl.add_listener = dlsym(lib, "wl_proxy_add_listener");
    *(void**)&adm_wl.destroy = dlsym(lib, "wl_proxy_destroy");
    *(void**)&adm_wl.version = dlsym(lib, "wl_proxy_get_version");
    *(void**)&adm_wl.create_wrapper = dlsym(lib, "wl_proxy_create_wrapper");
    *(void**)&adm_wl.wrapper_destroy = dlsym(lib, "wl_proxy_wrapper_destroy");
    *(void**)&adm_wl.set_queue = dlsym(lib, "wl_proxy_set_queue");
    *(void**)&adm_wl.create_queue = dlsym(lib, "wl_display_create_queue");
    *(void**)&adm_wl.queue_destroy = dlsym(lib, "wl_event_queue_destroy");
    *(void**)&adm_wl.roundtrip_queue = dlsym(lib, "wl_display_roundtrip_queue");
    *(void**)&adm_wl.flush = dlsym(lib, "wl_display_flush");
    adm_wl.registry_interface = dlsym(lib, "wl_registry_interface");
    adm_wl.compositor_interface = dlsym(lib, "wl_compositor_interface");
    adm_wl.region_interface = dlsym(lib, "wl_region_interface");
    adm_wl.usable = adm_wl.marshal != NULL && adm_wl.add_listener != NULL && adm_wl.destroy != NULL &&
        adm_wl.version != NULL && adm_wl.create_wrapper != NULL && adm_wl.wrapper_destroy != NULL &&
        adm_wl.set_queue != NULL && adm_wl.create_queue != NULL && adm_wl.queue_destroy != NULL &&
        adm_wl.roundtrip_queue != NULL && adm_wl.flush != NULL && adm_wl.registry_interface != NULL &&
        adm_wl.compositor_interface != NULL && adm_wl.region_interface != NULL;
    return adm_wl.usable;
}

static void adm_wl_global(void* data, void* registry, uint32_t name, const char* interface, uint32_t version) {
    (void)data;
    (void)version;
    if (adm_wl.compositor != NULL || strcmp(interface, "wl_compositor") != 0) return;
    // A wl_interface starts with its name. Version 1 has the regions.
    const char* bound = *(const char* const*)adm_wl.compositor_interface;
    adm_wl.compositor = adm_wl.marshal(registry, ADM_WL_REGISTRY_BIND, adm_wl.compositor_interface, 1, 0, name, bound, (uint32_t)1, NULL);
}

static void adm_wl_global_remove(void* data, void* registry, uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}

static const adm_wl_registry_listener adm_wl_registry_events = { adm_wl_global, adm_wl_global_remove };

// Gives up the compositor bound for the connection; called before SDL closes it.
void adm_sdl_wayland_forget(void) {
    if (adm_wl.compositor != NULL) adm_wl.destroy(adm_wl.compositor);
    if (adm_wl.queue != NULL) adm_wl.queue_destroy(adm_wl.queue);
    adm_wl.compositor = NULL;
    adm_wl.queue = NULL;
    adm_wl.display = NULL;
}

// The connection's wl_compositor, bound once for it. The registry answers on a
// queue of its own, so none of SDL's events is handled here.
static void* adm_wl_compositor(void* display) {
    if (adm_wl.display == display && adm_wl.compositor != NULL) return adm_wl.compositor;
    adm_sdl_wayland_forget();
    void* queue = adm_wl.create_queue(display);
    if (queue == NULL) return NULL;
    void* wrapper = adm_wl.create_wrapper(display);
    if (wrapper == NULL) {
        adm_wl.queue_destroy(queue);
        return NULL;
    }
    adm_wl.set_queue(wrapper, queue);
    void* registry = adm_wl.marshal(wrapper, ADM_WL_DISPLAY_GET_REGISTRY, adm_wl.registry_interface, adm_wl.version(wrapper), 0, NULL);
    adm_wl.wrapper_destroy(wrapper);
    if (registry == NULL) {
        adm_wl.queue_destroy(queue);
        return NULL;
    }
    adm_wl.add_listener(registry, (void (**)(void))&adm_wl_registry_events, NULL);
    adm_wl.roundtrip_queue(display, queue);
    adm_wl.destroy(registry);
    if (adm_wl.compositor == NULL) {
        adm_wl.queue_destroy(queue);
        return NULL;
    }
    adm_wl.display = display;
    adm_wl.queue = queue;
    return adm_wl.compositor;
}

// Makes the surface take the pointer where the alpha of `pixels` (`width` by
// `height`, four bytes a pixel, stretched over the `wide` by `high` surface
// coordinates of the window) is not 0, and nowhere else. A surface coordinate
// is in when any pixel over it is. False when libwayland-client or the
// compositor's regions cannot be reached.
bool adm_sdl_wayland_input_shape(void* display, void* surface, const uint8_t* pixels, int width, int height, int wide, int high) {
    if (display == NULL || surface == NULL || pixels == NULL || width <= 0 || height <= 0 || wide <= 0 || high <= 0) return false;
    if (!adm_wl_load()) return false;
    void* compositor = adm_wl_compositor(display);
    if (compositor == NULL) return false;
    // One byte a surface column for the row being read and for the row before it.
    uint8_t* rows = (uint8_t*)malloc((size_t)wide * 2);
    if (rows == NULL) return false;
    void* region = adm_wl.marshal(compositor, ADM_WL_COMPOSITOR_CREATE_REGION, adm_wl.region_interface, adm_wl.version(compositor), 0, NULL);
    if (region == NULL) {
        free(rows);
        return false;
    }
    uint8_t* now = rows;
    uint8_t* before = rows + wide;
    // The rows from `since` on have the columns of `before`: one rectangle a run.
    int since = 0;
    for (int y = 0; y <= high; y++) {
        if (y < high) {
            int top = (int)((int64_t)y * height / high);
            int bottom = (int)(((int64_t)(y + 1) * height + high - 1) / high);
            memset(now, 0, (size_t)wide);
            for (int x = 0; x < wide; x++) {
                int left = (int)((int64_t)x * width / wide);
                int right = (int)(((int64_t)(x + 1) * width + wide - 1) / wide);
                uint8_t any = 0;
                for (int py = top; py < bottom && any == 0; py++) {
                    const uint8_t* alpha = pixels + ((size_t)py * (size_t)width + (size_t)left) * 4 + 3;
                    for (int px = left; px < right; px++, alpha += 4) any |= *alpha;
                }
                now[x] = any != 0;
            }
            if (y > 0 && memcmp(now, before, (size_t)wide) == 0) continue;
        }
        if (y > 0) {
            for (int x = 0; x < wide;) {
                if (before[x] == 0) {
                    x++;
                    continue;
                }
                int start = x;
                while (x < wide && before[x] != 0) x++;
                adm_wl.marshal(region, ADM_WL_REGION_ADD, NULL, adm_wl.version(region), 0, (int32_t)start, (int32_t)since, (int32_t)(x - start), (int32_t)(y - since));
            }
        }
        uint8_t* swap = before;
        before = now;
        now = swap;
        since = y;
    }
    free(rows);
    adm_wl.marshal(surface, ADM_WL_SURFACE_SET_INPUT_REGION, NULL, adm_wl.version(surface), 0, region);
    adm_wl.marshal(region, ADM_WL_REGION_DESTROY, NULL, adm_wl.version(region), ADM_WL_MARSHAL_DESTROY);
    // The region belongs to the surface's next state, which SDL commits with a
    // frame only; a window that draws nothing more gets it now.
    adm_wl.marshal(surface, ADM_WL_SURFACE_COMMIT, NULL, adm_wl.version(surface), 0);
    adm_wl.flush(display);
    return true;
}
