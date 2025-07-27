#!/bin/sh
# Copyright 2017-2025 Adrian Johnston
#
# sudo apt install python3 python3-clang

set -o errexit

HX_DIR=$(dirname "$(pwd)")

PY_CFLAGS="$(python3-config --cflags)"
PY_LDFLAGS="$(python3-config --ldflags --embed)"

HX_CFLAGS="$PY_CFLAGS -fPIC -pthread -I$HX_DIR/include -DHX_RELEASE=0 \
	-fdiagnostics-absolute-paths -Wfatal-errors"
HX_LDFLAGS="$PY_LDFLAGS -shared -Wl,-s"


set -o xtrace

# {src,test}/*.c -> bin/*.o
clang $HX_CFLAGS -std=c17 -fvisibility=hidden -c $HX_DIR/src/*.c $HX_DIR/test/*.c

# {src,test}/*.cpp -> bin/hxtest
clang++ $HX_CFLAGS -std=c++17 -fvisibility=hidden \
	-c $HX_DIR/src/*.cpp $HX_DIR/test/*.cpp

# entanglement_py_template/*.cpp bin/*.o -> bin/hxtest
clang++ $HX_CFLAGS $HX_LDFLAGS -std=c++17 -lstdc++ -lpthread \
	$HX_DIR/entanglement_py_template/*.cpp *.o -o libentanglement_py_template.so.1

{ set +o xtrace; } 2> /dev/null
rm *.o
echo 🐉🐉🐉
