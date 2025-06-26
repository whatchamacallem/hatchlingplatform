#!/bin/sh
# Copyright 2017-2025 Adrian Johnston
#
# sudo apt install musl musl-dev musl-tools

export POSIXLY_CORRECT=1

set -o errexit

if [ ! -f bin/hxtest ]; then
    echo "bin/hxtest not found..."
	return 2; # file not found.
fi

nm -a bin/hxtest | grep " T " | awk '{print $3}' | c++filt | sort | less

echo 🐉🐉🐉
