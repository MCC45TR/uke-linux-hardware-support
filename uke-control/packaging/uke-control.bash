# Command metadata is provided by the same registry as --help.
_uke_control() {
    local current=${COMP_WORDS[COMP_CWORD]}
    COMPREPLY=()
    if [[ $COMP_CWORD == 2 && ${COMP_WORDS[1]} == set ]]; then
        mapfile -t COMPREPLY < <(compgen -W 'rollback-enforced rollback-permissive rollback-off' -- "$current")
        return
    fi
    if [[ $COMP_CWORD == 2 && ${COMP_WORDS[1]} == rollback ]]; then
        mapfile -t COMPREPLY < <(compgen -W 'status' -- "$current")
        return
    fi
    if [[ $COMP_CWORD == 1 || $current == --* ]]; then
        mapfile -t COMPREPLY < <(/usr/bin/uke-control complete -- "$current")
    fi
}
complete -F _uke_control uke-control
