#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# After building the emsdk, these commands need to be run in the emsdk directory:
#
#   ./emsdk activate latest && source ./emsdk_env.sh

set -o errexit
set -o monitor # job control

export POSIXLY_CORRECT=1

# Build artifacts are not retained.
rm -rf ./bin; mkdir ./bin && cd ./bin

emcc -I../include -O2 -fpic -fdiagnostics-absolute-paths -c ../src/*.c ../test/*.c

# -sMAIN_MODULE=2 dead-strips without leaving code for other modules.
# Dump the memory manager because a web browser doesn't need that.
emcc -O2 -fpic -sMAIN_MODULE=2 -fno-exceptions -fno-rtti -fdiagnostics-absolute-paths \
	 -Werror -Wfatal-errors -DHX_MEMORY_MANAGER_DISABLE=1 -DHX_USE_THREADS=0 -I../include \
	*.o ../src/*.cpp ../test/*.cpp -o index.html

if [ "$1" != "--headless" ]; then

	# Start a web server in the background.
	python3 -m http.server 9876 &

	# Launch Chrome if it is installed.
	if which google-chrome; then
		google-chrome http://0.0.0.0:9876/ >/dev/null 2>&1;
	fi

	# Bring the web server to the foreground so it can be killed.
	fg %$(jobs | grep 'http.server' | sed -E 's/^\[([0-9]+)\].*/\1/')
fi

# Say goodbye and make sure the script returns 0.
echo 🐉🐉🐉
