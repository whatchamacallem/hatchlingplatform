#!/bin/sh
# Copyright 2017-2025 Adrian Johnston
#
# sudo apt install musl musl-dev musl-tools

set -o errexit

export POSIXLY_CORRECT=1

if [ ! -f bin/hxtest ]; then
    echo "bin/hxtest not found..."
	return 2; # file not found.
fi

nm --radix=d --print-size bin/hxtest | awk 'NF == 4 {print $2, $3, $4}' | sort -r | head -n 200 \
 | python3 -c 'import re,sys;[print(re.sub(r"\b0+(?=\d)",lambda m:" "*len(m.group()),l),end="") for l in sys.stdin]' \
 | c++filt
