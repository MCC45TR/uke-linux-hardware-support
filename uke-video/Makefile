SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
outdir ?= $(CURDIR)/build/srpm
.PHONY: srpm
# Intermediate input for uke-additional; never a separate COPR source package.
srpm:
	bash packaging/source.sh '$(outdir)'
