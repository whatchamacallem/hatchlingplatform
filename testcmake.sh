#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.

# Only rerun cmake manually as is customary. Build artifacts ARE retained.

set -o errexit

export POSIXLY_CORRECT=1

if [ ! -f bin/build.ninja ]; then
	rm -rf bin
	cmake -S . -B bin -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
else
	echo found bin/build.ninja...
fi

ninja -C bin

echo run ./hxtest...
cd bin

./hxtest > testcmake.sh.txt 2>&1
CODE=$?

# Filter out critical status messages only. This should stop the test if no
# output matches. The goal is to prevent AI from panicking over spew.
grep -E '\[  PASSED  \]|\[  FAILED  \]|FAILED TESTS' testcmake.sh.txt

if [ "$CODE" -ne 0 ]; then
	# Dump everything a second time with all spew enabled.
	cat testcmake.sh.txt
	echo "Stopping due to bin/hxtest returning $CODE."
    exit "$CODE"
fi

cd ..

echo run-clang-tidy... This is very slow.
# Depends on -DCMAKE_EXPORT_COMPILE_COMMANDS=ON above. These two have to happen
# together. The output is filtered to avoid confusing AI.
OUT=$(run-clang-tidy -quiet -p bin src/*.cpp src/*.c test/*.cpp 2>&1)
CODE=$?
printf '%s\n' "$OUT" | grep -Ev '^[[:space:]]*[0-9]+ (warnings?|errors?) generated\.$' || true
[ "$CODE" -ne 0 ] && exit "$CODE"

echo 🐉🐉🐉
