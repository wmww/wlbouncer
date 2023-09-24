#ifndef WL_BOUNCER_H
#define WL_BOUNCER_H

extern "C" {
struct wl_display;
void wl_bouncer_init_for_display(wl_display* display);
}

#endif // WL_BOUNCER_H
