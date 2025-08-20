#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# sudo apt install python3 python3-clang

set -o errexit

HX_DIR=$(dirname "$(pwd)")

PY_CFLAGS="$(python3-config --cflags)"
PY_LDFLAGS="$(python3-config --ldflags --embed)"

HX_CFLAGS="$PY_CFLAGS -DHX_RELEASE=0 -I$HX_DIR/include -fPIC -pthread \
	-fdiagnostics-absolute-paths -Wfatal-errors"

HX_LDFLAGS="$PY_LDFLAGS -shared -Wl,-s"

set -o xtrace

# The .toml requires the .so to be in the src directory.
clang++ $HX_CFLAGS $HX_LDFLAGS -std=c++17 -lstdc++ -lpthread \
	$HX_DIR/entanglement_example/src/*.cpp -o src/libentanglement_example.so.1

{ set +o xtrace; } 2> /dev/null
rm -f *.o
