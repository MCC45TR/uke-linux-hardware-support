# SPDX-License-Identifier: MIT
# Add qualified graphics providers to the retained Core selection only.
function fail(message) { print message > "/dev/stderr"; failed=1; exit 1 }
BEGIN { current="uke-control" }
/^%package -n / { current=$3; print; next }
/^Release:/ {
    if(current=="uke-control") {
        if($2!="5.2.fc46") fail("Unexpected retained source release")
        print "Release: 5.3.fc46"; source_release++
    } else if(current=="uke-core-meta") {
        if($2!="19.1.fc46") fail("Unexpected retained Core release")
        print "Release: 19.2.fc46"; core_release++
    } else fail("Unexpected retained child")
    next
}
/^Source2:/ { print; print "Source3: core-graphics-overlay.json"; source_inputs++; next }
/^Requires: audit audit-rules strace$/ && current=="uke-core-meta" {
    print
    print "Requires: uke-mesa-libOpenCL"
    print "Requires: uke-mesa-libclc22-spirv"
    print "Requires: uke-vaapi-v4l2 >= 1.0.0-12.fc46"
    graphics++; next
}
/^%changelog$/ {
    print
    print "* Tue Nov 03 2026 Uke Maintainers <75160848+MCC45TR@users.noreply.github.com> - 2.8.1-5.3.fc46"
    print "- Require the matched OpenCL provider pair and corrected VA encoder package."
    print "- Preserve the installed Core payload, audit tools and lifecycle scripts."
    print ""
    next
}
{ print }
END {
    if(!failed && (source_release!=1 || core_release!=1 || source_inputs!=1 || graphics!=1))
        fail("Incomplete retained Core graphics overlay")
}
