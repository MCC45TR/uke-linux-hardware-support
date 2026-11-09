# Existing Control package; one opt-in operation before the compositor loads.
# Healthy sessions without a request start no extra process or sensor stream.
if [ -f "${XDG_CONFIG_HOME:-$HOME/.config}/uke-control/brightness-curve-request.json" ]; then
    /usr/libexec/uke-control/brightness-curve apply-pending || :
fi
