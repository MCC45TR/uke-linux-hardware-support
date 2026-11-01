# Canonical generic speaker-preset inputs

These are the exact two data inputs already published in the qualified speaker
source package, imported from source-view commit
482f7d1170eeb8d8ffce38e53ec5173bb6890e0f. The preset SHA256 is
f319e71d60ed19d539296665fa06cc07fbcf93bf82f773c787bed65289129e7b;
the unmodified original ODM notice SHA256 is
71aef2c9cb076992714858bd3861590e7329c82098501c19a7075b00c9540727.

The owning component's packaging/speaker-firmware/provenance.json and Apache
license retain the exact notice mapping and source release. This admits only
the generic fs19xx.fsm preset, not other OEM firmware or private calibration.
Fedora's speaker-srpm target and the Debian main snapshot select these same
bytes. Neither source packaging nor a successful build proves hardware support.
