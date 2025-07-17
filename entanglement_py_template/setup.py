# setup.py
#
# Building C++ still requires a setup.py file.

import os
import sys
import setuptools
import subprocess
from typing import List

# C++ build command.  E.g. cmake, make, ./build.sh.
SETUP_CPP_COMMAND = ['./setup_cpp.sh']

LIBRARY='libentanglement_py_template.so.1'
HEADER_FILES=['entanglement_cpp_test.hpp']
OUTPUT_FILE='entanglement_py_template.py'

ENTANGLEMENT_COMMAND=['python3', 'entanglement.py', '-std=c++17', '-DENTANGLEMENT_PASS=1',
    '-I../include', '-DHX_RELEASE=0', '-fdiagnostics-absolute-paths',
    '-Wfatal-errors', LIBRARY] + HEADER_FILES + [OUTPUT_FILE]

VERBOSE = 1

_exit_error = 1

def _verbose(x: str) -> None:
    if VERBOSE:
        print(x)

# Takes an argv and throws an exception if it doesn't return 0.
def _run_argv(argv: List[str]) -> None:
    try:
        _verbose(' '.join(argv))
        result = subprocess.run(
            argv,
            cwd=os.getcwd(),
            stdout=subprocess.PIPE, # a.k.a capture
            stderr=subprocess.STDOUT,
            text=True,
            check=True,
        )
        _verbose(result.stdout)

    except subprocess.CalledProcessError as e:
        if e.stdout:
            print(e.stdout, file=sys.stderr)
        print(e, file=sys.stderr)
        sys.exit(e.returncode if e.returncode else _exit_error)

    except Exception as e:
        print(e, file=sys.stderr)
        sys.exit(_exit_error)

# Run Commands #

#_run_argv(SETUP_CPP_COMMAND)

_run_argv(ENTANGLEMENT_COMMAND)

_run_argv(['python3', 'entanglement_py_template.py'])



# This is legacy. The manifest is now supposed to be in the .toml file.
#setuptools.setup(
#    install_requires=[],
#    python_requires='>=3.7',
#)

print("🐉🐉🐉")
