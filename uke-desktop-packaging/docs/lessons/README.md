# Engineering lessons

| ID | Date | Finding | Validation |
|---|---|---|---|
| UKE-INITIAL-001 | 2026-10-04 | Initial source/packaging scope established from the Nabu COPR inventory | Manifest checks only; package and physical validation open |

## UKE-INITIAL-001

- Date: 2026-10-04.
- Environment: project-owned Git repository; no tablet execution.
- Evidence: component manifest, source policy and acceptance roadmap.
- Finding: Nabu package roles can organize Uke work; their hardware assumptions
  cannot establish Uke behavior.
- Consequence: package admission requires Uke sources and isolated payload tests.
- Uncertainty: no functional package exists at this initial scaffold stage.
- Next validation: complete the manifest's component-specific gates.

| UKE-PACKAGE-002 | 2026-10-05 | Implemented real source/RPM rules from independently scoped Uke inputs | Source validation; target and native results recorded separately |

## UKE-PACKAGE-002

- Date: 2026-10-05.
- Environment: official Fedora host source tools; no tablet execution.
- Evidence: package specs, Make source factory and explicit Uke readiness records.
- Finding: the Nabu package roles are reusable, while firmware, panel, services
  and boot integration require their own Uke evidence. Shared Fedora software
  remains upstream; package data and dependency selections have explicit scope.
- Consequence: only real source families enter automatic COPR compilation.
- Uncertainty: source-generation checks do not establish target closure, native
  binary acceptance, graphical rendering or physical operation.
- Next validation: collect native COPR and signed target transaction results in
  distinct reports, then separately qualify hardware.

## UKE-DESKTOP-003 — COPR locates its source factory at repository root

- Date: 2026-10-05.
- Environment: COPR source-generation mock with two configured subdirectories.
- Evidence: failed source jobs 11075047/11075048 and repeated 11075049/11075050;
  their native source command selected the repository-root `.copr/Makefile` while
  setting its working directory to `packaging/PACKAGE`.
- Finding: a `.copr/Makefile` placed inside each package directory was not selected.
  The ordinary source generation passed locally, but the remote factory stopped
  before an SRPM was collected.
- Consequence: one root factory now validates the working-directory package name,
  dispatches to the ordinary root Makefile, and exports only its mode-0644 SRPM.
  The ineffective nested factory files were removed.
- Uncertainty: the corrected factory still requires its own remote collection
  and binary build results; local source success is not substituted for them.
- Next validation: collect replacement source jobs and audit signed target packages.

## UKE-DESKTOP-004 — current KF6 exports require explicit Qt QML imports

- Date: 2026-10-05.
- Environment: published upstream stable source and native Rawhide AArch64 COPR.
- Evidence: binary build 11075055 stopped in CMake generation because
  `KF6::I18nQml` referenced the undefined `Qt6::QmlIntegration` target.
- Finding: the stable project's Qt Widgets/DBus declaration did not create the
  QML targets exported by the current KF6 I18n toolchain.
- Consequence: an attributed small CMake patch explicitly imports Qt Qml and
  QmlIntegration; the spec adds official qt6-qtdeclarative-devel and increments
  its release. The SRPM retains the patch as an inspectable separate source.
- Uncertainty: successful source preparation alone does not establish the
  corrected C++ binary build, plugin load, dependency closure or rendering.
- Next validation: build the corrected native RPM, inspect its ELF/plugins and
  validate its signed target package transaction.
- Correction evidence: the first local patch trial had an incorrect unified-diff
  hunk count and was rejected by RPM preparation. Correcting the count allowed
  zero-fuzz application. An EOF blank context line then failed publication's
  whitespace check; narrowing the context removed it. Original failed logs and
  the corrected preparation are retained separately.

## UKE-DESKTOP-005 — inspect payloads as well as dependency names

- Date: 2026-10-05.
- Environment: isolated Rawhide AArch64 dependency transaction and official RPM archive audit.
- Evidence: the complete Plasma selection resolved six Python packages; five
  native source families declared the immediate interpreter dependencies.
  An independent 600-RPM file-list audit found two additional Python scripts
  in Dolphin, despite their missing interpreter Requires.
- Consequence: six exact-pinned official source variants remove optional host
  utilities/bindings or replace required migrations with native C++ helpers.
  The core selection refuses the Python ABI and Plasma requires explicit native
  variant capabilities. Neither RPM metadata nor successful compilation alone
  is sufficient target evidence.
- Host evidence: calendar and Dolphin C++ fixtures passed transformations,
  preservation/idempotence and unsafe-input checks. These are host fixtures,
  separate from COPR binary builds and target transactions.
