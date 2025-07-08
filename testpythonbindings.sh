#!/bin/sh
# Copyright 2017-2025 Adrian Johnston
#
# sudo apt install python3 python3-clang nanobind-dev

set -o errexit

HX_DIR=`pwd`

HX_MODULE="bindings_test"
HX_HEADER_FILES="$HX_DIR/test/$HX_MODULE.hpp"
HX_OUTPUT_FILE="$HX_MODULE.bind.cpp"

# TODO.
PY_BIND="/usr/share/nanobind"

PY_CFLAGS="$(python3-config --cflags)"
PY_LDFLAGS="$(python3-config --ldflags --embed)"

HX_CFLAGS="$PY_CFLAGS -I$HX_DIR/include -DHX_RELEASE=0 \
    -fdiagnostics-absolute-paths -Wfatal-errors"

# Smoke test Python's Clang module first.
if python3 -c 'import clang'; then
    echo "tested python3-clang..."
else
    echo "unable to load python module clang.  see script for instructions."
    exit 1
fi

# Only nuke the bin dir if it is not being used for binding already.
if [ ! -f "bin/$HX_OUTPUT_FILE" ]; then
	rm -rf ./bin; mkdir ./bin
fi
cd ./bin

set +o errexit

# Check timestamps and regenerate the bindings if they have changed.
# bin/$HX_MODULE.cpp nanobind/src/*.cpp -> bin/*.o
python3 $HX_DIR/py/entanglement.py -DHX_BINDINGS_PASS=1 -std=c++17 \
    $HX_CFLAGS $HX_MODULE $HX_HEADER_FILES $HX_OUTPUT_FILE
if [ $? -eq 0 ]; then
    # Build nanobind and the bindings in the bin directory.
    set -o errexit -o xtrace
    clang++ $HX_CFLAGS -I$PY_BIND/include -std=c++17 -pthread \
        -c $PY_BIND/src/*.cpp $HX_OUTPUT_FILE
elif [ $? -eq 1 ]; then
    exit $? # Either the script failed somehow or it returned failure.
else
    # Code 2: All done.
    set -o errexit -o xtrace
fi

# {src,test}/*.c -> bin/*.o
clang $HX_CFLAGS -std=c17 -fvisibility=hidden -pthread -c $HX_DIR/src/*.c $HX_DIR/test/*.c

# {src,test}/*.cpp bin/*.o -> bin/hxtest
clang++ $HX_CFLAGS $PY_LDFLAGS -std=c++17 -fvisibility=hidden -lstdc++ -Wl,-s \
    -pthread -lpthread $HX_DIR/src/*.cpp $HX_DIR/test/*.cpp *.o -o hxtest

{ set +o xtrace; } 2> /dev/null
echo 🐉🐉🐉
