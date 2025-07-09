# setup.py
#
# Building C++ still requires a setup.py file. This just runs _cpp_setup_command.

import os
import sys
import setuptools
import subprocess
from typing import List

# C++ build command.  E.g. cmake, make, ./build.sh.
_cpp_setup_command = ["./setup_cpp.sh"]

def run_command(command: List[str]):
    try:
        subprocess.check_call(command, cwd=os.getcwd())
    except subprocess.CalledProcessError as e:
        print(f"Error: {' '.join(command)}: {e}")
        sys.exit(1)

# Just run the command. There is no trick to it.
run_command(_cpp_setup_command)

# Legacy: The manifest is now supposed to be in the .toml file.
setuptools.setup(
    install_requires=[],
    python_requires='>=3.7',
)