- Correction: inherited Plasma source metadata used a KF6 minimum-version macro
  unavailable in some source factories. Its inspected 6.7.91 value is explicit.
  An inherited nonchronological Fedora changelog is retained in the original
  archive, while the generated candidate records its own change.
- Uncertainty: native rebuilds and the complete corrected runtime transaction
  remain required. Optional Python accessibility/account consumers and Wacom
  helper users are deliberately outside this native-runtime selection.
- Next validation: audit every signed native binary payload and re-run complete
  fresh-install/upgrade/removal with the hard no-Python dependency gate.
- Additional source-factory gate: the minimal compiler image deliberately lacks
  KF6 RPM macros. The shared source factory now installs official kf6-rpm-macros
  before generating KDE source RPMs; this also resolves Dolphin's inherited
  versioned BuildRequires before mock's binary dependency solver runs.

## UKE-DESKTOP-006 — declare the native compiler in a minimal build root

- Date: 2026-10-05.
- Environment: native Rawhide AArch64 COPR binary job 11075173.
- Evidence: source collection passed, then Meson reported `gcc --version` could
  not execute because the compiler was absent. The inherited libwacom spec did
  not declare GCC; the project's minimal worker does not imply Fedora's full
  default build group.
- Consequence: the libwacom variant now explicitly requires GCC and increments
  its candidate release to `1.uke2`. A source/RPM collection result cannot prove
  native dependency closure or compilation.
- Next validation: replacement native compilation and signed runtime audit.
- Superseding diagnosis: inspection of the original source spec showed
  `BuildRequires: meson gcc`. The initial pinning adapter replaced the entire
  multi-dependency line and accidentally removed GCC. The adapter now preserves
  the complete original BuildRequires and adds the exact host pin separately,
  including for GObject-introspection. The earlier statement that the inherited
  libwacom spec omitted GCC was incorrect. This is an adapter defect, not an
  upstream packaging defect. The explicit GCC requirement remains harmless.

## UKE-DESKTOP-007 — audit all generated binary subpackages

- Date: 2026-10-05.
- Environment: native GStreamer source build 11075171, all-subpackage BUILDROOT.
- Evidence: C compilation completed, but the installed-file policy gate rejected
  three GDB Python helpers in the developer package. The original runtime-only
  file-list audit correctly did not include this SDK package; its result was
  insufficient for the entire generated source family.
- Consequence: release `1.uke2` excludes the optional GDB Python helper directory
  and its file-list entries alongside the two installed host documentation
  scanners. Native media, debug logging and plugin scanning remain intact.
- Uncertainty: host GDB's Python convenience layer is outside this native SDK.
- Next validation: inspect every generated binary RPM, then the actual selected
  complete runtime. Neither successful C compilation nor one subpackage suffices.

## UKE-DESKTOP-008 — classify conservative privacy matches accurately

- Date: 2026-10-05.
- Environment: signed Dolphin job 11075175 extracted documentation.
- Evidence: native C++ compilation, signatures and no-Python payload audit passed.
  The conservative privacy scan rejected translated HTML tutorial text; inspected
  English content contains public upstream home-directory notation with a
  sample person name. This is not evidence of a leaked worker or owner identity.
- Consequence: the lean native runtime excludes optional translated HTML
  tutorials, retains UI translations, native help metadata, standard licensing
  and README, and keeps the complete documentation in the pinned source RPM.
  Release `1.uke2` receives this explicit packaging scope change. No broad privacy
  scan exemption is introduced and no source documentation is rewritten.
- Uncertainty: this mobile runtime does not provide Dolphin's offline HTML manual.
- Next validation: re-run signed native payload/privacy and full runtime gates.

## UKE-DESKTOP-009 — audit the inherited base, not only new RPM inputs

- Date: 2026-10-05.
- Environment: pinned Rawhide AArch64 base, offline full desktop replay and host installed-root audit.
- Evidence: all 603 selected RPMs passed signatures and extracted no-Python
  checks. Fresh installation, six capabilities, native AArch64 migration
  fixtures, actual signed meta release-1 to release-2 upgrade and admitted
  package removal passed. The full installed `usr/` and `etc/` audit then failed
  on 15 Python/PYC GDB helper files already owned by the base's
  `libstdc++-16.2.1-2.fc46.1`. Package-name checks cannot detect those helpers.
- Consequence: that complete root is rejected. The official
  `gcc-16.2.1-2.fc46.1.src.rpm`, SHA-256
  `b8f6cc1f055a057233a3b807bf5e9cc2a43836c025ba664b1231c79d86271175`,
  supplies a real standalone native C++ library build. Its target selection
  excludes debugger scripts, preserves standard library ABI and requires all
  6,100 original versioned symbols plus a native C++ smoke fixture.
