# SSC request lifetime candidate

3 November 2026. This candidate gives each asynchronous libssc request its own
report subscription and keeps its task alive independently through report
delivery and transport acknowledgement. An earlier request can no longer
disconnect a later request's listener. Send failures remove the corresponding
listener and complete the request once, including discovery and attributes.

The COPR recipe retains the exact release22 source input. Native fixtures
compile the complete parent C translation unit and use real GObject, GTask,
signals and generated protobuf code with a simulated transport. They check an
original-source overlapping-request failure, both acknowledgement orders,
failures, cancellation, initialization and object release, normally and with
ASan/UBSan. No sensor hardware is accessed by these fixtures.

COPR11250524 succeeds on fedora-rawhide-aarch64 at source
5159a76732dd97c75e0f84f39fca24055b545c1c. The original-source overlap assertion
fails with134; the corrected actual-source tests pass normally and with
ASan/UBSan, alongside the retained property, parser and measurement tests.
Signed release23 is installed through the standard DNF update; SensorProxy
restarts successfully with the new library. Sustained runtime recurrence
testing is still required before attributing the recorded crash to this
source defect. Packet formats, device calibration, placement and proximity
value conversion are unchanged; the unrecognized vendor event and controlled
physical sensor acceptance remain open.
