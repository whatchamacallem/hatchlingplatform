#!/bin/sh

set -o errexit

# Keep a cached copy of the python bindings for clang. They are in a python
# module named clang.
PY_MODULE_NAME="clang"
PY_LIB_DIR="cache/py"

TEST_PY="import sys; sys.path.insert(0, \"$PY_LIB_DIR\"); import $PY_MODULE_NAME"
if [ -d "$PY_LIB_DIR" ] && python3 -c "$TEST_PY"; then
    echo "successfully tested $PY_MODULE_NAME from $PY_LIB_DIR..."
else
    echo "downloading and installing $PY_MODULE_NAME to $PY_LIB_DIR"
    set +x
    mkdir -p $PY_LIB_DIR
    pip install --target=$PY_LIB_DIR $PY_MODULE_NAME --no-cache-dir
fi

export PYTHONPATH="`pwd`/$PY_LIB_DIR:."
python3 py/generate_bindings.py include/hx/hatchling_pch.hpp bindings.cpp
