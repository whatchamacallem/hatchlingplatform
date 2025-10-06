#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# The entanglement.py script and the test suite can be run on the command
# line without building a package. It is also possible to debug the venv
# built by this script. See debugging.txt in entanglement_test.

set -o errexit

rm -rf ./bin; mkdir ./bin && cd ./bin

# Create and activate a Python virtual environment.
python3 -m venv --system-site-packages python_venv
. python_venv/bin/activate

# Use the system clang wrapper because clang needs to be installed anyway.
python3 -c "import clang.cindex; print('clang.cindex is ok...')"

set -o xtrace

# Executes setup.py and then installs the Python package in the venv.
# --editable causes the installed package to use links to the source
# instead of copies. This allows you to edit an installed package
# during testing.
python3 -m pip install --editable ../entanglement_example

# Run Python tests for the package.
python -m entanglement_example.entanglement_test

{ set +o xtrace; } 2> /dev/null

# Shut down the virtual environment.
deactivate
