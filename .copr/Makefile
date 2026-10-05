SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: srpm
# COPR reads this root rule, with cwd set to the configured source subdirectory.
srpm:
	# Git is absent from COPR's initial source chroot. This rule's location is
	# already supplied by COPR, so root discovery needs no additional tool.
	root="$(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)"
	subdir="$$(realpath --relative-to="$$root" .)"
	case "$$subdir" in
		uke-core-meta|uke-desktop-metas|uke-orangefox-packaging|uke-boot) component="$$subdir"; package=;;
		uke-desktop-packaging/packaging/*)
			component=uke-desktop-packaging; package="$${subdir##*/}"
			case "$$package" in material-decoration|plymouth-uke|at-spi2-core|gstreamer1|libaccounts-glib|libwacom|libstdcxx-uke-runtime) ;; *) echo 'Unreviewed or withdrawn source package' >&2; exit 1;; esac;;
		*) echo 'Unreviewed component or source directory' >&2; exit 1;;
	esac
	dnf -y --setopt=install_weak_deps=False install make curl jq xz tar gzip rpm-build coreutils cpio gawk bash kf6-rpm-macros
	stage="$$root/$$component/build/copr-source"
	if test -n "$$package"; then
		$(MAKE) -C "$$root/$$component" srpm PACKAGE="$$package" outdir="$$stage/$$package"
		stage="$$stage/$$package"
	else
		$(MAKE) -C "$$root/$$component" srpm outdir="$$stage"
	fi
	mkdir -p "$(outdir)"
	install -m644 "$$stage/"*.src.rpm "$(outdir)/"
