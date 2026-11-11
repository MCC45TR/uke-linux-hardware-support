SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
FAMILY ?= uke-core
outdir ?= $(CURDIR)/build/$(FAMILY)
.PHONY: srpm check check-native-migration
srpm:
	bash host/compose.sh '$(FAMILY)' '$(outdir)'
check-native-migration:
	bash tests/native-migration.sh
check:
	bash tests/native-migration.sh
	bash tests/split-spec.sh
	bash tests/filelists.sh
	bash tests/cohort.sh
	bash tests/service-rename.sh
	bash tests/contracts.sh
	bash tests/rawhide.sh
	bash tests/fedora-return-abi.sh
	bash tests/fedora-return-meta.sh
	bash tests/fedora-cxx-runtime.sh
	bash -n tests/gnu-initramfs.sh tests/published-fedora-returns.sh
