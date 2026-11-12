# SPDX-License-Identifier: GPL-2.0-or-later
# Experimental offline composer for checkpolicy -b -M -C output.
# This does not install or load policy. Runtime lifecycle is not qualified.
function has(words, item) { return (" " words " ") ~ (" " item " ") }
function rule(text) {
    if (condition != "") {
        if (branch == "") { bad=1; return }
        text=condition "\n    (" branch "\n        " text "\n    )\n)"
    }
    rules[++nr]=text
}
function extend(class, names,    n,i,a,missing) {
    n=split(names,a," "); missing=""
    for(i=1;i<=n;i++) if(!has(perms[class],a[i])) {
        needed[class,a[i]]=1
        missing=missing (missing=="" ? "" : " ") a[i]
    }
    if(missing!="") {
        if(common[class]!="") {
            print "Cannot extend a class already associated with a common: " class > "/dev/stderr"
            bad=1; return
        }
        definitions[++nd]="(common uke_" class "_compat (" missing "))\n(classcommon " class " uke_" class "_compat)"
    }
}
BEGIN { depth=0 }
{
    line=$0; sub(/^[ \t]+/,"",line); sub(/[ \t]+$/,"",line)
    if(match(line,/^\(class ([[:alnum:]_]+) \(([^()]*)\)\)$/,m)) {
        declared[m[1]]=1; perms[m[1]]=m[2]
    }
    if(match(line,/^\(classcommon ([[:alnum:]_]+) ([[:alnum:]_]+)\)$/,m)) common[m[1]]=m[2]
    if(match(line,/^\(common ([[:alnum:]_]+) \(([^()]*)\)\)$/,m)) commons[m[1]]=m[2]
    if(line ~ /^\(booleanif /) {
        if(depth!=0) bad=1
        condition=line
    }
    if(condition!="" && line ~ /^\((true|false)$/) branch=substr(line,2)
    if(match(line,/^\(allow ([[:alnum:]_]+) ([[:alnum:]_]+) \(([[:alnum:]_]+) \(([^()]*)\)\)\)$/,m)) {
        source=m[1]; target=m[2]; class=m[3]; permissions=m[4]
        if(target=="io_uring_t" && class=="anon_inode" && has(permissions,"create"))
            rule("IO " source)
        # Preserve the six existing wildcard/system-role compatibility grants,
        # only while their original kernel-module grant remains in the input.
        if(class=="system" && has(permissions,"module_load") &&
           ((source ~ /^(init_t|staff_t|sysadm_t|user_t)$/ && target=="self") ||
            (source=="kern_unconfined" && target=="kernel_t") ||
            (source=="unconfined_domain_type" && target=="domain")))
            rule("SYS " source " " target)
    }
    # checkpolicy emits quoted filenames too; ignore parentheses inside strings.
    quoted=0; escaped=0
    for(i=1;i<=length(line);i++) {
        c=substr(line,i,1)
        if(escaped) { escaped=0; continue }
        if(quoted && c=="\\") { escaped=1; continue }
        if(c=="\"") { quoted=!quoted; continue }
        if(!quoted) { if(c=="(")depth++; if(c==")")depth-- }
    }
    if(depth<0) bad=1
    if(depth==0) { condition=""; branch="" }
}
END {
    if(depth!=0) bad=1
    for(class in common) perms[class]=perms[class] " " commons[common[class]]
    if(!declared["system"] || !declared["bpf"] || !declared["io_uring"] || commons["file"]=="") bad=1
    extend("system","firmware_load kexec_image_load kexec_initramfs_load policy_load x509_certificate_load")
    extend("bpf","map_create_as prog_load_as")
    extend("io_uring","allowed")
    if(!declared["memfd_file"])
        definitions[++nd]="(class memfd_file (execute_no_trans entrypoint))\n(classcommon memfd_file file)\n(classorder (unordered memfd_file))"
    if(bad) exit 1
    print "; Experimental offline kernel compatibility composition; no policy load."
    for(i=1;i<=nd;i++) print definitions[i]
    for(i=1;i<=nr;i++) {
        text=rules[i]
        if(text ~ /IO /) {
            if(!needed["io_uring","allowed"]) continue
            text=gensub(/IO ([[:alnum:]_]+)/,"(allow \\1 self (io_uring (allowed)))","g",rules[i])
        } else {
            list=""
            n=split("firmware_load kexec_image_load kexec_initramfs_load policy_load x509_certificate_load",a," ")
            for(j=1;j<=n;j++) if(needed["system",a[j]]) list=list (list=="" ? "" : " ") a[j]
            if(list=="") continue
            text=gensub(/SYS ([[:alnum:]_]+) ([[:alnum:]_]+)/,"(allow \\1 \\2 (system (" list ")))","g",text)
        }
        print text
    }
    if(needed["system","firmware_load"]) {
        print "(allow kernel_t lib_t (system (firmware_load)))"
        print "(booleanif secure_mode_insmod (false (allow can_load_kernmodule lib_t (system (firmware_load)))))"
    }
}
