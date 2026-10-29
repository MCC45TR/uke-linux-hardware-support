# ddcutil first-line reader correction

29 October 2026. This independent Rawhide AArch64 build retains the Fedora
`ddcutil 3.0.2-1.fc46` source recipe and upstream version. Release `2.uke1`
corrects failed and unterminated first-line reads in the common utility code.
The source input and SHA-256 are recorded in [source.json](source.json).

[COPR build 11228224](https://copr.fedorainfracloud.org/coprs/build/11228224)
passes the native AArch64 regression. All six binary RPMs pass signature and
digest verification against the unstable project key. Their runtime payloads
retain SONAME 5 and contain no Python or regression executable. Device
installation and monitor acceptance remain pending.

`getline` may allocate a buffer before failing. The patched function frees that
buffer and returns null. A successful read removes only an actual newline,
preserving the last character of an unterminated line. The packaged CLI and
shared library use the same corrected source; the existing SONAME is retained.

The COPR check extracts the actual upstream function and tests normal, empty,
unterminated, blank, directory and missing-file input, including repeated
failure. ASan/UBSan checks must pass for the patch, while the original function
must fail the regression assertion. Test tooling remains build-only; no project
test executable, compiler or Python code is installed by these packages.

This correction does not establish successful DDC/CI communication through
Juo JH925. Raw I2C replies remained invalid in the previous tablet trials.
Native COPR builds, signed RPM audits and physical monitor behavior are separate
acceptance gates. Package installation and hardware tests remain deferred to
the coordinated tablet migration and an available display test window.