- Correction: earlier console/kernel package reports checked their own
  payloads and installed package names; those results do not establish that
  the entire inherited Fedora root contains no Python script. Preserve the
  successful bounded checks and this superseding complete-root rejection.
- Uncertainty: the new native source, signatures, replacement/removal of old
  RPM-owned helpers and complete root still require acceptance. A library smoke
  fixture does not establish Plasma startup or physical Uke support.
- Next validation: build the seventh source, require its capability from the
  core meta and repeat complete root, dependency and lifecycle gates.
- Failed native trial: job 11076609 prepared and compiled the runtime, then
  Fedora's default LTO merged intentionally different C++ standard translation
  units and rejected a versioned assembler alias at final linking. The inherited
  full GCC build avoids these generic RPM LTO flags. The standalone spec now
  clears only `_lto_cflags`, retaining other reviewed optimization/hardening
  flags and the ABI/smoke gates. The failure occurred before binary admission.
- Build scheduling correction: individual package specifications use COPR's
  existing subdirectory push hook. The extra Actions workflow now watches only
  shared adapter/source/manifest changes, so a one-family correction does not
  request duplicate healthy KDE builds.

## UKE-DESKTOP-010 — owner policy withdraws KDE application variants

- Date: 2026-10-05.
- Environment: source allowlists, GitHub workflow and COPR Rawhide AArch64.
- Evidence: the owner explicitly forbids cloning KDE desktop applications.
  Plasma/Dolphin automatic builds were disabled and their active jobs 11076607,
  11076665 and 11076667 were canceled following this new incompatible scope.
- Consequence: Make/source factories reject both applications before retrieving
  an archive; the shared workflow requests only five non-KDE source families.
  Existing artifacts are retained as historical evidence and withdrawn from
  the active channel. Original distribution applications remain authoritative.
- Uncertainty: original Plasma/Dolphin contain Python components. A complete
  KDE selection cannot satisfy the current target rule, so its admission stays
  blocked and the desktop metadata does not install a graphical session.
- Next validation: verify source rejection and remote automatic-build settings;
  complete native console/runtime tests independently of KDE.
- Native-library correction: trial 11076668 linked but failed the unchanged
  6,100-symbol baseline because GCC's standalone thread probe lacked the
  generated POSIX header. Preparation now creates that header and requires the
  thread macro before compilation. No ABI baseline is reduced to pass the test.

## UKE-DESKTOP-011 — isolate standalone GNU C++ build headers

- Date: 2026-10-05.
- Environment: native Rawhide AArch64 COPR 11076770 and a local GCC 16.2.1
  x86_64 configure/header/module experiment.
- Evidence: all thread exports returned after the generated POSIX header fix,
  but the strict 6,100-symbol comparison rejected the two module initialization
  exports. The compiler reported missing C fenv declarations; upstream then
  silently substituted empty module objects. Locally, the default installed
  C++ header search reproduced 81 diagnostic lines. `-nostdinc++` with the
  generated build headers produced zero diagnostics, compiled both real
  modules and exported `_ZGIW3std` and `_ZGIW3stdW6compat`.
- Consequence: pass `CXX='g++ -nostdinc++'` only to the library make step;
  configure probes retain their normal host compiler search. Preserve every
  original required symbol and keep empty-object fallbacks inadmissible.
- Uncertainty: the local objects are x86_64 host evidence, not native AArch64
  library or target-root acceptance. KDE application targets remain withdrawn.
- Next validation: native AArch64 compilation, full ABI and smoke gates, signed
  payload audit and a complete isolated console-root lifecycle test.
- Superseding recipe correction: trial 11076948 still failed the two-export
  gate because upstream clears `MAKEOVERRIDES` and did not forward the top-level
  `CXX` override to recursive modules. Pass `-nostdinc++` through `CXXFLAGS`,
  explicitly forwarded by `AM_MAKEFLAGS`. This preserves the observed header
  diagnosis while correcting the ineffective first recipe.
- Native result: trial 11076968 passed the complete unchanged 6,100-symbol
  gate and executed the real native C++ smoke fixture. Packaging then rejected
  an absolute Source3 pathname mixed with relative `%license` entries. Copy
  the exact Boost license into the prepared source tree and use its relative
  name. Preserve native build/ABI/smoke evidence separately from RPM acceptance.
- Accepted correction: native build 11077014 produced the signed GNU C++ RPM
  and complete source RPM. All 6,100 original exports, native smoke, binary
  signatures and extracted Python/privacy gates passed. The independent
  52-input console selection subsequently passed offline fresh installation,
  actual earlier signed metadata upgrade, both complete inherited-root scans
  and removal. The builder's console report records exact identities, the
  explicit reviewed vendor transition and an excluded fixture restart mistake.
  This result does not admit a KDE session or establish device boot.
