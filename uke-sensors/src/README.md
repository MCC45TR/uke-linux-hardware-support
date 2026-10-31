# Development source

Place original native implementation or reviewed upstream development here.
No Nabu source or compiled payload is implicitly accepted for Uke.

31 October 2026: the SSC cache can consume the optional OEM-derived defaults
under /run/uke-oem-resources/selected. It requires private owned directories and
byte-identical complete configuration/helper inputs against the current package
on every invocation. Missing, changed, unsafe or unreadable references select the
existing packaged inputs. The same reader lock and atomic RAM publication remain
in force; read-only persist calibration is neither replaced nor corrected again.
