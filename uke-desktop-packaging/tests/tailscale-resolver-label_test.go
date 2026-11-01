// SPDX-License-Identifier: MIT
//go:build linux

package dns

import (
	"bytes"
	"fmt"
	"net/netip"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"golang.org/x/sys/unix"
	"tailscale.com/health"
	"tailscale.com/util/eventbus"
)

const ukeTestContext = "user.uke_dns_context"

// The native fixture substitutes external tools, never the production DNS code.
// User xattrs model context metadata; they are not real SELinux acceptance.
func TestMain(m *testing.M) {
	switch filepath.Base(os.Args[0]) {
	case "getenforce":
		fmt.Println("Enforcing")
		os.Exit(0)
	case "systemctl":
		if len(os.Args) == 3 && os.Args[1] == "is-active" { os.Exit(3) }
		os.Exit(99)
	case "restorecon":
		root := os.Getenv("UKE_DNS_TEST_ROOT")
		if root == "" || len(os.Args) != 2 || !strings.HasPrefix(os.Args[1], root+"/etc/") { os.Exit(98) }
		path := os.Args[1]
		stage := "incoming"
		if filepath.Base(path) == "resolv.conf" {
			buf := make([]byte, 64)
			n, err := unix.Getxattr(path, ukeTestContext, buf)
			stage = "published-missing-context"
			if err == nil && string(buf[:n]) == "net_conf_t" { stage = "published-with-context" }
		}
		trace := os.Getenv("UKE_DNS_TEST_TRACE")
		if !strings.HasPrefix(trace, root+string(os.PathSeparator)) { os.Exit(97) }
		f, err := os.OpenFile(trace, os.O_WRONLY|os.O_CREATE|os.O_APPEND, 0600)
		if err != nil { os.Exit(97) }
		fmt.Fprintln(f, stage)
		f.Close()
		if err := unix.Setxattr(path, ukeTestContext, []byte("net_conf_t"), 0); err != nil { os.Exit(96) }
		os.Exit(0)
	}
	bin, err := os.MkdirTemp("", "uke-dns-tools-")
	if err != nil { panic(err) }
	exe, err := os.Executable()
	if err != nil { panic(err) }
	for _, name := range []string{"getenforce", "restorecon", "systemctl"} {
		if err := os.Symlink(exe, filepath.Join(bin, name)); err != nil { panic(err) }
	}
	os.Setenv("PATH", bin+string(os.PathListSeparator)+os.Getenv("PATH"))
	status := m.Run()
	os.RemoveAll(bin)
	os.Exit(status)
}

func ukeContext(t *testing.T, path string) string {
	t.Helper()
	buf := make([]byte, 64)
	n, err := unix.Getxattr(path, ukeTestContext, buf)
	if err != nil { t.Fatalf("context metadata: %v", err) }
	return string(buf[:n])
}

func ukeManager(t *testing.T) (*directManager, directFS, string) {
	t.Helper()
	root := t.TempDir()
	if err := os.Mkdir(filepath.Join(root, "etc"), 0700); err != nil { t.Fatal(err) }
	t.Setenv("UKE_DNS_TEST_ROOT", root)
	trace := filepath.Join(root, "label-events")
	t.Setenv("UKE_DNS_TEST_TRACE", trace)
	fs := directFS{prefix: root}
	if err := fs.WriteFile(resolvConf, []byte("nameserver 192.0.2.53\n"), 0644); err != nil { t.Fatal(err) }
	if err := unix.Setxattr(fs.ActualPath(resolvConf), ukeTestContext, []byte("net_conf_t"), 0); err != nil { t.Fatal(err) }
	m := newDirectManagerOnFS(t.Logf, health.NewTracker(eventbus.New()), nil, fs)
	t.Cleanup(func() { m.ctxClose(); m.trampleTimer.Stop() })
	return m, fs, trace
}

