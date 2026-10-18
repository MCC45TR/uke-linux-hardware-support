SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
outdir ?= $(CURDIR)/build
component := $(abspath ../..)
commit := 1c1136ebd1e943d6d2ba7c204a11deee3e948dd8
.PHONY: srpm
srpm:
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS}
	archive='$(component)/referances/fastfetch/2.68.1/$(commit).tar.gz'
	mkdir -p "$$(dirname "$$archive")"
	if [[ ! -s $$archive ]]; then
		curl -fsSL 'https://codeload.github.com/fastfetch-cli/fastfetch/tar.gz/$(commit)' -o "$$archive.part"
		mv "$$archive.part" "$$archive"
	fi
	test "$$(sha256sum "$$archive" | cut -d ' ' -f1)" = aaf759881615c9df0aae1c2872562001b58c9c81489579f787ceef65c2743c6c
	cp "$$archive" "$$top/SOURCES/fastfetch-$(commit).tar.gz"
	cp *.patch smbios-fallback-contract.sh source-lock.json "$$top/SOURCES/"
	cp fastfetch.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" "$$top/SPECS/fastfetch.spec"
