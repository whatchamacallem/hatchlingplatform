#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.

rm -rf ./bin; mkdir ./bin && cd ./bin

# Create and activate a Python virtual environment.
python3 -m venv --system-site-packages python_venv
. python_venv/bin/activate

# Use the system clang wrapper because clang needs to be installed anyhow.
python3 -c "import clang.cindex; print('clang.cindex is ok...')"

set -o xtrace -o errexit

# Executes ./setup.py and then installs a Python package.
python3 -m pip install ../entanglement_example

# Run Python tests for the package.
python -c "import entanglement_example; entanglement_example.run_all_tests();"

# Shut down the virtual environment.
deactivate
