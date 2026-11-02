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
        print "rm -f %{buildroot}/usr/share/senemos/uke/core/profile.json"
        print "rmdir %{buildroot}/usr/share/senemos/uke/core"
        print "rm -f %{buildroot}/usr/share/senemos/uke/desktops/profile.json %{buildroot}/usr/share/senemos/uke/desktops/cutefish.json"
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
    print "Source1: kde-dictionary-overlay.json"
    print "Source2: uke-control-published-spec.txt"
    next
}
/^%package -n / { current=$3; keep=(current=="uke-kde-plasma-meta"); if(keep) print; next }
/^Release:/ {
    if(current=="uke-control") {
        if($2!="5.fc46") fail("Unexpected parent source release")
        print "Release: 5.1.fc46"; source_release++
    } else if(keep) {
        if($2!="35.fc46") fail("Unexpected parent KDE release")
        print "Release: 35.1.fc46"; kde_release++
    }
    next
}
/^Requires: glibc-all-langpacks$/ && current=="uke-kde-plasma-meta" {
    print; print "Requires: hunspell-tr"; dictionary++; next
}
/^%description/ { keep=(package_tail()=="uke-control" || package_tail()=="uke-kde-plasma-meta"); if(keep) print; next }
/^%(prep|build|install|check)([[:space:]]|$)/ {
    finish_phase(); body=substr($1,2); keep=1; print; next
}
/^%(pre|post|preun|postun|pretrans|posttrans|files|changelog)([[:space:]]|$)/ {
    finish_phase(); keep=(package_tail()=="uke-kde-plasma-meta"); if(keep) print; next
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
        if(source_release!=1 || kde_release!=1 || dictionary!=1) fail("Incomplete KDE metadata overlay")
    }
}
