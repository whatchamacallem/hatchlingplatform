#!/bin/sh
#
# sudo apt install python3 python3-clang nanobind-dev
#

set -o errexit

# Smoke test.
if python3 -c 'import clang'; then
    echo "tested python3-clang..."
else
    echo "unable to load python module clang"
    exit 1
fi

PY_CFLAGS="$(python3-config --cflags)"
PY_LDFLAGS="$(python3-config --ldflags) -lpython3.12" # wtf this library was missing.

PY_BIND="/usr/share/nanobind"
HX_DIR=`pwd`

# Only nuke the bin dir if it is not being used for binding already.
if [ ! -f "bin/python_bindings.cpp" ]; then
	rm -rf ./bin; mkdir ./bin
fi
cd ./bin

set -x

# Check timestamps and regenerate the bindings if they have changed.
python3 $HX_DIR/py/generate_bindings.py $HX_DIR/include/hx/hatchling_pch.hpp python_bindings.cpp

clang -I$HX_DIR/include -DHX_RELEASE=0 -std=c17 -I$PY_BIND/include $PY_CFLAGS \
    -fdiagnostics-absolute-paths -Wfatal-errors -c $HX_DIR/src/*.c $HX_DIR/test/*.c

clang++ -I$HX_DIR/include -I$PY_BIND/include -I. $PY_CFLAGS $PY_LDFLAGS \
    -DHX_RELEASE=0 -pthread -lpthread -std=c++17 -lstdc++ \
    -fdiagnostics-absolute-paths -Wfatal-errors python_bindings.cpp \
    $PY_BIND/src/*.cpp $HX_DIR/*/*.cpp *.o -o hxtest
