#include "wlbouncer.h"
#include "policy.h"
#include <unordered_map>
#include <string>
#include <iostream>
#include <wayland-server-core.h>

void (*real_wl_display_set_global_filter)(
    wl_display *display,
    wl_display_global_filter_func_t filter,
    void *data
) = wl_display_set_global_filter;

const bool bouncer_debug = getenv("BOUNCER_DEBUG");

namespace {

struct ClientCtx;

struct DisplayCtx {
    Policy policy;
    std::unordered_map<wl_client const*, ClientCtx*> clients;
    wl_listener client_construction_listener;
    wl_listener display_destruction_listener;
};

static_assert(
    std::is_standard_layout<DisplayCtx>::value,
    "DisplayCtx must be standard layout due to wl_container_of requirements");

struct ClientCtx {
    std::unordered_map<std::string, bool> cache;
    std::shared_ptr<Policy::Client> policy_client;
    DisplayCtx* display_ctx = nullptr;
    wl_client const* client = nullptr;
    wl_listener destroy_listener;
};

static_assert(
    std::is_standard_layout<ClientCtx>::value,
    "ClientCtx must be standard layout due to wl_container_of requirements");

auto filter_func(wl_client const* client, wl_global const* global, void* data) -> bool {
    auto const display_ctx = reinterpret_cast<DisplayCtx*>(data);
    auto client_ctx = display_ctx->clients.find(client);
    if (client_ctx == display_ctx->clients.end()) {
        std::cerr << "wlbouncer: unknown client " << client << std::endl;
        return false;
    }
    std::string const interface_name = wl_global_get_interface(global)->name;
    auto value = client_ctx->second->cache.find(interface_name);
    bool used_cache = true;
    if (value == client_ctx->second->cache.end()) {
        auto const policy_client = client_ctx->second->policy_client.get();
        auto const result = policy_client
            ? display_ctx->policy.check(*policy_client, interface_name)
            : false;
        value = client_ctx->second->cache.insert({interface_name, result}).first;
        used_cache = false;
    }
    if (bouncer_debug) {
        std::cerr << "wlbouncer: " << interface_name
            << (value->second ? " enabled" : " disabled")
            << (used_cache ? " (cached)" : "")
            << std::endl;
    }
    return value->second;
}

void handle_client_destroyed(wl_listener* listener, void* data) {
    auto const client = reinterpret_cast<wl_client*>(data);
    if (bouncer_debug) {
        std::cerr << "wlbouncer: client " << client << " destroyed" << std::endl;
    }
    ClientCtx* ctx = wl_container_of(listener, ctx, destroy_listener);
    ctx->display_ctx->clients.erase(client);
    delete ctx;
}

void handle_client_created(wl_listener* listener, void* data) {
    DisplayCtx* display_ctx;
    display_ctx = wl_container_of(listener, display_ctx, client_construction_listener);
    auto const client = reinterpret_cast<wl_client*>(data);
    if (bouncer_debug) {
        std::cerr << "wlbouncer: client " << client << " created" << std::endl;
    }
    auto const client_ctx = new ClientCtx{};
    display_ctx->clients.insert({client, client_ctx});
    client_ctx->display_ctx = display_ctx;
    client_ctx->client = client;
    client_ctx->policy_client = display_ctx->policy.client(client);
    client_ctx->destroy_listener.notify = &handle_client_destroyed;
    wl_client_add_destroy_listener(client, &client_ctx->destroy_listener);
}

void handle_display_destroyed(wl_listener* listener, void* data) {
    auto const display = reinterpret_cast<wl_display*>(data);
    if (bouncer_debug) {
        std::cerr << "wlbouncer: display " << display << " destroyed" << std::endl;
    }
    DisplayCtx* display_ctx;
    display_ctx = wl_container_of(listener, display_ctx, display_destruction_listener);
    delete display_ctx;
}
}

extern "C" {

void wl_bouncer_init_for_display(wl_display* display) {
    if (bouncer_debug) {
        std::cerr << "wlbouncer: initialized for display " << display << std::endl;
    }
    auto display_ctx = new DisplayCtx{};
    display_ctx->client_construction_listener.notify = &handle_client_created;
    wl_display_add_client_created_listener(display, &display_ctx->client_construction_listener);
    // This handles deleting DisplayCtx when display is destoryed
    display_ctx->display_destruction_listener.notify = &handle_display_destroyed;
    wl_display_add_destroy_listener(display, &display_ctx->display_destruction_listener);
    real_wl_display_set_global_filter(display, filter_func, display_ctx);
}
}
