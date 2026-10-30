// SPDX-License-Identifier: MIT
#define main orientation_daemon_main
#include "../src/uke-speaker-orientation.cpp"
#undef main
#include <cassert>

static std::array<unsigned, 4> actual;
static int query_index, query_item, writes, fail_write, missing_control = -1;
static bool wrong_type, wrong_labels, ignore_write, fail_read;
static int fake_active = 1, fake_remote;
static uid_t fake_uid;
static const char *fake_type = "wayland";
static const char *pid_session;
static std::array<const char *, 4> sessions;
static unsigned session_count;
static int session_error, enumeration_calls;
static int index_of(const char *name) {
    for (unsigned i = 0; i < controls.size(); ++i) if (!std::strcmp(name, controls[i])) return i;
    assert(false); return -1;
}
extern "C" int snd_ctl_elem_info(snd_ctl_t *, snd_ctl_elem_info_t *info) {
    query_index = index_of(snd_ctl_elem_info_get_name(info));
    return query_index == missing_control ? -ENOENT : 0;
}
extern "C" void snd_ctl_elem_info_set_item(snd_ctl_elem_info_t *, unsigned item) { query_item = item; }
extern "C" snd_ctl_elem_type_t snd_ctl_elem_info_get_type(const snd_ctl_elem_info_t *) {
    return wrong_type ? SND_CTL_ELEM_TYPE_INTEGER : SND_CTL_ELEM_TYPE_ENUMERATED;
}
extern "C" unsigned snd_ctl_elem_info_get_count(const snd_ctl_elem_info_t *) { return 1; }
extern "C" unsigned snd_ctl_elem_info_get_items(const snd_ctl_elem_info_t *) { return 4; }
extern "C" const char *snd_ctl_elem_info_get_item_name(const snd_ctl_elem_info_t *) {
    return wrong_labels ? "invalid" : angles.at(query_item);
}
extern "C" int snd_ctl_elem_read(snd_ctl_t *, snd_ctl_elem_value_t *value) {
    if (fail_read) return -EIO;
    snd_ctl_elem_value_set_enumerated(value, 0, actual.at(index_of(snd_ctl_elem_value_get_name(value))));
    return 0;
}
extern "C" int snd_ctl_elem_write(snd_ctl_t *, snd_ctl_elem_value_t *value) {
    if (++writes == fail_write) return -EIO;
    if (!ignore_write) actual.at(index_of(snd_ctl_elem_value_get_name(value))) =
        snd_ctl_elem_value_get_enumerated(value, 0);
    return 0;
}
extern "C" int sd_session_get_uid(const char *session, uid_t *uid) {
    *uid = !std::strcmp(session, "foreign") ? getuid() + 1 : fake_uid; return 0;
}
extern "C" int sd_session_is_active(const char *) { return fake_active; }
extern "C" int sd_session_is_remote(const char *session) {
    return !std::strcmp(session, "remote") ? 1 : fake_remote;
}
extern "C" int sd_session_get_type(const char *session, char **type) {
    *type = strdup(!std::strcmp(session, "x11") ? "x11" : fake_type); return 0;
}
extern "C" int sd_pid_get_session(pid_t, char **session) {
    if (!pid_session) return -ENODATA;
    *session = strdup(pid_session); return 0;
}
extern "C" int sd_uid_get_sessions(uid_t, int, char ***list) {
    ++enumeration_calls;
    if (session_error) return session_error;
    *list = static_cast<char **>(calloc(session_count + 1, sizeof(char *)));
    for (unsigned i = 0; i < session_count; ++i) (*list)[i] = strdup(sessions[i]);
    return session_count;
}
static void reset() {
    actual.fill(0); writes = 0; fail_write = -1; missing_control = -1;
    wrong_type = wrong_labels = ignore_write = fail_read = false;
}

