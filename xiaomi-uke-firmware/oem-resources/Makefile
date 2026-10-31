SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: srpm
srpm:
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1793404800 --owner=0 --group=0 --numeric-owner --transform='s#^#uke-oem-resources-1.0.0/#' -cJf "$$top/SOURCES/uke-oem-resources-1.0.0.tar.xz" src tests packaging README.md LICENSE Makefile
	cp packaging/uke-oem-resources.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --define "_topdir $$top" "$$top/SPECS/uke-oem-resources.spec"
