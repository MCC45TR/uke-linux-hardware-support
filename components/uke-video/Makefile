SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
outdir ?= $(CURDIR)/build/srpm
.PHONY: srpm
# Source input for independent Rawhide builds and retained stable/test cohorts.
srpm:
	bash packaging/source.sh '$(outdir)'
