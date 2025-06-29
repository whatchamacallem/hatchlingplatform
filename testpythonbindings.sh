#!/bin/sh
#
# sudo apt install python3 python3-clang nanobind-dev
#

# Smoke test.
if python3 -c 'import clang'; then
    echo "tested python3-clang..."
else
    echo "unable to load python module clang.  see script for instructions."
    exit 1
fi

PY_CFLAGS="$(python3-config --cflags)"
PY_LDFLAGS="$(python3-config --ldflags --embed)"

PY_BIND="/usr/share/nanobind"

HX_DIR=`pwd`
HX_CFLAGS="$PY_CFLAGS -fdiagnostics-absolute-paths -Wfatal-errors \
    -I$HX_DIR/include -DHX_RELEASE=0"

HX_MODULE="bindings_test"
HX_HEADERS="$HX_DIR/test/$HX_MODULE.hpp"

# Only nuke the bin dir if it is not being used for binding already.
if [ ! -f "bin/$HX_MODULE.cpp" ]; then
	rm -rf ./bin; mkdir ./bin
fi
cd ./bin

# Check timestamps and regenerate the bindings if they have changed.
# bin/$HX_MODULE.cpp nanobind/src/*.cpp -> bin/*.o
python3 $HX_DIR/py/hatchling_bindings.py -DHX_BINDINGS_PASS=1 -std=c++17 \
    $HX_CFLAGS $HX_MODULE $HX_HEADERS $HX_MODULE.cpp

set -o errexit
if [ $? -eq 0 ]; then
    # Build nanobind and the bindings in the bin directory.
    set -x
    clang++ $HX_CFLAGS -I$PY_BIND/include -std=c++17 -pthread \
        -c $PY_BIND/src/*.cpp $HX_MODULE.cpp
elif [ $? -eq 2 ]; then
    exit 1 # Assume error messages have been printed.
fi

set -x

# {src,test}/*.c -> bin/*.o
clang $HX_CFLAGS -std=c17 -pthread -c $HX_DIR/src/*.c $HX_DIR/test/*.c

# {src,test}/*.cpp bin/*.o -> bin/hxtest
clang++ $HX_CFLAGS $PY_LDFLAGS -std=c++17 -lstdc++ -pthread -lpthread \
    $HX_DIR/src/*.cpp $HX_DIR/test/*.cpp *.o -o hxtest
