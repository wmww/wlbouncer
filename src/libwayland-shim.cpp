#include <iostream>
#include <dlfcn.h>
#include "wlbouncer.h"
#include <wayland-server-core.h>

wl_display* (*real_wl_display_create)(void) = nullptr;
extern void (*real_wl_display_set_global_filter)(
    wl_display *display,
    wl_display_global_filter_func_t filter,
    void *data
);

bool const keep_ld_preload = getenv("BOUNCER_KEEP_LD_PRELOAD");

static void libwayland_shim_init()
{
    if (real_wl_display_create)
        return;

    void *handle = dlopen("libwayland-server.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "wlbouncer: failed to dlopen libwayland" << std::endl;
        return;
    }

#define INIT_SYM(name) if (!(real_##name = reinterpret_cast<decltype(real_##name)>(dlsym(handle, #name)))) {\
    std::cerr << "wlbouncer: dlsym failed to load " << #name << std::endl; }

    INIT_SYM(wl_display_create);
    INIT_SYM(wl_display_set_global_filter);

#undef INIT_SYM

    // Do not close as long as any symbols are in use
    //dlclose(handle);
}

extern "C" {

struct wl_display* wl_display_create() {
    if (!keep_ld_preload) {
        unsetenv("LD_PRELOAD");
    }
    libwayland_shim_init();
    struct wl_display* const display = real_wl_display_create();
    wl_bouncer_init_for_display(display);
    return display;
}

void wl_display_set_global_filter(
    struct wl_display *display,
    wl_display_global_filter_func_t filter,
    void *data
) {
    (void)display;
    (void)filter;
    (void)data;
    std::cerr <<
        "wlbouncer: compositor called wl_display_set_global_filter(), "
        "but this is being ignored because wlbouncer has been preloaded." << std::endl;
}

}
