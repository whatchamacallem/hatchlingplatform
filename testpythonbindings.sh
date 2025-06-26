#!/bin/sh

set -o errexit

# Keep a cached copy of the python modules required.
PY_CLANG_NAME="clang"
PY_CLANG_VERSION="18.1.8"

PY_BIND_NAME="pybind11"
PY_BIND_VERSION="2.13.6"

PY_LIB_DIR="cache/py"

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

export PYTHONPATH="`pwd`/$PY_LIB_DIR:."
export LLVM_CONFIG=llvm-config-18
python3 py/generate_bindings.py include/hx/hatchling_pch.hpp bindings.cpp
