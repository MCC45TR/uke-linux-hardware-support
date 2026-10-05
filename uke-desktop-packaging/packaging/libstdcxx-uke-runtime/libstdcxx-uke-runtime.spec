%global debug_package %{nil}
%global _lto_cflags %{nil}
%global source_date 20260819
Name: libstdcxx-uke-runtime
Version: 16.2.1
Release: 2.uke1%{?dist}.1
Summary: Native GNU C++ runtime source build for the Uke package selection
License: GPL-3.0-or-later AND LGPL-3.0-or-later AND (GPL-3.0-or-later WITH GCC-exception-3.1) AND (GPL-3.0-or-later WITH Texinfo-exception) AND (LGPL-2.1-or-later WITH GCC-exception-2.0) AND (GPL-2.0-or-later WITH GCC-exception-2.0) AND (GPL-2.0-or-later WITH GNU-compiler-exception) AND BSL-1.0 AND GFDL-1.3-or-later AND Linux-man-pages-copyleft-2-para AND SunPro AND BSD-1-Clause AND BSD-2-Clause AND BSD-2-Clause-Views AND BSD-3-Clause AND BSD-4-Clause AND BSD-Source-Code AND Zlib AND MIT AND Apache-2.0 AND (Apache-2.0 WITH LLVM-Exception) AND ZPL-2.1 AND ISC AND LicenseRef-Fedora-Public-Domain AND HP-1986 AND curl AND Martin-Birgmeier AND HPND-Markus-Kuhn AND dtoa AND SMLNJ AND AMD-newlib AND OAR AND HPND-merchantability-variant AND HPND-Intel
URL: https://gcc.gnu.org/onlinedocs/libstdc++/
Source0: gcc-%{version}-%{source_date}.tar.xz
Source1: runtime-smoke.cpp
Source2: fedora-exported-symbols.txt
Source3: LICENSE.Boost
Source99: gcc-16.2.1-2.fc46.1.src.rpm
Patch4: gcc16-libtool-no-rpath.patch
ExclusiveArch: aarch64
BuildRequires: gcc-c++ = 16.2.1-2.fc46.1
BuildRequires: python3 = 3.15.0~rc2-1.fc46
BuildRequires: make binutils glibc-devel tzdata
%description
Complete reviewed GCC source and build rules for a standalone native libstdc++.
The binary subpackage retains the GNU C++ ABI and excludes optional Python GDB
pretty-printers. No compiler or Python tool is installed on the tablet.
%package -n libstdc++
Summary: GNU Standard C++ Library without optional Python GDB helpers
Provides: senemos-native-runtime(libstdc++) = %{version}-%{release}
Requires: glibc >= 2.10.90-7
Recommends: tzdata >= 2017c
%description -n libstdc++
The native GNU Standard C++ Library, rebuilt from the exact Fedora GCC source.
Optional GDB Python printers are outside this target runtime. The source build
checks the original versioned symbol set and executes a native C++ smoke test.
%prep
%setup -q -n gcc-%{version}-%{source_date}
# The inherited patch changes the shared libtool infrastructure used here.
# Remaining Fedora patches affect compilers, other languages or HTML manuals;
# the complete original source RPM is retained as Source99 for provenance.
%patch -P 4 -p0
# GCC's top-level build normally creates this target threading header.
cp libgcc/gthr-posix.h libgcc/gthr-default.h
cp %{SOURCE3} LICENSE.Boost
sed -n '1,39p' libstdc++-v3/src/c++17/fast_float/fast_float.h > NOTICE.fast-float
sed -n '1,17p' libstdc++-v3/src/c++17/ryu/d2s.c > NOTICE.ryu
sed -n '1,/^#ifndef/p' libbacktrace/backtrace.h | sed '$d' > NOTICE.libbacktrace
%build
mkdir -p senemos-libstdcxx-build
cd senemos-libstdcxx-build
export CC=gcc CXX=g++
export CFLAGS="%{build_cflags}" CXXFLAGS="%{build_cxxflags}" LDFLAGS="%{build_ldflags}"
../libstdc++-v3/configure --prefix=%{_prefix} --libdir=%{_libdir} \
  --build=%{_build} --host=%{_host} --disable-multilib \
  --enable-shared --enable-libstdcxx-threads=yes --enable-libstdcxx-backtrace \
  --with-libstdcxx-zoneinfo=%{_datadir}/zoneinfo --disable-libstdcxx-pch
grep -Fx '#define _GLIBCXX_HAS_GTHREADS 1' config.h
# A standalone installed compiler would otherwise search its own C++ headers
# after these generated headers. Duplicate fenv.h include guards suppress the
# system C declarations and make upstream silently replace both std modules
# with empty objects. Configure probes use the host compiler normally; only
# the actual library build isolates its C++ header search.
# Upstream clears MAKEOVERRIDES and forwards CXXFLAGS explicitly. Passing only
# CXX at the top level would lose this isolation in recursive module builds.
%make_build CXXFLAGS="$CXXFLAGS -nostdinc++"
%check
cd senemos-libstdcxx-build
nm -D --defined-only src/.libs/libstdc++.so.6.0.36 | awk '{print $3}' | grep '@' | LC_ALL=C sort -u > native-symbols.txt
LC_ALL=C comm -23 %{SOURCE2} native-symbols.txt > missing-symbols.txt
test ! -s missing-symbols.txt || { cat missing-symbols.txt; exit 1; }
g++ -std=c++23 -O2 %{SOURCE1} -L"$PWD/src/.libs" -pthread -o runtime-smoke
LD_LIBRARY_PATH="$PWD/src/.libs" ./runtime-smoke
%install
# Select the runtime library directly from the real source build. Header,
# static-library, documentation and Python debugger targets are not installed.
install -Dm755 senemos-libstdcxx-build/src/.libs/libstdc++.so.6.0.36 %{buildroot}%{_libdir}/libstdc++.so.6.0.36
ln -s libstdc++.so.6.0.36 %{buildroot}%{_libdir}/libstdc++.so.6
if find %{buildroot} -type f \( -name '*.py' -o -name '*.pyc' -o -name '*.pyo' \) -print | grep .; then exit 1; fi
%files -n libstdc++
%license COPYING3 COPYING.RUNTIME NOTICE.fast-float NOTICE.ryu NOTICE.libbacktrace LICENSE.Boost
%{_libdir}/libstdc++.so.6
%{_libdir}/libstdc++.so.6.0.36
%changelog
* Mon Oct 05 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 16.2.1-2.uke1
- Rebuild the native runtime without Python debugger helpers and require ABI/smoke checks.
