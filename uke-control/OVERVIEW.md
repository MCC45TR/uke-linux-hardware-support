# Read-only overview and local evidence

30 October 2026, Control 2.4.0 source. These commands query existing providers;
they never qualify hardware, play/record audio, open a camera stream, mount a
partition, suspend, install a package or run code from an inspected root.

```sh
uke-control --json capabilities
uke-control --json overview battery
uke-control overview save before.json battery
uke-control overview save after.json battery
uke-control --json compare before.json after.json
uke-control support export before.json public-summary.json
uke-control --json roots list
uke-control --json roots inspect PARTUUID
uke-control --json overview --fixture fixture.json
```

The fifteen domains are battery, charge, display, USB, desktop, hardware,
sensors, audio, camera, NFC, sleep, storage, updates, recovery and workload.
Each component has a versioned schema, availability, observation boottime,
collection span and explicit unknown source sample age. A collection is not an
atomic hardware snapshot. Physical acceptance remains null. A missing provider
produces an independent failure; it does not blank unrelated components.
The combined CLI collection is sequential. KCM requests individual domains with
at most three concurrent children, a 15-second query timeout and 1 MiB output
limit. Timeout applies only to these read-only children.

Snapshots/exports use new regular files, mode 0600, no symlink following and
no replacement. JSON input/output is bounded to 8 MiB; duplicate keys and unknown
or duplicate components are refused. Fixtures visibly mark simulation and run
no provider query. They are not a hardware-control capability grant.

Comparison preserves missing versus null and retains both observation times.
It ignores the named observation timestamps, collection durations, `timestamp`
and `pid`; it retains values and validity. It limits nesting to 32 and flattened
leaves to 50,000. Arrays are compared by index, so discovery-order changes may
appear as changes. A difference establishes neither a regression nor causality.

`support export` copies only a fixed application version/family and the known
component availability enum. It excludes observations, free-form strings, logs,
paths and identifiers. The output is deliberately small; no upload occurs.
Review any additional evidence separately before sharing it.

Root discovery uses bounded `lsblk` JSON. Inspection requires one exact partition
UUID and at most one already existing mount, verifies the mounted filesystem's
device number, confines metadata resolution with `openat2`, reads bounded
regular os-release/shell-header files and executes no target program. An
unmounted root is reported without mounting it. Root entry, bind mounts,
namespace execution and repair are intentionally outside this interface until
their ownership, teardown, privilege and untrusted-root contracts are tested.

Use the workload observations for a baseline before changing swap or power
policy. Initial measurement targets are warm-query P95 ≤200 ms (30 samples),
first meaningful KCM card ≤500 ms, and initial available-provider view ≤2 s.
These are targets, not measured results. Record brightness, refresh, wireless,
power source, temperature and SOC; report repeated paired trials and uncertainty.
