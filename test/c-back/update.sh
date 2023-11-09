#!/bin/bash

if [[ -z "$C_FRONT" ]]; then
    C_FRONT=C_Front
fi

SED=sed

if which gsed >/dev/null 2>&1; then
    SED=gsed
fi

for in in cback_*.c; do
    out="${in%.c}.xml"

    echo "$in ..."
    cat $in | $SED -e 's/time="[^"]*"//' | "$C_FRONT" -o "$out"
done
