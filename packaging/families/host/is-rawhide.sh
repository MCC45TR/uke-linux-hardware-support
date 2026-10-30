#!/usr/bin/env bash
# Read os-release as data; never execute it or infer Rawhide from VERSION_ID alone.
set -Eeuo pipefail
[[ $# == 2 && $2 =~ ^[0-9]+$ ]] || exit 2
awk -v number="$2" '
    /^[A-Z_]+=/ {
        at=index($0,"="); key=substr($0,1,at-1); value=substr($0,at+1)
        first=substr(value,1,1)
        if((first=="\"" || first=="\047") && substr(value,length(value),1)==first)
            value=substr(value,2,length(value)-2)
        fields[key]=value
    }
    END {
        if(fields["ID"]!="fedora")exit 1
        if(fields["VERSION"]=="Rawhide" || fields["VERSION_ID"]=="rawhide")exit 0
        exit !(fields["VERSION_ID"]==number &&
            fields["VERSION"]==number " (Rawhide Prerelease)" &&
            fields["RELEASE_TYPE"]=="development" &&
            fields["REDHAT_BUGZILLA_PRODUCT_VERSION"]=="rawhide")
    }
' "$1"
