SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: srpm
# COPR reads this root rule, with cwd set to the configured source subdirectory.
srpm:
	root="$$(git rev-parse --show-toplevel)"
	subdir="$$(realpath --relative-to="$$root" .)"
	case "$$subdir" in
		uke-core-meta|uke-desktop-metas|uke-orangefox-packaging) component="$$subdir"; package=;;
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
