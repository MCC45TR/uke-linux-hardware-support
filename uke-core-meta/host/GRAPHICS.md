# Qualified graphics runtime selection

3 November 2026. The retained Core-only recipe requires the OpenCL provider,
its compatible libclc SPIR-V data and `uke-vaapi-v4l2 >= 1.0.0-12.fc46`.
The minimum VA version includes the private encoder staging correction built
in COPR 11251033. The installed backend passed 72 independently decoded H264
frames at three sizes and a byte-exact 30-frame decoder regression on POCO
Pad X1 / Linux 7.3.1-1011. Full codec/profile and sustained qualification is
separate. Normal updates to backend revision 13 additionally deliver its
package-owned GStreamer admission defaults and the original VA plugin.

The recipe retains the signed installed `uke-control-2.8.1-5.2.fc46` source,
including Core audit tools, and emits only `uke-core-meta-1.0.7-19.2.fc46`.
Its source identity is `uke-control-2.8.1-5.3.fc46`; the separate backend/SDK
producer and installed Control 2.8.1-7 are preserved. Source archive, Core
payload and lifecycle sections stay byte-identical.

Run `copr-retained-graphics.sh` only in an immutable reviewed COPR custom source
worker in `mcc45tr/uke-linux-unstable`, `fedora-rawhide-aarch64`. Check the
actual sole binary, signature, dependency delta and normal DNF solver result.
Use DNF for tablet delivery. Future Core selection updates must retain these
three dependencies and the existing audit requirements.
