# UKE-DISPLAY-INPUTS-20261117: Preserve existing OEM update inputs

On17 November2026, the fresh POCO Pad X1 installation runs kernel7.3.4-4
with super repurposed for swap and rescue. The existing kernel updater still
requires ODM color-profile and video input acquisition before staging a new
kernel. Its Control26 trigger correctly refused full5 before any boot write.

Public OEM supplier data at the pinned Uke vendor commit exactly matches both
existing admitted QDCM hashes. The common vendor's current video file does not
match either admitted hash and was rejected as a source candidate. Its earlier
bf664e7e revision contains the exact existing3a4cac66 firmware. The source
manifest pins repository, full revision, path, size, digest and destination for
all three inputs. No new firmware hash or ELF/layout admission is introduced.

Firmware18 extends the existing package with these generic, read-only inputs;
the complete firmware17 recipe and all eight parent inputs remain unchanged.
The public JSON is supplier design data, not a fitted-unit EEPROM or persist
dump. Derived private profile output remains private and versioned. The video
source resides under a separate factory-input path and cannot start a driver
or remote processor merely by being installed. Existing OEM licensing and
notices remain; complete file-level redistribution review is not claimed.

The updater must retain exact board/supplier/hash checks, legacy stock-source
fallback, protected output, signed RPM ownership and inactive-slot rollback.
Source folding, native RPM checks, signed delivery and physical gray-screen
acceptance are separate gates. No completed camera or optical fix is claimed.
