SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: srpm
srpm:
	# Official host tools stay inside COPR's source-generation chroot.
	dnf -y --setopt=install_weak_deps=False install make curl jq tar gzip rpm-build coreutils
	$(MAKE) -f Makefile srpm outdir="$(outdir)"
