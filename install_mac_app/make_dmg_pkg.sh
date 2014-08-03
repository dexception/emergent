#!/bin/bash

rtnm=$1

if [[ "$rtnm" == "" ]]; then
    echo "ERROR: you must specify file name without .pkg extension as only arg"
    exit 1
fi

set pkgnm="$rtnm".pkg
set dmgnm="$rtnm".dmg

echo "making dmg: $dmgnm from package: $pkgnm"

/usr/bin/hdiutil create -ov -srcfolder "$pkgnm" "$dmgnm"