// SPDX-License-Identifier: GPL-3.0-or-later
// Native COPR fixture. All payloads/identifiers are synthetic; no SSC opened.
#include <gio/gio.h>
#include <cassert>
#include <vector>
#include "libssc-client-private.h"

extern "C" {
SSCSensor *fixture_sensor_new(SSCClient **);
void fixture_measurement(SSCClient *);
void fixture_discovery(SSCClient *, gboolean, gboolean);
void fixture_attributes(SSCClient *);
}

struct Pending { GTask *task; guint32 message; };
static std::vector<Pending> pending;
static SSCClient *initializing_client;
static void drain() { while (g_main_context_iteration(nullptr, FALSE)) {} }

extern "C" void fixture_client_send(SSCClient *client, guint64, guint64,
    guint32 message, GArray *, GCancellable *cancel,
    GAsyncReadyCallback callback, gpointer data)
{
    pending.push_back({g_task_new(client, cancel, callback, data), message});
}
extern "C" gboolean fixture_client_send_finish(SSCClient *, GAsyncResult *r, GError **e)
{
    return g_task_propagate_boolean(G_TASK(r), e);
}
extern "C" void fixture_client_new(GCancellable *cancel, GAsyncReadyCallback callback, gpointer data)
{
    assert(!initializing_client);
    initializing_client = SSC_CLIENT(g_object_new(SSC_TYPE_CLIENT, nullptr));
    auto *task = g_task_new(nullptr, cancel, callback, data);
    g_task_return_pointer(task, g_object_ref(initializing_client), g_object_unref);
    g_object_unref(task);
}
extern "C" SSCClient *fixture_client_new_finish(GAsyncResult *r, GError **e)
{
    return SSC_CLIENT(g_task_propagate_pointer(G_TASK(r), e));
}
static void ack(unsigned index = 0, bool success = true)
{
    assert(index < pending.size());
    GTask *task = pending[index].task;
    pending.erase(pending.begin() + index);
    if (success) g_task_return_boolean(task, TRUE);
    else g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_FAILED, "synthetic send failure");
    g_object_unref(task);
    drain();
}
static bool subscribed(SSCClient *c)
{
    return g_signal_has_handler_pending(c, g_signal_lookup("report", SSC_TYPE_CLIENT), 0, TRUE);
}
struct Done { unsigned calls = 0, successes = 0, failures = 0; };
static void opened(GObject *source, GAsyncResult *result, gpointer data)
{
    auto &done = *static_cast<Done *>(data);
    GError *error = nullptr;
    const bool success = ssc_sensor_open_finish(SSC_SENSOR(source), result, &error);
    ++done.calls;
    if (success) { assert(!error); ++done.successes; }
    else { assert(error); ++done.failures; g_error_free(error); }
}
static void weak(gpointer data, GObject *) { *static_cast<bool *>(data) = true; }
static void release(SSCSensor *sensor, SSCClient *client)
{
    assert(pending.empty() && !subscribed(client));
    bool gone = false;
    g_object_weak_ref(G_OBJECT(sensor), weak, &gone);
    g_object_unref(sensor);
    drain();
    assert(gone);
    g_object_unref(client);
}
static void overlap()
{
    SSCClient *client;
    auto *sensor = fixture_sensor_new(&client);
    Done done;
    ssc_sensor_open(sensor, nullptr, opened, &done);
    ssc_sensor_open(sensor, nullptr, opened, &done);
    assert(pending.size() == 2);
    ack(1); ack(0);
    fixture_measurement(client); drain();
    // Original shared-ID code disconnects the second listener, completing one.
    assert(done.calls == 2 && done.successes == 2);
    assert(!subscribed(client));
    fixture_measurement(client); drain();
    assert(done.calls == 2);
    release(sensor, client);
}
static void report_before_ack(bool success)
{
    SSCClient *client;
    auto *sensor = fixture_sensor_new(&client);
    Done done;
    bool gone = false;
    g_object_weak_ref(G_OBJECT(sensor), weak, &gone);
    ssc_sensor_open(sensor, nullptr, opened, &done);
    g_object_unref(sensor);
    assert(!gone);
    fixture_measurement(client); drain();
    assert(done.calls == 1 && done.successes == 1 && !gone);
    ack(0, success);
    assert(done.calls == 1 && gone && !subscribed(client));
    g_object_unref(client);
}
static void failed_send(bool cancel)
{
    SSCClient *client;
    auto *sensor = fixture_sensor_new(&client);
    Done done;
    auto *cancellable = g_cancellable_new();
    ssc_sensor_open(sensor, cancellable, opened, &done);
    if (cancel) g_cancellable_cancel(cancellable);
    ack(0, cancel);
    assert(done.calls == 1 && done.failures == 1 && !subscribed(client));
    fixture_measurement(client); drain();
    assert(done.calls == 1);
    g_object_unref(cancellable);
    release(sensor, client);
}
static void mixed_overlap()
{
    SSCClient *client;
    auto *sensor = fixture_sensor_new(&client);
    Done done;
    for (int i = 0; i < 4; ++i) ssc_sensor_open(sensor, nullptr, opened, &done);
    ack(1, false); ack(2); ack(0);
    fixture_measurement(client); drain();
    assert(done.calls == 4 && done.successes == 3 && done.failures == 1);
    ack(0, false); // An already accepted measurement wins over a late failure.
    release(sensor, client);
}
struct Initialized { Done done; SSCSensor *sensor = nullptr; };
static void initialized(GObject *, GAsyncResult *result, gpointer data)
{
    auto &state = *static_cast<Initialized *>(data);
    GError *error = nullptr;
    state.sensor = ssc_sensor_new_finish(result, &error);
    ++state.done.calls;
    if (state.sensor) { assert(!error); ++state.done.successes; }
    else { assert(error); ++state.done.failures; g_error_free(error); }
}
static void initialization(int failed_stage)
{
    assert(!initializing_client && pending.empty());
    Initialized state;
    ssc_sensor_new(const_cast<gchar *>("proximity"), nullptr, initialized, &state);
    drain();
    assert(initializing_client && pending.size() == 1);
    if (failed_stage == 0) ack(0, false);
    else {
        fixture_discovery(initializing_client, TRUE, TRUE); drain();
        assert(pending.size() == 2);
        if (failed_stage == 1) ack(1, false);
        else {
            fixture_discovery(initializing_client, FALSE, TRUE); drain();
            assert(pending.size() == 3);
            if (failed_stage == 2) ack(2, false);
            else { fixture_attributes(initializing_client); drain(); }
        }
    }
    assert(state.done.calls == 1);
    assert(state.done.successes == unsigned(failed_stage < 0));
    assert(!subscribed(initializing_client));
    while (!pending.empty()) ack(0, false);
    assert(state.done.calls == 1);
    if (state.sensor) g_object_unref(state.sensor);
    g_clear_object(&initializing_client);
    drain();
}
int main()
{
    overlap(); // Same assertion is the mandatory pre-fix negative control.
    for (unsigned i = 0; i < 32; ++i) {
        overlap(); report_before_ack(true); report_before_ack(false);
        failed_send(false); failed_send(true); mixed_overlap();
    }
    for (int stage : {-1, 0, 1, 2}) initialization(stage);
    g_print("Actual parent source: overlapping subscriptions, report/ack orders, errors, cancellation, initialization and object release pass; no hardware accessed\n");
}
