# Native hardware path diagnostics

Date: 20 October 2026. Native C++ backend; original Fedora desktop applications.

```sh
uke-control hardware status
uke-control test hardware
```

The normal command uses the shared backend; the development package supplies
the test alias. The private `report` collector includes `hardware.json` and
retains an individual failure receipt if metadata collection fails. Raw metadata
does not enter the anonymous GitHub summary.

The bounded snapshot reads existing ALSA, video/media, APR/GPR, SoundWire,
input and LED class metadata. Driver links, endpoint enumeration and input
capability bitmaps are observations, not functional or calibrated acceptance.
Input records decode advertised pen, pressure and tilt bits, retaining null for
missing/malformed bitmaps. An advertised bit does not prove a physical pen event.
ALSA card/PCM identities do not distinguish internal, HDMI, USB and other routes
by themselves. Video nodes can represent cameras or codecs. External webcams
do not prove an internal camera path. Class absence does not prove absent silicon.

An exact compatible allowlist also records selected camera/audio DT nodes.
`reported_status` is the node's own property; `effective_available` checks its
ancestors as well. A missing status property means available under DT rules;
an unreadable or malformed property returns unknown. Effective availability
does not establish driver binding, selected sensor identity or a valid graph.
The WCD USB switch remains a separate stage from the internal WCD codec.

Collection is sequential, with a five-second checked work budget, 256 objects
per class, 32,768 DT entries, 4 KiB per metadata attribute and 1 MiB output.
These bounds do not make an atomic snapshot or a real-time I/O deadline.
Symlink escapes and nonregular attributes are excluded. The fixed allowlist
excludes input unique IDs, serials, register/calibration files, EEPROM, memory
dumps, runtime power controls and vendor calibration-looking getters.

No device node, PCM, camera, input stream, bus service, mixer, sensor power,
module load or firmware operation is opened or requested. There is no new daemon
or dependency. Speaker stimulus and camera/audio capture admission remain false;
physical acceptance remains null. This command supplies the next investigation's
evidence; it cannot admit a speaker or manufacture camera/pen support.