func TestUkeResolverLabelBeforePublication(t *testing.T) {
	m, fs, trace := ukeManager(t)
	if err := m.atomicWriteFile(fs, resolvConf, []byte("new DNS configuration\n"), 0644); err != nil { t.Fatal(err) }
	events, err := os.ReadFile(trace); if err != nil { t.Fatal(err) }
	if string(events) != "incoming\npublished-with-context\n" { t.Fatalf("publication ordering: %q", events) }
	data, err := fs.ReadFile(resolvConf); if err != nil || string(data) != "new DNS configuration\n" { t.Fatal("new resolver not committed") }
	if ukeContext(t, fs.ActualPath(resolvConf)) != "net_conf_t" { t.Fatal("context metadata did not survive atomic rename") }
	st, err := os.Stat(fs.ActualPath(resolvConf)); if err != nil || st.Mode().Perm() != 0644 { t.Fatal("resolver mode changed") }
	if err := m.atomicWriteFile(fs, "/etc/other.conf", []byte("other"), 0644); err != nil { t.Fatal(err) }
	after, err := os.ReadFile(trace); if err != nil || !bytes.Equal(events, after) { t.Fatal("unrelated file was relabeled") }
}

func TestUkeResolverBackupRetainsContext(t *testing.T) {
	m, fs, trace := ukeManager(t)
	original, err := fs.ReadFile(resolvConf); if err != nil { t.Fatal(err) }
	inode, err := os.Stat(fs.ActualPath(resolvConf)); if err != nil { t.Fatal(err) }
	if err := m.SetDNS(OSConfig{Nameservers: []netip.Addr{netip.MustParseAddr("192.0.2.54")}}); err != nil { t.Fatal(err) }
	if ukeContext(t, fs.ActualPath(backupConf)) != "net_conf_t" { t.Fatal("backup inode context changed") }
	if err := m.SetDNS(OSConfig{}); err != nil { t.Fatal(err) }
	restored, err := fs.ReadFile(resolvConf); if err != nil || !bytes.Equal(original, restored) { t.Fatal("original resolver not restored") }
	st, err := os.Stat(fs.ActualPath(resolvConf)); if err != nil || !os.SameFile(inode, st) { t.Fatal("original backup inode replaced") }
	events, err := os.ReadFile(trace); if err != nil || strings.Contains(string(events), "missing") { t.Fatalf("backup context publication: %q, %v", events, err) }
}

type ukeBoundResolverFS struct { directFS }

func (fs ukeBoundResolverFS) Rename(old, new string) error {
	if old == resolvConf || new == resolvConf { return fmt.Errorf("fixture bind mount cannot be renamed") }
	return fs.directFS.Rename(old, new)
}

func (fs ukeBoundResolverFS) Remove(path string) error {
	if path == resolvConf { return fmt.Errorf("fixture bind mount cannot be removed") }
	return fs.directFS.Remove(path)
}

func TestUkeResolverBoundFileFallback(t *testing.T) {
	m, fs, trace := ukeManager(t)
	m.fs = ukeBoundResolverFS{fs}
	original, err := fs.ReadFile(resolvConf); if err != nil { t.Fatal(err) }
	if err := m.SetDNS(OSConfig{Nameservers: []netip.Addr{netip.MustParseAddr("192.0.2.55")}}); err != nil { t.Fatal(err) }
	if !m.renameBroken || ukeContext(t, fs.ActualPath(resolvConf)) != "net_conf_t" { t.Fatal("bind fallback changed context metadata") }
	if err := m.SetDNS(OSConfig{}); err != nil { t.Fatal(err) }
	restored, err := fs.ReadFile(resolvConf); if err != nil || !bytes.Equal(original, restored) { t.Fatal("bound resolver was not restored") }
	events, err := os.ReadFile(trace); if err != nil || strings.Contains(string(events), "missing") { t.Fatalf("bound publication: %q, %v", events, err) }
}
