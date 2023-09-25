#include "policy.h"
#include <iostream>
#include <wayland-server-core.h>
#include <pwd.h>
#include <grp.h>
#include <yaml-cpp/yaml.h>
#include <set>
#include <functional>
#include <optional>

extern bool bouncer_debug;

namespace {

static const std::set<std::string> default_extensions {
    "wl_compositor",
    "wl_subcompositor",
    "wl_data_device_manager",
    "wl_shm",
    "wl_seat",
    "wl_output",
    "xdg_wm_base",
    "xdg_activation_v1",
    "zxdg_output_manager_v1",
    "zxdg_decoration_manager_v1",
    "zwp_relative_pointer_manager_v1",
    "zwp_tablet_manager_v2",
    "zwp_text_input_manager_v3",
    "zwp_text_input_manager_v2",
    "zwp_text_input_manager_v1",
    "org_kde_kwin_server_decoration_manager",
};

using ConditionList = std::vector<std::function<bool(Policy::Client const&)>>;
}

struct Policy::Client {
    pid_t const pid;
    uid_t const uid;
    gid_t const gid;
    std::string const username;
    std::string const groupname;
};

struct Policy::Directive {
public:
    ConditionList const conditions;
    bool const fallthrough;
    bool const enable;
    std::set<std::string> const extensions;

    auto check(Client const& client, std::string const& interface) -> std::optional<bool> {
        auto const is_in_set = extensions.contains(interface);
        // Do not return a value if the interface is not mentioned and this directive falls through
        if (fallthrough && !is_in_set) {
            return std::nullopt;
        }
        // Do not return a value if all the conditions do not match
        for (auto const& condition : conditions) {
            if (!condition(client)) {
                return std::nullopt;
            }
        }
        return std::make_optional(enable == is_in_set);
    }
};

Policy::Policy() {
    load();
}

Policy::~Policy() {}

auto Policy::client(wl_client* client) -> std::shared_ptr<Client> {
    pid_t pid;
    uid_t uid;
    gid_t gid;
    wl_client_get_credentials(client, &pid, &uid, &gid);
    passwd const* const pw = getpwuid(uid);
    if (!pw) {
        std::cerr << "wlbouncer: failed to get username of user with uid = " << uid << std::endl;
        return nullptr;
    }
    group const* const g = getgrgid(gid);
    if (!g) {
        std::cerr << "wlbouncer: failed to get group name of user with gid = " << gid << std::endl;
        return nullptr;
    }
    auto const result = std::shared_ptr<Client>(new Client{
        pid, uid, gid, pw->pw_name, g->gr_name,
    });
    if (bouncer_debug) {
        std::cerr << "wlbouncer: " << result->pid << " from " << result->username << " connected" << std::endl;
    }
    return result;
}

auto Policy::check(Client const& client, std::string const& interface) -> bool {
    // Iterate through directives backwards because last one that returns a value is what we care about
    std::optional<bool> result;
    for (auto i = directives.rbegin(); i != directives.rend(); ++i ) {
        result = i->check(client, interface);
        if (result.has_value()) {
            break;
        }
    }
    if (result.has_value()) {
        return result.value();
    } else {
        return default_extensions.contains(interface) || client.pid == getpid();
    }
}

namespace {

void parse_protocol_list(
    YAML::Node const& node,
    bool enable,
    bool only,
    bool* enable_out,
    bool* fallthrough_out,
    std::set<std::string>* extensions_out,
    std::string* found_key_out
) {
    auto const key = std::string{enable ? "enable" : "disable"} + std::string{only ? "-only" : ""};
    if (node[key]) {
        if (!found_key_out->empty()) {
            throw std::runtime_error{"policy directive contains both " + *found_key_out + " and " + key};
        }
        *found_key_out = key;
        *enable_out = enable;
        *fallthrough_out = !only;
        if (node[key].IsScalar()) {
            if (node[key].as<std::string>() == "all") {
                *enable_out = !enable;
                *fallthrough_out = false;
            } else {
                extensions_out->insert(node[key].as<std::string>());
            }
        } else if (node[key].IsSequence()) {
            for (auto const& extension : node[key]) {
                if (extension.IsScalar()) {
                    if (extension.as<std::string>() == "all") {
                        throw std::runtime_error{"'all' is only allowed when it is the only item"};
                    }
                    extensions_out->insert(extension.as<std::string>());
                } else {
                    throw std::runtime_error{key + " list items should be strings"};
                }
            }
        } else {
            throw std::runtime_error{key + " value should be string or list"};
        }
    }
}

template<typename T>
void parse_condition(
    YAML::Node const& node,
    std::string const& name,
    std::function<T(Policy::Client const&)> get_item,
    ConditionList* conditions_out
) {
    auto const name_plural = name + "s";
    if (node[name]) {
        if (node[name_plural]) {
            throw std::runtime_error{"policy directive should not have both " + name + " and " + name_plural};
        }
        if (node[name].IsScalar()) {
            T const item = node[name].as<T>();
            conditions_out->push_back([=](auto client){
               return item == get_item(client);
            });
        } else {
            throw std::runtime_error{
                name + " should have a single value, use " + name_plural + " for a list of " + name_plural};
        }
    } else if (node[name_plural]) {
        if (node[name_plural].IsSequence()) {
            std::set<T> result;
            for (auto const& item : node[name_plural]) {
                if (item.IsScalar()) {
                    result.insert(item.as<T>());
                } else {
                    throw std::runtime_error{name_plural + " contains non-string value"};
                }
            }
            conditions_out->push_back([=](auto client){
               return result.contains(get_item(client));
            });
        } else {
            throw std::runtime_error{name_plural + " should be a list of values"};
        }
    }
}
}

void Policy::load()
{
    directives.clear();
    auto const filename = "wlbouncer.yaml";
    try {
        YAML::Node root = YAML::LoadFile(filename);
        if (root["version"].as<int>() != 0) {
            throw std::runtime_error{"invalid config file version"};
        }
        for (auto const& node : root["policy"]) {
            bool enable = false;
            bool fallthrough = false;
            std::set<std::string> extensions;
            std::string found_key;
            parse_protocol_list(node, true, false, &enable, &fallthrough, &extensions, &found_key);
            parse_protocol_list(node, false, false, &enable, &fallthrough, &extensions, &found_key);
            parse_protocol_list(node, true, true, &enable, &fallthrough, &extensions, &found_key);
            parse_protocol_list(node, false, true, &enable, &fallthrough, &extensions, &found_key);
            if (found_key.empty()) {
                throw std::runtime_error{
                    "policy directive did not contain enable, enable-only, disable or disable-only"};
            }
            ConditionList conditions;
            parse_condition<pid_t>(node, "pid", [](auto client){ return client.pid; }, &conditions);
            parse_condition<uid_t>(node, "uid", [](auto client){ return client.uid; }, &conditions);
            parse_condition<gid_t>(node, "gid", [](auto client){ return client.gid; }, &conditions);
            parse_condition<std::string>(node, "user", [](auto client){ return client.username; }, &conditions);
            parse_condition<std::string>(node, "group", [](auto client){ return client.groupname; }, &conditions);
            directives.emplace_back(conditions, fallthrough, enable, extensions);
        }
    } catch (std::exception& e) {
        std::cerr << "wlbouncer: error loading " << filename << ": " << e.what() << std::endl;
    }
    std::cerr << "wlbouncer: " << directives.size() << " policy directives loaded" << std::endl;
}
