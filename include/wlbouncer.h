#ifndef WL_BOUNCER_H
#define WL_BOUNCER_H

extern "C" {
struct wl_display;

typedef bool (*wl_display_global_filter_func_t)(
    const struct wl_client *client,
    const struct wl_global *global,
    void *data);

/// Should be called immediately after creating a Wayland display, instead of calling wl_display_set_global_filter()
/// display: the Wayland display to filter globals for
/// config_file: the config file to load, or null to use default config file discovery
/// filter: an additional filter to use, or null
/// filter_data: the data argument for the filter function
void wl_bouncer_init_for_display(
    wl_display* display,
    const char* config_file,
    wl_display_global_filter_func_t filter,
    void* filter_data);
}

#endif // WL_BOUNCER_H
