#include "policy.h"
#include <iostream>
#include <wayland-server-core.h>
#include <pwd.h>
#include <grp.h>

struct Policy::Client {
    pid_t const pid;
    uid_t const uid;
    gid_t const gid;
    std::string const username;
    std::string const groupname;
    std::string const path;
};

Policy::Policy()
{
}

Policy::~Policy()
{
}

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
    return std::shared_ptr<Client>(new Client{
        pid, uid, gid, pw->pw_name, g->gr_name, ""
    });
}

auto Policy::check(Client const& client, std::string const& interface) -> bool {
    return client.username == "root" || interface != "zwlr_screencopy_manager_v1";
}
