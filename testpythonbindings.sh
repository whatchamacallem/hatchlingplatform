#!/bin/sh

set -o errexit

# Check for clang version 18. This is tested on Ubuntu 24.04 LTS.
CLANG_VERSION_REQ=18
CLANG_VERSION_STR=`clang --version 2>/dev/null | head -n 1`
CLANG_VERSION_NUM=`echo "$CLANG_VERSION_STR" | sed 's/[^0-9]*\([0-9][0-9]*\).*/\1/'`

if [ "$CLANG_VERSION_NUM" != "$CLANG_VERSION_REQ" ]; then
    echo "Error: clang $CLANG_VERSION_REQ required."
    exit 1
fi

# Check for Python3.
PYTHON_EXEC=$(which python3)
if [ -z "$PYTHON_EXEC" ]; then
    echo "Error: Python executable not found" >&2
    exit 1
fi

# Keep a cached copy of the python modules required.
PY_CLANG_NAME="clang"
PY_CLANG_VERSION="18.1.8"

PY_BIND_NAME="nanobind"
PY_BIND_VERSION="2.7.0"

HX_DIR="`pwd`"
PY_LIB_DIR="$HX_DIR/cache/py"

# Shell variables for external tools.
export PYTHONPATH="$PY_LIB_DIR"
export LLVM_CONFIG=llvm-config-$CLANG_VERSION_REQ

# Run python with the two cached packages and see if it works.
TEST_PY="import sys; sys.path.insert(0, \"$PY_LIB_DIR\"); import $PY_CLANG_NAME; import $PY_BIND_NAME"
if [ -d "$PY_LIB_DIR" ] && python3 -c "$TEST_PY" > /dev/null 2>&1; then
    echo "tested $PY_CLANG_NAME, $PY_BIND_NAME from $PY_LIB_DIR..."
else
    echo "downloading and installing to $PY_LIB_DIR..."
    set -x
    mkdir -p $PY_LIB_DIR
    pip install --target=$PY_LIB_DIR --no-cache-dir $PY_CLANG_NAME==$PY_CLANG_VERSION
    pip install --target=$PY_LIB_DIR --no-cache-dir $PY_BIND_NAME==$PY_BIND_VERSION
    { set +x; } 2> /dev/null
fi

# Only nuke the bin dir if it is not being used for binding already.
if [ ! -f "bin/python_bindings.cpp" ]; then
	rm -rf ./bin; mkdir ./bin
fi
cd ./bin

PY_CFLAGS="$(python3-config --cflags)"
PY_LDFLAGS="$(python3-config --ldflags) -lpython3.12" # wtf this library was missing.
PY_BIND_INCLUDE="$PY_LIB_DIR/$PY_BIND_NAME/include"

set -x

# Check timestamps and regenerate the bindings if they have changed.
$PYTHON_EXEC $HX_DIR/py/generate_bindings.py $HX_DIR/include/hx/hatchling_pch.hpp python_bindings.cpp

clang -I$HX_DIR/include -DHX_RELEASE=0 -std=c17 -I$PY_BIND_INCLUDE $PY_CFLAGS \
    -fdiagnostics-absolute-paths -Wfatal-errors -c $HX_DIR/src/*.c $HX_DIR/test/*.c

clang++ -I$HX_DIR/include -I$PY_BIND_INCLUDE $PY_CFLAGS $PY_LDFLAGS \
    -DHX_RELEASE=0 -pthread -lpthread -std=c++17 -lstdc++ \
    -fdiagnostics-absolute-paths -Wfatal-errors python_bindings.cpp $HX_DIR/*/*.cpp *.o -o hxtest
