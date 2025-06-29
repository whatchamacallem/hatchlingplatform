#!/bin/sh
#
# sudo apt install python3 python3-clang nanobind-dev
#

set -o errexit

# Smoke test.
if python3 -c 'import clang'; then
    echo "tested python3-clang..."
else
    echo "unable to load python module clang.  see script for instructions."
    exit 1
fi

PY_CFLAGS="$(python3-config --cflags)"
PY_LDFLAGS="$(python3-config --ldflags) -lpython3.12" # wtf this library was missing.

PY_BIND="/usr/share/nanobind"

HX_DIR=`pwd`
HX_CFLAGS="$PY_CFLAGS -fdiagnostics-absolute-paths -Wfatal-errors \
    -I$HX_DIR/include -DHX_RELEASE=0"

# Only nuke the bin dir if it is not being used for binding already.
if [ ! -f "bin/python_bindings.cpp" ]; then
	rm -rf ./bin; mkdir ./bin
fi
cd ./bin

set -x

# Check timestamps and regenerate the bindings if they have changed.
# bin/python_bindings.cpp nanobind/src/*.cpp -> bin/*.o
if python3 $HX_DIR/py/generate_bindings.py -DHX_BIND_GEN=1 -std=c++17 \
    $HX_CFLAGS $HX_DIR/include/hx/hatchling_pch.hpp \
        python_bindings.cpp; then
    # Build nanobind and the bindings in the bin directory.
    clang++ $HX_CFLAGS -I$PY_BIND/include -I. -std=c++17 -pthread \
        -c $PY_BIND/src/*.cpp $HX_DIR/bin/python_bindings.cpp
fi

# {src,test}/*.c -> bin/*.o
clang $HX_CFLAGS -std=c17 -pthread -c $HX_DIR/src/*.c $HX_DIR/test/*.c

# {src,test}/*.cpp bin/*.o -> bin/hxtest
clang++ $HX_CFLAGS $PY_LDFLAGS -std=c++17 -lstdc++ -pthread -lpthread \
    $HX_DIR/src/*.cpp $HX_DIR/test/*.cpp *.o -o hxtest
