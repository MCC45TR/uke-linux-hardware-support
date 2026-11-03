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

Native results are pending. No candidate package is installed. Runtime
recurrence testing is required before attributing the recorded SensorProxy
crash to this source defect. Packet formats, device calibration, placement and
proximity value conversion are unchanged; the unrecognized vendor event and
controlled physical sensor acceptance remain open.
