# Preserve Fedora sources/lifecycle; add resolver ordering patch and native checks.
/^Release:/ {print "Release:        " release "%{?dist}"; rel++; next}
/^Source10:/ {
 print
 print "Source9001: tailscale-resolver-label_test.go"
 print "Source9998: tailscale-1.104.4-1.fc46.src.rpm"
 print "Source9999: tailscale-dns-build-source.json"
 print "Patch9000: tailscale-direct-prelabel.patch"
 sources++; next
}
/^BuildRequires:[[:space:]]+systemd-rpm-macros$/ {
 print
 print "BuildRequires: golang >= 1.27.3"
 print "BuildRequires: policycoreutils libselinux-utils selinux-policy-targeted patch"
 br++; next
}
/^Requires:[[:space:]]+iproute$/ {print; print "Requires: policycoreutils"; req++; next}
/^%autopatch -p1$/ {
 print
 print "install -m644 %{SOURCE9001} net/dns/uke_resolver_label_test.go"
 print "gofmt -w net/dns/uke_resolver_label_test.go"
 print "cp %{SOURCE9999} tailscale-dns-build-source.json"
 prep++; next
}
/^%build$/ {
 print
 print "export GOTOOLCHAIN=local GOPROXY=off GOSUMDB=off"
 print "go version"
 build++; next
}
/^%if %\{with check\}$/ {print "%check";print; in_check=1; check++; next}
in_check && /^%check$/ {next}
in_check && /^%endif$/ {
 print
 print "export GOTOOLCHAIN=local GOPROXY=off GOSUMDB=off GOFLAGS=-mod=vendor"
 print "test \"$(matchpathcon -n /etc/resolv.conf)\" = \"$(matchpathcon -n /etc/resolv.conf.uke-native.tmp)\""
 print "mapfile -t dns_files < <(go list -mod=vendor -f '{{range .GoFiles}}{{.}}{{\"\\n\"}}{{end}}{{range .CgoFiles}}{{.}}{{\"\\n\"}}{{end}}' ./net/dns | sed '/^$/d;s|^|net/dns/|')"
 print "test ${#dns_files[@]} -gt 1"
 print "printf '%s\\n' \"${dns_files[@]}\" | grep -Fxq net/dns/direct.go"
 print "go test -mod=vendor -vet=off -timeout 120s -v \"${dns_files[@]}\" net/dns/uke_resolver_label_test.go -run '^TestUkeResolver'"
 print "tar -xJOf %{SOURCE0} tailscale-1.104.4/net/dns/direct.go > net/dns/uke_baseline_direct_test.go"
 print "trap 'rm -f net/dns/uke_baseline_direct_test.go' EXIT"
 print "baseline_files=()"
 print "for file in \"${dns_files[@]}\"; do [[ $file != net/dns/direct.go ]] || continue; baseline_files+=(\"$file\"); done"
 print "if go test -mod=vendor -vet=off -timeout 120s -v \"${baseline_files[@]}\" net/dns/uke_baseline_direct_test.go net/dns/uke_resolver_label_test.go -run '^TestUkeResolverLabelBeforePublication$' > uke-baseline-negative.log 2>&1; then cat uke-baseline-negative.log; exit 1; fi"
 print "grep -Fq 'publication ordering:' uke-baseline-negative.log"
 print "grep -Fq 'published-missing-context' uke-baseline-negative.log"
 print "rm -f net/dns/uke_baseline_direct_test.go"
 print "trap - EXIT"
 print "printf '%s\\n' 'Original-source negative control reproduced publication before context; patched native resolver fixtures passed. Synthetic user xattrs, no real SELinux acceptance.'"
 in_check=0; next
}
/^%files$/ {print; print "%doc tailscale-dns-build-source.json"; files++; next}
/^%changelog$/ {
 print
 print "* Sun Nov 01 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.104.4-" release
 print "- Label the complete resolver temporary before atomic publication."
 print "- Retain Fedora source, lifecycle and post-rename/backup behavior."
 print ""
 history++; next
}
{print}
END {if(rel!=1 || sources!=1 || br!=1 || req!=1 || prep!=1 || build!=1 || check!=1 || in_check || files!=1 || history!=1)exit 1}
