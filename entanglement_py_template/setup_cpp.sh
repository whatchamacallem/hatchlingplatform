#!/bin/sh
# Copyright 2017-2025 Adrian Johnston
#
# sudo apt install python3 python3-clang

set -o errexit

VERBOSE=1

HX_DIR=$(dirname "$(pwd)")

HX_PACKAGE="entanglement_py_template"
HX_HEADER_FILES="$HX_DIR/test/entanglement_cpp_test.hpp"
HX_OUTPUT_FILE="$HX_PACKAGE.py"

PY_CFLAGS="$(python3-config --cflags)"
PY_LDFLAGS="$(python3-config --ldflags --embed)"

HX_CFLAGS="$PY_CFLAGS -I$HX_DIR/include -DHX_RELEASE=0 \
    -fdiagnostics-absolute-paths -Wfatal-errors"

# VERBOSE > 0
if [ "$VERBOSE" -ne 0 ]; then
  echo "HX_DIR=$HX_DIR"
  echo "HX_PACKAGE=$HX_PACKAGE"
  echo "HX_HEADER_FILES=$HX_HEADER_FILES"
  echo "HX_OUTPUT_FILE=$HX_OUTPUT_FILE"
  echo "PY_CFLAGS=$PY_CFLAGS"
  echo "PY_LDFLAGS=$PY_LDFLAGS"
  echo "HX_CFLAGS=$HX_CFLAGS"
fi

# Smoke test the Python clang module.
if python3 -c 'import clang'; then
    echo "tested python3-clang..."
else
    echo "unable to load python module clang."
    exit 1
fi

set +o errexit

# Check timestamps and regenerate the bindings if they have changed.
python3 entanglement.py -DHX_BINDINGS_PASS=1 -std=c++17 \
    $HX_CFLAGS $HX_PACKAGE $HX_HEADER_FILES $HX_OUTPUT_FILE
if [ $? -ne 0 ] && [ $? -ne 2 ]; then
    exit $? # Either the script failed somehow or it returned failure.
fi

cd ./bin
set -o errexit -o xtrace

# {src,test}/*.c -> bin/*.o
clang $HX_CFLAGS -std=c17 -fvisibility=hidden -pthread -c $HX_DIR/src/*.c $HX_DIR/test/*.c

# {src,test}/*.cpp -> bin/hxtest
clang++ $HX_CFLAGS $PY_LDFLAGS -std=c++17 -fvisibility=hidden \
    -pthread $HX_DIR/src/*.cpp $HX_DIR/test/*.cpp

# entanglement_py_template/*.cpp bin/*.o -> bin/hxtest
clang++ $HX_CFLAGS $PY_LDFLAGS -std=c++17 -lstdc++ -Wl,-s \
    -pthread -lpthread $HX_DIR/entanglement_py_template/*.cpp *.o -o hxtest

{ set +o xtrace; } 2> /dev/null
echo 🐉🐉🐉
