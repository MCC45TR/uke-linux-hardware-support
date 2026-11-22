// SPDX-License-Identifier: MIT
#pragma once
#include <cstdlib>
#include <optional>
#include <set>
#include <string>
#include <systemd/sd-login.h>
#include <vector>

namespace uke::reporting {
struct Session {uid_t uid;bool active;std::string type;};
inline std::optional<uid_t> select_uid(const std::vector<Session>& sessions) {
    std::set<uid_t> users;
    for(const auto& session:sessions)if(session.uid&&session.active&&(session.type=="wayland"||session.type=="x11"))users.insert(session.uid);
    if(users.size()!=1)return std::nullopt;
    return *users.begin();
}
inline std::optional<uid_t> graphical_uid() {
    char** names=nullptr;const int count=sd_get_sessions(&names);std::vector<Session> sessions;
    for(int i=0;i<count;++i) {
        uid_t uid=0;char* type=nullptr;
        if(count<=256&&sd_session_get_uid(names[i],&uid)>=0&&sd_session_get_type(names[i],&type)>=0)
            sessions.push_back({uid,sd_session_is_active(names[i])>0,type?type:""});
        std::free(type);std::free(names[i]);
    }
    std::free(names);return select_uid(sessions);
}
}