int main() {
    char reference[] = "/tmp/uke-reference-XXXXXX";
    int fd = mkstemp(reference); assert(fd >= 0); close(fd);
    unsigned zero = 99;
    for (unsigned degrees : {0U, 90U, 180U, 270U}) {
        { std::ofstream file(reference); file << degrees << '\n'; }
        assert(read_reference(reference, zero) == 0 && zero == degrees / 90);
    }
    for (const char *invalid : {"", "-1", "45", "360", "90 trailing", "999999999999999999999"}) {
        { std::ofstream file(reference); file << invalid; }
        assert(read_reference(reference, zero) == -EINVAL && zero == 0);
    }
    assert(unlink(reference) == 0);
    assert(read_reference(reference, zero) == 0 && zero == 0);
    assert(target_card("sm7675", "POCO-Pad-X1"));
    assert(!target_card("sm7675", "POCO-Pad-X1-HDMI"));
    assert(!target_card("foreign", "POCO-Pad-X1"));
    reset(); assert(synchronize(nullptr, 4) == -EINVAL && writes == 0);
    for (unsigned turn = 0; turn < 4; ++turn) {
        reset(); assert(synchronize(nullptr, turn) == 0);
        for (auto value : actual) assert(value == turn);
        assert(writes == (turn ? 4 : 0));
        int before = writes; assert(synchronize(nullptr, turn) == 0 && writes == before);
    }
    reset(); missing_control = 3;
    assert(synchronize(nullptr, 1) == -ENOENT && writes == 0);
    reset(); wrong_type = true; assert(synchronize(nullptr, 1) == -EPROTO && writes == 0);
    reset(); wrong_labels = true; assert(synchronize(nullptr, 1) == -EPROTO && writes == 0);
    reset(); fail_read = true; assert(synchronize(nullptr, 1) == -EIO && writes == 0);
    reset(); fail_write = 2; assert(synchronize(nullptr, 1) == -EIO && writes == 2);
    assert(actual[0] == 1 && actual[1] == 0 && actual[2] == 0);
    fail_write = -1; assert(synchronize(nullptr, 1) == 0);
    for (auto value : actual) assert(value == 1);
    reset(); ignore_write = true; assert(synchronize(nullptr, 1) == -EIO && writes == 1);

    fake_uid = getuid(); assert(active_session("fixture"));
    fake_active = 0; assert(!active_session("fixture")); fake_active = 1;
    fake_remote = 1; assert(!active_session("fixture")); fake_remote = 0;
    fake_uid = getuid() + 1; assert(!active_session("fixture")); fake_uid = getuid();
    fake_type = "x11"; assert(!active_session("fixture")); fake_type = "wayland";
    assert(!active_session("") && !active_session(nullptr));
    unsetenv("XDG_SESSION_ID");
    pid_session = "pid-fixture";
    char *selected = resolve_session(); assert(selected && !std::strcmp(selected, pid_session)); free(selected);
    assert(enumeration_calls == 0); pid_session = nullptr;
    setenv("XDG_SESSION_ID", "env-fixture", 1);
    selected = resolve_session(); assert(selected && !std::strcmp(selected, "env-fixture")); free(selected);
    assert(enumeration_calls == 0); unsetenv("XDG_SESSION_ID");
    session_count = 1; sessions[0] = "single-fixture";
    selected = resolve_session(); assert(selected && !std::strcmp(selected, sessions[0])); free(selected);
    fake_active = 0;
    selected = resolve_session(); assert(selected && !active_session(selected)); free(selected);
    fake_active = 1; session_count = 0; assert(!resolve_session());
    session_count = 2; sessions[0] = "fixture-a"; sessions[1] = "fixture-b";
    assert(!resolve_session());
    session_count = 4; sessions = {"remote", "x11", "foreign", "single-fixture"};
    selected = resolve_session(); assert(selected && !std::strcmp(selected, "single-fixture")); free(selected);
    session_error = -EIO; assert(!resolve_session()); session_error = 0;

    App app;
    app.outputs.push_back(Output{&app, 1, nullptr, {}});
    auto &output = app.outputs.back();
    name(&output, nullptr, "DSI-1");
    mode(&output, nullptr, WL_OUTPUT_MODE_CURRENT, 3200, 2136, 60000);
    geometry(&output, nullptr, 0, 0, 237, 158, 0, "fixture", "fixture", 1);
    assert(!app.desired); done(&output, nullptr); assert(app.desired == 1);
    for (int transform = 0; transform < 4; ++transform) {
        geometry(&output, nullptr, 0, 0, 237, 158, 0, "fixture", "fixture", transform);
        assert(app.desired == (transform ? static_cast<unsigned>(transform - 1) : 1));
        done(&output, nullptr); assert(app.desired == static_cast<unsigned>(transform));
    }
    app.zero = 1; app.refresh(); assert(app.desired == 0);
    geometry(&output, nullptr, 0, 0, 237, 158, 0, "fixture", "fixture", 4);
    done(&output, nullptr); assert(!app.desired);
    output.pending_transform = 0; done(&output, nullptr); assert(app.desired == 1);
    app.outputs.push_back(Output{&app, 2, nullptr, "DP-1"});
    auto &external = app.outputs.back();
    external.width = 3200; external.height = 2136; external.transform = 2;
    app.refresh(); assert(app.desired == 1);
    external.name = "DSI-1"; app.refresh(); assert(!app.desired);
    removed(&app, nullptr, 2); assert(app.desired == 1);
    output.pending_width = 1920; done(&output, nullptr); assert(!app.desired);
    removed(&app, nullptr, 1); assert(app.outputs.empty() && !app.desired);
    std::puts("Speaker orientation: actual ALSA transaction, active-session gate and Wayland event handlers passed; host fixtures only");
}
