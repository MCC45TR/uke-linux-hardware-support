SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: srpm
# COPR locates this file at the repository root, then sets cwd to subdirectory.
package := $(notdir $(CURDIR))
srpm:
	case '$(package)' in material-decoration|plymouth-uke|at-spi2-core|gstreamer1|libaccounts-glib|libwacom|libstdcxx-uke-runtime) ;; *) echo 'Unsupported or withdrawn COPR package subdirectory' >&2; exit 1;; esac
	dnf -y --setopt=install_weak_deps=False install make curl jq xz tar rpm-build coreutils cpio gawk bash kf6-rpm-macros
	$(MAKE) -C ../.. srpm PACKAGE=$(package) outdir="$(CURDIR)/build/copr-source"
	mkdir -p "$(outdir)"
	install -m644 build/copr-source/*.src.rpm "$(outdir)/"
