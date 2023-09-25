#ifndef WL_BOUNCER_POLICY_H
#define WL_BOUNCER_POLICY_H

#include <unistd.h>
#include <string>
#include <memory>
#include <vector>

struct wl_client;

class Policy {
public:
    struct Client;

    Policy(const char* config_file);
    ~Policy();

    auto client(wl_client* client) -> std::shared_ptr<Client>;
    auto check(Client const& client, std::string const& interface) -> bool;

private:
    Policy(Policy const&) = delete;
    auto operator=(Policy const&) = delete;

    struct Directive;

    std::vector<Directive> directives;

    void load(const char* config_file);
};

#endif // WL_BOUNCER_POLICY_H
