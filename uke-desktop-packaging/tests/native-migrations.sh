# Host-only C++ fixtures; never installed or run on the tablet.
set -Eeuo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
cxx=${CXX:-c++}
# pkg-config flags intentionally become separate compiler arguments.
# shellcheck disable=SC2046
"$cxx" -std=c++20 -Wall -Wextra -Werror $(pkg-config --cflags Qt6Core) "$project/src/native-runtime/calendar-migration.cpp" -o "$tmp/calendar" $(pkg-config --libs Qt6Core)
# shellcheck disable=SC2046
"$cxx" -std=c++20 -Wall -Wextra -Werror $(pkg-config --cflags Qt6Core Qt6Xml) "$project/src/native-runtime/dolphin-migration.cpp" -o "$tmp/dolphin_replace_view_mode_with_view_settings_in_toolbar" $(pkg-config --libs Qt6Core Qt6Xml)
cp "$tmp/dolphin_replace_view_mode_with_view_settings_in_toolbar" "$tmp/dolphin_tab_key_shortcut_for_focus_other_view"
printf '[Calendar]\r\nenabledCalendarPlugins=/usr/lib64/plugin.so,unknown-id,,/opt/other.so\r\nOther=unchanged\r\n' > "$tmp/calendar.conf"
chmod 600 "$tmp/calendar.conf"
"$tmp/calendar" --file "$tmp/calendar.conf"
printf '[Calendar]\r\nenabledCalendarPlugins=plugin,unknown-id,,other\r\nOther=unchanged\r\n' > "$tmp/calendar.expected"
cmp "$tmp/calendar.conf" "$tmp/calendar.expected"
test "$(stat -c %a "$tmp/calendar.conf")" = 600
identity=$(stat -c '%i:%Y' "$tmp/calendar.conf")
"$tmp/calendar" --file "$tmp/calendar.conf"
test "$(stat -c '%i:%Y' "$tmp/calendar.conf")" = "$identity"
ln -s calendar.conf "$tmp/calendar-link"
if "$tmp/calendar" --file "$tmp/calendar-link"; then exit 1; fi
printf '%s\n' '<gui><ToolBar><Action name="view_mode"/><Action name="unchanged"/></ToolBar><ActionProperties/></gui>' > "$tmp/input.xml"
printf '%s\n' '[General]' 'UseTabForSwitchingSplitView=true' > "$tmp/dolphinrc"
chmod 600 "$tmp/input.xml"
"$tmp/dolphin_replace_view_mode_with_view_settings_in_toolbar" --file "$tmp/input.xml" --settings "$tmp/dolphinrc"
grep -F 'view_settings' "$tmp/input.xml" >/dev/null
grep -F 'unchanged' "$tmp/input.xml" >/dev/null
"$tmp/dolphin_tab_key_shortcut_for_focus_other_view" --file "$tmp/input.xml" --settings "$tmp/dolphinrc"
grep -F 'Ctrl+F3; Tab' "$tmp/input.xml" >/dev/null
test "$(stat -c %a "$tmp/input.xml")" = 600
identity=$(stat -c '%i:%Y' "$tmp/input.xml")
"$tmp/dolphin_tab_key_shortcut_for_focus_other_view" --file "$tmp/input.xml" --settings "$tmp/dolphinrc"
test "$(stat -c '%i:%Y' "$tmp/input.xml")" = "$identity"
printf '<malformed' > "$tmp/bad.xml"
if "$tmp/dolphin_replace_view_mode_with_view_settings_in_toolbar" --file "$tmp/bad.xml" --settings "$tmp/dolphinrc"; then exit 1; fi
ln -s input.xml "$tmp/xml-link"
if "$tmp/dolphin_replace_view_mode_with_view_settings_in_toolbar" --file "$tmp/xml-link" --settings "$tmp/dolphinrc"; then exit 1; fi
printf '%s\n' 'Native migration host fixtures passed; no target or hardware tests performed'
