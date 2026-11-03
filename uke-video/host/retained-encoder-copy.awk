# SPDX-License-Identifier: MIT
# Keep the signed integration's complete source/tests and change one copy default.
function fail(message) { print message > "/dev/stderr"; failed=1; exit 1 }
BEGIN { current="source" }
/^%package -n / { current=$3; print; next }
/^Release:/ {
    if(current=="source") {
        if($2!="1%{?dist}") fail("Unexpected integration source release")
        print "Release: 2%{?dist}"; source_release++
    } else if(current=="uke-vaapi-v4l2") {
        if($2!="11.fc46") fail("Unexpected installed backend release")
        print "Release: 12.fc46"; backend_release++
    } else if(current=="uke-video-session") {
        if($2!="2.fc46") fail("Unexpected installed session release")
        print "Release: 3.fc46"; session_release++
    } else fail("Unexpected package release")
    next
}
/^Source0:/ {
    print
    print "Source1: 0008-encoder-cpu-copy-default.patch"
    print "Source2: ENCODER-COPY-FIX.md"
    print "Source3: uke-video-integration-published-spec.txt"
    print "Source4: encoder-copy-overlay.json"
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
/^%files -n uke-vaapi-v4l2$/ {
    print
    print "%doc work/uke-vaapi-v4l2/radxa-pkg-libva-v4l2-6d03c3f/ENCODER-COPY-FIX.md"
    backend_doc++; next
}
/^%changelog$/ {
    print
    print "* Tue Nov 03 2026 Uke Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-2"
    print "- Prefer synchronized CPU copies for private encoder staging."
    print "- Preserve hardware encoding, source archive, dependencies and lifecycle scripts."
    print ""
    next
}
{ print }
END {
    if(!failed && (source_release!=1 || backend_release!=1 || session_release!=1 ||
                  source_inputs!=1 || prep_patch!=1 || backend_doc!=1))
        fail("Incomplete retained video overlay")
}
