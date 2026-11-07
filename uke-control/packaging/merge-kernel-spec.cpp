// SPDX-License-Identifier: MIT
// COPR source-worker tool: compose retained source recipes, never binary RPMs.
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

struct Section { std::string kind, marker; std::vector<std::string> lines; };
using Recipe = std::vector<Section>;
static void require(bool ok, const std::string &why) { if (!ok) throw std::runtime_error(why); }
static Recipe read_recipe(const char *path) {
    std::ifstream input(path);
    require(bool(input), "Cannot read retained recipe");
    Recipe result{{"header", "", {}}};
    const std::regex boundary(R"(^%(package|description|prep|build|install|check|clean|changelog|files|pre|post|preun|postun|pretrans|posttrans|transfiletriggerin|transfiletriggerun|filetriggerin|filetriggerun)([ \t].*|$))");
    std::string line;
    std::size_t size = 0;
    while (std::getline(input, line)) {
        size += line.size();
        require(size <= 4 * 1024 * 1024 && line.find('\0') == std::string::npos, "Invalid retained recipe size/text");
        std::smatch match;
        if (std::regex_match(line, match, boundary)) result.push_back({match[1], line, {}});
        else result.back().lines.push_back(line);
    }
    return result;
}
static std::string package(const Section &section, const std::string &primary) {
    std::istringstream words(section.marker);
    std::string kind, option, name;
    words >> kind >> option;
    if (option == "-n") { words >> name; require(!name.empty(), "Missing section package"); return name; }
    if (option.empty() || option == "--") return primary;
    throw std::runtime_error("Unreviewed section marker: " + section.marker);
}
static bool metadata(const std::string &line) {
    return std::regex_search(line, std::regex(R"(^(Requires(?:\([^)]*\))?|Provides|Obsoletes|Conflicts|Recommends|Suggests|Supplements|Enhances):)"));
}
static std::string updated(std::string line) {
    for (const std::string old : {"2.8.1-26%{?dist}", "2.8.1-26.fc46"}) {
        for (std::size_t p = 0; (p = line.find(old, p)) != std::string::npos;) {
            const std::string replacement = "2.9.0-2%{?dist}";
            line.replace(p, old.size(), replacement); p += replacement.size();
        }
    }
    return line;
}
static std::string body(const std::vector<std::string> &lines) {
    std::string text;
    for (const auto &line : lines) text += line + '\n';
    return text;
}
int main(int argc, char **argv) {
    try {
        require(argc == 4, "Usage: merge-kernel-spec CONTROL26.spec BOOT102.spec COLOR07.spec");
        const auto control = read_recipe(argv[1]), boot = read_recipe(argv[2]), color = read_recipe(argv[3]);
        const std::set<std::string> folded_control{"uke-control", "uke-control-client", "uke-control-tests"};
        const std::set<std::string> folded_boot{"uke-boot-integration", "uke-kernel-update", "uke-installation-profile"};
        const std::set<std::string> optional_boot{"uke-esp32-cdc", "uke-cdc-debug", "uke-boot-recovery"};
        const std::string sdk = "uke-control-client-devel";
        const std::string tui = "uke-control-tui";
        std::vector<std::string> header, main_files, sdk_header, sdk_files, sdk_description;
        std::vector<std::string> tui_header, tui_files, tui_description;
        std::map<std::string, std::vector<std::string>> phases, callbacks;
        std::set<std::string> unit_names, excluded_files;
        std::vector<std::string> changelog;
        std::vector<std::string> staged_documents;
        unsigned headers = 0, setups = 0, trigger = 0;
        for (const auto &[recipe, primary, is_boot] : std::vector<std::tuple<Recipe, std::string, bool>>{{control,"uke-control",false},{boot,"uke-boot-integration",true}}) {
            const auto &folded = is_boot ? folded_boot : folded_control;
            for (const auto &section : recipe) {
                if (section.kind == "header") {
                    ++headers;
                    for (auto line : section.lines) {
                        if (is_boot) {
                            if (line == "Requires: /usr/lib/systemd/systemd-growfs") header.push_back("Requires: systemd-udev");
                            else if (metadata(line) || line.starts_with("BuildRequires:")) header.push_back(line);
                        } else if (line.starts_with("Version:")) header.push_back("Version: 2.9.0");
                        else if (line.starts_with("Release:")) header.push_back("Release: 2%{?dist}");
                        else if (line.starts_with("License:")) header.push_back("License: MIT AND Apache-2.0 AND GPL-3.0-or-later");
                        else if (line.starts_with("SourceLicense:")) header.push_back("SourceLicense: MIT AND Apache-2.0 AND GPL-3.0-or-later AND GPL-2.0-only");
                        else header.push_back(metadata(line) ? updated(line) : line);
                    }
                } else if (section.kind == "package") {
                    const auto name = package(section, primary);
                    require(folded.contains(name) || name == sdk || name == tui || (is_boot && optional_boot.contains(name)), "Unexpected retained binary package: " + name);
                    if (name == sdk || name == tui) {
                        auto &target = name == sdk ? sdk_header : tui_header;
                        for (const auto &line : section.lines) {
                            if (line.starts_with("Version:")) target.push_back("Version: 2.9.0");
                            else if (line.starts_with("Release:")) target.push_back("Release: 2%{?dist}");
                            else target.push_back(updated(line));
                        }
                    } else if (folded.contains(name)) {
                        for (const auto &line : section.lines) if (metadata(line)) {
                            if (is_boot && line == "Requires: /usr/lib/systemd/systemd-growfs") header.push_back("Requires: systemd-udev");
                            else header.push_back(is_boot ? line : updated(line));
                        }
                    }
                } else if (section.kind == "description") {
                    if (package(section, primary) == sdk) sdk_description = section.lines;
                    if (package(section, primary) == tui) tui_description = section.lines;
                } else if (section.kind == "files") {
                    const auto name = package(section, primary);
                    if (folded.contains(name) || name == sdk || name == tui) {
                        auto &files = name == sdk ? sdk_files : name == tui ? tui_files : main_files;
                        for (auto line : section.lines) {
                            if (is_boot && (line.starts_with("%license ") || line.starts_with("%doc "))) {
                                const auto space = line.find(' ');
                                const auto relative = line.substr(space + 1);
                                require(relative.starts_with("work/uke-boot-integration/uke-boot-integration-1.8.102/") &&
                                        std::regex_match(relative, std::regex(R"(^[a-zA-Z0-9/_+.-]+$)")), "Unreviewed boot document");
                                const auto basename = relative.substr(relative.rfind('/') + 1);
                                const auto destination = "BOOT-RUNTIME-" + basename;
                                const auto absolute = "%{_builddir}/uke-boot-integration-1.8.102/" + relative;
                                staged_documents.push_back("cp '" + absolute + "' '" + destination + "'");
                                line = line.substr(0, space + 1) + destination;
                            }
                            files.push_back(line);
                        }
                    } else {
                        require(is_boot && optional_boot.contains(name), "Unexpected discarded package");
                        for (const auto &line : section.lines) {
                            if (line.empty() || line.front() == '#' || line.starts_with("%license ") || line.starts_with("%doc ") || line.starts_with("%dir ")) continue;
                            require(std::regex_match(line, std::regex(R"(^/[a-zA-Z0-9/_+.-]+$)")), "Review optional payload path: " + line);
                            excluded_files.insert(line);
                        }
                    }
                } else if (section.kind == "prep" || section.kind == "build" || section.kind == "install" || section.kind == "check") {
                    auto lines = section.lines;
                    if (section.kind == "prep") {
                        for (auto &line : lines) if (line == "%setup -q") {
                            ++setups;
                            line = is_boot ? "tar -xJf %{SOURCE9} -C %{_builddir}" : "%setup -q -n uke-control-2.8.1";
                        }
                    }
                    phases[section.kind].push_back(body(lines));
                } else if (section.kind == "changelog") {
                    if (!is_boot) changelog = section.lines;
                } else {
                    const auto name = package(section, primary);
                    if (!folded.contains(name)) { require(is_boot && optional_boot.contains(name), "Unknown lifecycle owner"); continue; }
                    std::string marker = "%" + section.kind + " -n uke-control";
                    if (section.kind.starts_with("transfiletrigger") || section.kind.starts_with("filetrigger")) {
                        const auto at = section.marker.find(" -- "); require(at != std::string::npos, "Missing file trigger path");
                        marker += section.marker.substr(at); ++trigger;
                    }
                    for (const auto &line : section.lines) if (is_boot && line.find("remove-system-units") != std::string::npos) {
                        std::istringstream words(line); std::string word;
                        while (words >> word) if (word.ends_with(".service") || word.ends_with(".timer")) {
                            require(std::regex_match(word, std::regex(R"(^[a-zA-Z0-9@_.-]+[.](service|timer)$)")), "Invalid migration unit");
                            unit_names.insert(word);
                        }
                    }
                    callbacks[marker].push_back(body(section.lines));
                }
            }
        }
        require(headers == 2 && setups == 2 && trigger == 1 && !sdk_header.empty() && !sdk_files.empty(), "Retained recipe structure changed");
        require(!tui_header.empty() && !tui_files.empty(), "Missing mandatory terminal package");
        require(!main_files.empty() && !excluded_files.empty() && !unit_names.empty(), "Missing retained payload or migration units");
        // The original color helper remains a native, nonresident provisioning
        // command. Keep all source tests and licenses while transferring ownership.
        const std::string color_input = "%{_builddir}/uke-control-2.8.1/o82-source/";
        const std::string color_build = "%{_builddir}/uke-control-2.8.1/o82-build/";
        std::map<std::string, std::string> color_sources;
        for (const auto &section : color) if (section.kind == "header") {
            for (const auto &line : section.lines) {
                std::smatch match;
                if (std::regex_match(line, match, std::regex(R"(^Source([0-9]+):[ \t]+([a-zA-Z0-9_.+-]+)$)")))
                    color_sources[match[1]] = color_input + match[2].str();
                else if (metadata(line) || line.starts_with("BuildRequires:")) header.push_back(line);
            }
        }
        require(color_sources.size() == 32, "Retained color source inventory changed");
        const auto color_line = [&](std::string line) {
            const std::regex source(R"(%\{SOURCE([0-9]+)\})");
            std::smatch match;
            while (std::regex_search(line, match, source)) {
                require(color_sources.contains(match[1]), "Missing color source reference");
                line.replace(static_cast<std::size_t>(match.position()), static_cast<std::size_t>(match.length()), color_sources.at(match[1]));
            }
            return line;
        };
        for (const auto &section : color) {
            if (section.kind == "prep" || section.kind == "build" || section.kind == "install" || section.kind == "check") {
                std::string part = "(\nmkdir -p '" + color_build + "'\ncd '" + color_build + "'\n";
                for (const auto &line : section.lines) part += color_line(line) + '\n';
                part += ")\n";
                phases[section.kind].push_back(part);
            } else if (section.kind == "files") {
                for (const auto &line : section.lines) {
                    if (line.starts_with("%license ")) {
                        require(line == "%license LICENSE", "Unexpected color license path");
                        staged_documents.push_back("cp '" + color_build + "LICENSE' O82-COLOR-LICENSE");
                        main_files.push_back("%license O82-COLOR-LICENSE");
                    } else if (line.starts_with("%doc ")) {
                        require(line == "%doc O82-COLOR-PROFILE.md", "Unexpected color document path");
                        staged_documents.push_back("cp '" + color_build + "O82-COLOR-PROFILE.md' O82-COLOR-PROFILE.md");
                        main_files.push_back("%doc O82-COLOR-PROFILE.md");
                    } else main_files.push_back(line);
                }
            } else require(section.kind == "header" || section.kind == "description" || section.kind == "changelog", "Unexpected color lifecycle section");
        }
        // The archive is extracted before its original %prep runs. The reviewed
        // parser overlay follows all retained prep phases, before native builds.
        phases["prep"].insert(phases["prep"].end() - 1, "mkdir -p '" + color_input + "'\ntar -xJf %{SOURCE19} -C '" + color_input + "'\n");
        header.push_back("Source9: uke-boot-integration-1.8.102.tar.xz");
        header.push_back("Source10: 60-uke-filelists.conf");
        header.push_back("Source11: control-version.hpp");
        header.push_back("Source12: merge-kernel-spec.cpp");
        header.push_back("Source13: copr-control-kernel.sh");
        header.push_back("Source14: KERNEL-CONSOLIDATION.md");
        header.push_back("Source15: kernel-consolidation.sh");
        header.push_back("Source16: control-consolidated-spec.txt");
        header.push_back("Source17: unstable-release.hpp");
        header.push_back("Source18: runtime733-version.patch");
        header.push_back("Source19: retained-o82-color-0.7.0.tar.xz");
        header.push_back("Source20: unstable-release.cpp");
        header.push_back("BuildRequires: patch");
        header.push_back("Requires: uke-control-tui%{?_isa} = 2.9.0-2%{?dist}");
        header.push_back("Provides: uke-o82-color-profile = 0.7.0-2%{?dist}");
        header.push_back("Provides: uke-o82-color-profile%{?_isa} = 0.7.0-2%{?dist}");
        header.push_back("Obsoletes: uke-o82-color-profile <= 0.7.0-2%{?dist}");
        for (const auto &name : folded_boot) {
            header.push_back("Provides: " + name + " = 1.8.102-1%{?dist}");
            header.push_back("Provides: " + name + "%{?_isa} = 1.8.102-1%{?dist}");
            header.push_back("Obsoletes: " + name + " <= 1.8.102-1%{?dist}");
        }
        for (const auto &name : folded_control) if (name != "uke-control") {
            header.push_back("Provides: " + name + " = 2.9.0-2%{?dist}");
            header.push_back("Provides: " + name + "%{?_isa} = 2.9.0-2%{?dist}");
            header.push_back("Obsoletes: " + name + " < 2.9.0-2%{?dist}");
        }
        std::set<std::string> seen;
        for (const auto &line : header) if (line.empty() || seen.insert(line).second) std::cout << line << '\n';
        std::cout << "\n%description\nNative Uke device control, terminal and client interfaces, with the retained\ninstallation classifier and automatic alternating kernel delivery in one runtime\npackage. Original DNF triggers the existing admitted updater after transactions.\nPersistent preferences, recovery gates and original desktop packages remain.\n";
        std::cout << "\n%package -n " << sdk << '\n' << body(sdk_header) << "\n%description -n " << sdk << '\n' << body(sdk_description);
        std::cout << "\n%package -n " << tui << '\n' << body(tui_header) << "\n%description -n " << tui << '\n' << body(tui_description);
        for (const auto &phase : {"prep", "build", "install", "check"}) {
            std::cout << "\n%" << phase << '\n';
            for (const auto &part : phases[phase]) std::cout << part << '\n';
            if (std::string(phase) == "prep") {
                std::cout << "mapfile -t version_headers < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '*/src/version.hpp')\ntest ${#version_headers[@]} = 1\ninstall -m644 %{SOURCE11} \"${version_headers[0]}\"\n";
                std::cout << "install -m644 %{SOURCE14} KERNEL-CONSOLIDATION.md\n";
                std::cout << "patch_stage=$(mktemp -d)\nmkdir -p \"$patch_stage/uke-boot/src\" \"$patch_stage/uke-boot/tests\"\n"
                    << "boot_root=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102\n"
                    << "cp \"$boot_root/src/o82-private-color.hpp\" \"$patch_stage/uke-boot/src/\"\n"
                    << "cp \"$boot_root/tests/o82-private-color.cpp\" \"$patch_stage/uke-boot/tests/\"\n"
                    << "cp \"$boot_root/tests/managed-firmware.sh\" \"$patch_stage/uke-boot/tests/\"\n"
                    << "cp '" << color_build << "o82-stock-profile.hpp' '" << color_build << "o82-stock-profile-contract.cpp' \"$patch_stage/\"\n"
                    << "patch --batch --fuzz=0 --dry-run -p1 -d \"$patch_stage\" < %{SOURCE18}\n"
                    << "patch --batch --fuzz=0 -p1 -d \"$patch_stage\" < %{SOURCE18}\n"
                    << "install -m644 \"$patch_stage/uke-boot/src/o82-private-color.hpp\" \"$boot_root/src/\"\n"
                    << "install -m644 \"$patch_stage/uke-boot/tests/o82-private-color.cpp\" \"$boot_root/tests/\"\n"
                    << "install -m644 \"$patch_stage/uke-boot/tests/managed-firmware.sh\" \"$boot_root/tests/\"\n"
                    << "install -m644 \"$patch_stage/o82-stock-profile.hpp\" \"$patch_stage/o82-stock-profile-contract.cpp\" '" << color_build << "'\n"
                    << "install -m644 %{SOURCE17} \"$boot_root/src/unstable-release.hpp\"\n"
                    << "install -m644 %{SOURCE17} '" << color_build << "unstable-release.hpp'\n"
                    << "find \"$patch_stage\" -depth -delete\n";
                std::set<std::string> copied;
                for (const auto &line : staged_documents) if (copied.insert(line).second) std::cout << line << '\n';
            }
            if (std::string(phase) == "install") {
                for (const auto &file : excluded_files) std::cout << "if test -d '%{buildroot}" << file << "' && ! test -L '%{buildroot}" << file << "'; then find '%{buildroot}" << file << "' -depth -delete; else rm -f -- '%{buildroot}" << file << "'; fi\n";
                std::cout << "install -Dm644 %{SOURCE10} %{buildroot}/usr/share/dnf5/libdnf.conf.d/60-uke-filelists.conf\n";
            }
            if (std::string(phase) == "check") std::cout << "bash %{SOURCE15} %{SOURCE16}\n"
                << "for compiler in g++ clang++;do\n $compiler -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I%{_sourcedir} %{SOURCE20} -o unstable-release-test\n ./unstable-release-test\ndone\n";
        }
        callbacks["%pre -n uke-control"].insert(callbacks["%pre -n uke-control"].begin(),
            "# UKE_CONTROL_TRANSFER_SAVE_BEGIN\n"
            "if rpm -q uke-boot-integration >/dev/null 2>&1 || rpm -q uke-kernel-update >/dev/null 2>&1 || rpm -q uke-installation-profile >/dev/null 2>&1; then\n"
            " state=/run/uke-control-package-migration\n"
            " test ! -L \"$state\" && test ! -e \"$state/units\" || exit 1\n"
            " install -d -m700 \"$state\"\n"
            " : > \"$state/units\"\n"
            " chmod 600 \"$state/units\"\n"
            " for unit in " + [&] { std::string units; for (const auto &u : unit_names) units += u + " "; return units; }() + "; do\n"
            "  enabled=$(systemctl is-enabled \"$unit\" 2>/dev/null) || :\n"
            "  active=$(systemctl is-active \"$unit\" 2>/dev/null) || :\n"
            "  case $enabled in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) ;; not-found|'') continue;; *) exit 1;; esac\n"
            "  case $active in active|inactive|failed|activating|deactivating) ;; *) exit 1;; esac\n"
            "  printf '%%s\\t%%s\\t%%s\\n' \"$unit\" \"$enabled\" \"$active\" >> \"$state/units\"\n"
            " done\n"
            "fi\n# UKE_CONTROL_TRANSFER_SAVE_END\n");
        callbacks["%posttrans -n uke-control"].push_back(
            "# UKE_CONTROL_TRANSFER_RESTORE_BEGIN\n"
            "state=/run/uke-control-package-migration\n"
            "if test -f \"$state/units\"; then\n"
            " test ! -L \"$state\" && test ! -L \"$state/units\" || exit 1\n"
            " test \"$(stat -c %%u \"$state/units\")\" = \"$(id -u)\" || exit 1\n"
            " systemctl daemon-reload || exit 1\n"
            " while IFS=\"$(printf '\\t')\" read -r unit enabled active; do\n"
            "  case $unit in uke-*.service|uke-*.timer) ;; *) exit 1;; esac\n"
            "  case $enabled in enabled) systemctl enable \"$unit\" || exit 1;; enabled-runtime) systemctl enable --runtime \"$unit\" || exit 1;; masked|masked-runtime) continue;; disabled|static|indirect) ;; *) exit 1;; esac\n"
            "  case $active in active) systemctl --no-block start \"$unit\" || exit 1;; inactive|failed|activating|deactivating) ;; *) exit 1;; esac\n"
            " done < \"$state/units\"\n"
            " rm -f -- \"$state/units\"\n"
            " rmdir \"$state\" || :\n"
            "fi\n# UKE_CONTROL_TRANSFER_RESTORE_END\n");
        for (const auto &[marker, parts] : callbacks) {
            std::cout << '\n' << marker << '\n';
            for (const auto &part : parts) std::cout << "(\n" << part << ") || exit $?\n";
        }
        std::cout << "\n%files\n";
        seen.clear();
        for (const auto &line : main_files) if (line.empty() || seen.insert(line).second) std::cout << line << '\n';
        std::cout << "/usr/share/dnf5/libdnf.conf.d/60-uke-filelists.conf\n";
        std::cout << "%doc KERNEL-CONSOLIDATION.md\n";
        std::cout << "\n%files -n " << sdk << '\n' << body(sdk_files);
        std::cout << "\n%files -n " << tui << '\n' << body(tui_files);
        std::cout << "\n%changelog\n* Sat Nov 07 2026 Uke contributors - 2.9.0-2\n- Require the matching terminal package and retain the existing SDK separately.\n- Fold native private-color provisioning into Control and admit canonical future unstable versions.\n\n* Sat Nov 07 2026 Uke contributors - 2.9.0-1\n- Consolidate retained control and kernel delivery components in one runtime RPM.\n- Load filelists for reliable original DNF file-dependency resolution.\n\n" << body(changelog);
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
