# Native implementation

`battery/` contains the original C++ read-only collector, read-only D-Bus service
and shared health/energy model sessions. The collector calls the models, with
production qualification flags false unless matching per-boot pack/firmware
evidence is accepted by `profile.cpp`. It has no charging writer.
See `docs/BATTERY.md`; no build or test was run during preparation.
