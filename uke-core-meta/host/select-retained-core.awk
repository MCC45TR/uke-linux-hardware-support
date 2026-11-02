# SPDX-License-Identifier: MIT
# Select the unchanged metadata component of the pinned consolidated source.
# No backend, SDK, power package or new desktop selection is produced.
function fail(message) { print message > "/dev/stderr"; failed=1; exit 1 }
function package_tail( i) {
    for(i=2;i<=NF;i++) if($i=="-n") return $(i+1)
    return "uke-control"
}
function finish_phase() {
    if(body && (depth!=0 || groups!=3)) fail("Unexpected source phase: " body)
    if(body=="check") {
        # Run all retained metadata checks before removing unselected payloads.
        print "rm -f %{buildroot}/usr/share/senemos/uke/desktops/profile.json %{buildroot}/usr/share/senemos/uke/desktops/cutefish.json %{buildroot}/usr/share/senemos/uke/desktops/plasma.json"
        print "rmdir %{buildroot}/usr/share/senemos/uke/desktops"
        print "rm -f %{buildroot}/usr/lib/firewalld/policies/uke-kdeconnect.xml"
        print "rmdir %{buildroot}/usr/lib/firewalld/policies %{buildroot}/usr/lib/firewalld"
        print "rm -rf %{buildroot}/usr/share/plasma/plasmoids/org.senemos.uke.controls"
        print "rmdir %{buildroot}/usr/share/plasma/plasmoids %{buildroot}/usr/share/plasma"
    }
    body=""; depth=0; groups=0
}
BEGIN { keep=1; current="uke-control" }
/^BuildRequires:/ {
    if(!requirements_printed) {
        while((getline requirement < requirements)>0) print requirement
        close(requirements); requirements_printed=1
    }
    next
}
/^Name:/ { if($2!="uke-control") fail("Unexpected source name"); print; next }
/^Source0:/ {
    print
    print "Source1: core-audit-tools-overlay.json"
    print "Source2: uke-control-published-spec.txt"
    next
}
/^%package -n / { current=$3; keep=(current=="uke-core-meta"); if(keep) print; next }
/^Release:/ {
    if(current=="uke-control") {
        if($2!="5.fc46") fail("Unexpected parent source release")
        print "Release: 5.2.fc46"; source_release++
    } else if(keep) {
        if($2!="19.fc46") fail("Unexpected parent Core release")
        print "Release: 19.1.fc46"; core_release++
    }
    next
}
/^Requires: NetworkManager openssh-server sudo$/ && current=="uke-core-meta" {
    print; print "Requires: audit audit-rules strace"; tools++; next
}
/^%description/ { keep=(package_tail()=="uke-control" || package_tail()=="uke-core-meta"); if(keep) print; next }
/^%(prep|build|install|check)([[:space:]]|$)/ {
    finish_phase(); body=substr($1,2); keep=1; print; next
}
/^%changelog$/ {
    finish_phase(); keep=1; print
    print "* Mon Nov 02 2026 Uke Maintainers <75160848+MCC45TR@users.noreply.github.com> - 2.8.1-5.2.fc46"
    print "- Select original Fedora audit collection and bounded native tracing tools."
    print ""
    next
}
/^%(pre|post|preun|postun|pretrans|posttrans|files)([[:space:]]|$)/ {
    finish_phase(); keep=(package_tail()=="uke-core-meta"); if(keep) print; next
}
body {
    text=$0; gsub(/^[[:space:]]+|[[:space:]]+$/, "", text)
    if(text=="(") { if(depth==0) groups++; depth++; if(groups==2) print; next }
    if(text==")") { if(depth<=0) fail("Unbalanced source phase"); if(groups==2) print; depth--; next }
    if(depth==0 || groups==2) print
    next
}
keep { print }
END {
    if(!failed) {
        finish_phase()
        if(source_release!=1 || core_release!=1 || tools!=1) fail("Incomplete Core metadata overlay")
    }
}
