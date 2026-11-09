# Session and boot-acceptance fixes

9 November 2026. Control215 keeps the qualified214 source cohort, three existing
package names, mandatory matching TUI, speaker-runtime ownership and original
Fedora audio/desktop packages. It introduces no runtime service or sensor poll.

After the kernel11 reboot, the native health window passed but acceptance
stopped with `Durable GPT backup differs from current metadata`. The existing
acceptance filename hashes only partition entries. Valid GPT headers can differ
while both arrays remain identical, so an older durable record can use the same
name. The corrected key hashes the exact observed MBR, both headers and entries,
the same bytes the backup function saves. Existing records are not overwritten.
CRC, array equality, disk identity, geometry, compare-before-commit, verified
writeback and restore checks are unchanged. Host fixtures cover the same-entry,
different-header case, repeatability and refusal to overwrite a different record.
This is not authorization for arbitrary GPT repair or partition resizing.

The queued OSS brightness request also remained pending. The real KWin file
changed normal brightness and external DDC/HDR fields after queueing, while the
internal brightness curve was unchanged. Requiring whole-file equality prevents
normal shutdown updates from completing an import. The corrected pending path
checks the original curve and panel identity, then merges the requested curve
into the latest document. Learned curves, identity changes, concurrent writes
and a running compositor still refuse the transition. Restore and interrupted
completion follow the same rules. Startup diagnostics are journaled directly,
because original Plasma consumes environment-script stdout and discards stderr.

Kernel11/B and the retained9/A raw payload were identified independently; that
does not prove metadata/fallback coherence or physical sound/cover/brightness.
Source, native tests, signed RPM/DNF transition and actual boot/session results
must be reported separately. Source tests do not establish speaker gain243,
encrypted boot, sensor comfort, USB stability or surround-profile support.
