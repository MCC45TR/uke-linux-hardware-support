# Initial Uke roadmap

1. Inventory the named Nabu reference families and pin relevant upstream sources.
2. Establish Uke-specific interfaces, licenses and firmware profile boundaries.
3. Implement the intended packages and complete these gates:

- Record exact source/license/hash for every file.
- Do not publish OEM blobs without file-level redistribution terms.
- Never publish unit calibration, persist data or security firmware.

4. Build and audit AArch64 RPM/SRPM payloads and their complete dependency closure.
5. Test installation, upgrade and removal in an isolated target environment.
6. Publish accepted development candidates to COPR, then collect distinct physical
   results using a preserved stock/recovery route.

The initial repository validates structure and intent only. It claims no build,
boot, peripheral or physical acceptance.
