# setup.py
#
# Building C++ still requires a setup.py file. This just runs _setup_cpp_argv.

import os
import sys
import setuptools
import subprocess
from typing import List

# C++ build command.  E.g. cmake, make, ./build.sh.
_setup_cpp_argv = ["./setup_cpp.sh"]

_verbose = 1

def verbose(x: str) -> None:
    if _verbose >= 1:
        print(x)

def run_argv(argv: List[str]) -> None:
    try:
        result = subprocess.run(
            argv,
            cwd=os.getcwd(),
            stdout=subprocess.PIPE, # a.k.a capture
            stderr=subprocess.STDOUT,
            text=True,
            check=True,
        )
        verbose(result.stdout)

    except subprocess.CalledProcessError as e:
        print(f"Error: {' '.join(argv)}: {e}", file=sys.stderr)
        if e.stdout:
            print(e.stdout, file=sys.stderr)
        sys.exit(e.returncode if e.returncode else 1)

    except Exception as e:
        print(e, file=sys.stderr)
        sys.exit(1)

# There is no trick to it.
run_argv(_setup_cpp_argv)

# Legacy: The manifest is now supposed to be in the .toml file.
setuptools.setup(
    install_requires=[],
    python_requires='>=3.7',
)
