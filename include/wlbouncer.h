#ifndef WL_BOUNCER_H
#define WL_BOUNCER_H

extern "C" {
struct wl_display;

/// Should be called immediately after creating a Wayland display, instead of calling wl_display_set_global_filter()
/// display: the Wayland display to filter globals for
/// config_file: the config file to load, or null to use default config file discovery
void wl_bouncer_init_for_display(wl_display* display, const char* config_file);
}

#endif // WL_BOUNCER_H
