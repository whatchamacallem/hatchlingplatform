# setup.py
#
# Building C++ still requires a setup.py file.

import os
import sys
import setuptools
import subprocess
from typing import List

# C++ build command.  E.g. cmake, make, ./build.sh.
_cpp_setup_command = ["./setup_cpp.sh"]

# There is no secret sauce.  This is an argument vector starting with the
# command name.
def run_command(command: List[str]):
    try:
        subprocess.check_call(command, cwd=os.getcwd())
    except subprocess.CalledProcessError as e:
        print(f"Error running {command}: {e}")
        sys.exit(1)

run_command(_cpp_setup_command)


setuptools.setup(
    install_requires=[],
    classifiers=[
        'Programming Language :: C++',
        'Programming Language :: Python :: 3',
    ],
    python_requires='>=3.7',
)
