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

# Keep a cached copy of the python modules required.
PY_CLANG_NAME="clang"
PY_CLANG_VERSION="18.1.8"

PY_BIND_NAME="pybind11"
PY_BIND_VERSION="2.13.6"

PY_LIB_DIR="cache/py"
HX_DIR="`pwd`"

export PYTHONPATH="`pwd`/$PY_LIB_DIR:."
export LLVM_CONFIG=llvm-config-18

TEST_PY="import sys; sys.path.insert(0, \"$PY_LIB_DIR\"); import $PY_CLANG_NAME; import $PY_BIND_NAME"
if [ -d "$PY_LIB_DIR" ] && python3 -c "$TEST_PY" > /dev/null 2>&1; then
    echo "tested $PY_CLANG_NAME, $PY_BIND_NAME from $PY_LIB_DIR..."
else
    echo "downloading and installing $PY_CLANG_NAME to $PY_LIB_DIR"
    set +x
    mkdir -p $PY_LIB_DIR
    pip install --target=$PY_LIB_DIR --no-cache-dir $PY_CLANG_NAME==$PY_CLANG_VERSION
    pip install --target=$PY_LIB_DIR --no-cache-dir $PY_BIND_NAME==$PY_BIND_VERSION
fi

# Build artifacts are not retained.
rm -rf ./bin; mkdir ./bin && cd ./bin

python3 $HX_DIR/py/generate_bindings.py $HX_DIR/include/hx/hatchling_pch.hpp python_bindings.cpp
