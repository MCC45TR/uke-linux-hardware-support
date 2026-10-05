SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: srpm
srpm:
	# Official host tools stay inside COPR's source-generation chroot.
	dnf -y --setopt=install_weak_deps=False install make curl jq tar gzip rpm-build coreutils
	# Export only SRPMs; internal source/payload directories are not results.
	$(MAKE) -f Makefile srpm outdir="$(CURDIR)/build/copr-source"
	mkdir -p "$(outdir)"
	install -m644 build/copr-source/*.src.rpm "$(outdir)/"
