# Command metadata is provided by the same registry as --help.
_uke_control() {
    local current=${COMP_WORDS[COMP_CWORD]}
    COMPREPLY=()
    if [[ $COMP_CWORD == 1 || $current == --* ]]; then
        mapfile -t COMPREPLY < <(/usr/bin/uke-control complete -- "$current")
    fi
}
complete -F _uke_control uke-control
