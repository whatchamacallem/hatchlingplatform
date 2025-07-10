#!/bin/sh
# Copyright 2017-2025 Adrian Johnston

rm -rf ./bin; mkdir ./bin && cd ./bin

# Create and activate a Python virtual environment.
python3 -m venv --system-site-packages python_venv
. python_venv/bin/activate

set -o xtrace -o errexit

# Executes ./setup.py and then installs a Python package.
pip install ../entanglement_py_template

# Run Python tests for the package.
python -c "import entanglement_py_template; entanglement_py_template.run_all_tests();"

# Shut down the virtual environment and clean up.
deactivate
rm -rf python_venv __pycache__ build entanglement_py_template.egg-info
