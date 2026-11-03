# SPDX-License-Identifier: MIT
# Keep signed sources/tests, correct encoder staging and package VA admission.
function fail(message) { print message > "/dev/stderr"; failed=1; exit 1 }
BEGIN { current="source" }
/^%package -n / { current=$3; print; next }
/^Release:/ {
    if(current=="source") {
        if($2!="1%{?dist}") fail("Unexpected integration source release")
        print "Release: 3%{?dist}"; source_release++
    } else if(current=="uke-vaapi-v4l2") {
        if($2!="11.fc46") fail("Unexpected installed backend release")
        print "Release: 13.fc46"; backend_release++
    } else if(current=="uke-video-session") {
        if($2!="2.fc46") fail("Unexpected installed session release")
        print "Release: 4.fc46"; session_release++
    } else fail("Unexpected package release")
    next
}
/^Source0:/ {
    print
    print "Source1: 0008-encoder-cpu-copy-default.patch"
    print "Source2: ENCODER-COPY-FIX.md"
    print "Source3: uke-video-integration-published-spec.txt"
    print "Source4: encoder-copy-overlay.json"
    print "Source5: 60-uke-vaapi.conf"
    print "Source6: uke-vaapi.sh"
    source_inputs++; next
}
/^%build$/ {
    print "("
    print "cd \"%{_builddir}/uke-video-integration-1.0.0/work/uke-vaapi-v4l2/radxa-pkg-libva-v4l2-6d03c3f\""
    print "/usr/bin/patch --no-backup-if-mismatch --fuzz=0 -p1 < %{SOURCE1}"
    print "install -m0644 %{SOURCE2} ENCODER-COPY-FIX.md"
    print ")"
    print ""
    prep_patch++; print; next
}
/^Requires:[[:space:]]+uke-video-session >= 1[.]1[.]0$/ && current=="uke-vaapi-v4l2" {
    print
    print "Requires: gstreamer1-plugins-bad-free"
    va_plugin++; next
}
/^%check$/ {
    print "install -Dm644 %{SOURCE5} %{buildroot}/usr/lib/environment.d/60-uke-vaapi.conf"
    print "install -Dm644 %{SOURCE6} %{buildroot}%{_sysconfdir}/profile.d/uke-vaapi.sh"
    print "mkdir -p %{buildroot}%{_sysconfdir}/xdg/plasma-workspace/env"
    print "ln -s ../../../profile.d/uke-vaapi.sh %{buildroot}%{_sysconfdir}/xdg/plasma-workspace/env/uke-vaapi.sh"
    print ""
    environment_install++; print
    print "sh -n %{SOURCE6}"
    next
}
/^%files -n uke-vaapi-v4l2$/ {
    print
    print "%doc work/uke-vaapi-v4l2/radxa-pkg-libva-v4l2-6d03c3f/ENCODER-COPY-FIX.md"
    print "/usr/lib/environment.d/60-uke-vaapi.conf"
    print "%config(noreplace) %{_sysconfdir}/profile.d/uke-vaapi.sh"
    print "%config(noreplace) %{_sysconfdir}/xdg/plasma-workspace/env/uke-vaapi.sh"
    backend_doc++; next
}
/^%changelog$/ {
    print
    print "* Tue Nov 03 2026 Uke Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-3"
    print "- Deliver GStreamer VA admission through package-owned session environment settings."
    print "- Require the original Fedora modern VA plugin."
    print "- Prefer synchronized CPU copies for private encoder staging."
    print "- Preserve hardware encoding, source archive, dependencies and lifecycle scripts."
    print ""
    next
}
{ print }
END {
    if(!failed && (source_release!=1 || backend_release!=1 || session_release!=1 ||
                  source_inputs!=1 || prep_patch!=1 || backend_doc!=1 ||
                  environment_install!=1 || va_plugin!=1))
        fail("Incomplete retained video overlay")
}
