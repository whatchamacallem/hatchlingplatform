#!/bin/sh
# Copyright 2017-2025 Adrian Johnston

set -o xtrace

cd ./entanglement_py_template

# Create and activate a Python virtual environment.
python3 -m venv python_venv
source python_venv/bin/activate

# Executes ./setup.py and then installs a Python package.
pip install .

# Run Python tests for the package.
python -c "import entanglement_py_template; entanglement_py_template.run_all_tests();"

# Shut down the virtual environment and clean up.
deactivate
rm -rf python_venv __pycache__ build entanglement_py_template.egg-info
