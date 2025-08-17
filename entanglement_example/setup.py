# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# setup.py - Building C++ still requires a setup.py file.

import os, setuptools, subprocess, sys
from typing import List

# C++ build command.  E.g. cmake, make, ./build.sh.
SETUP_CPP_COMMAND = ['./setup_cpp.sh']

# Python wrapper build command.
LIBRARY='libentanglement_py_template.so.1'
HEADER_FILES=['entanglement_test_a.hpp', 'entanglement_test_b.hpp']
OUTPUT_FILE='entanglement_example.py'

ENTANGLEMENT_COMMAND=['python3', 'entanglement.py', '-std=c++17', '-DENTANGLEMENT_PASS=1',
	'-I../include', '-DHX_RELEASE=0', '-fdiagnostics-absolute-paths',
	'-Wfatal-errors', LIBRARY] + HEADER_FILES + [OUTPUT_FILE]

# Takes an argv and throws an exception if it doesn't return 0.
def _run_argv(argv: List[str]) -> None:
	try:
		print(' '.join(argv))
		result = subprocess.run(
			argv,
			cwd=os.getcwd(),
			stdout=subprocess.PIPE, # a.k.a capture
			stderr=subprocess.STDOUT,
			text=True,
			check=True,
		)
		print(result.stdout)

	except subprocess.CalledProcessError as e:
		if e.stdout:
			print(e.stdout, file=sys.stderr)
		print(e, file=sys.stderr)
		sys.exit(e.returncode if e.returncode else 1)

	except Exception as e:
		print(e, file=sys.stderr)
		sys.exit(1)

# Run C++ and Python wrapper build commands.
_run_argv(SETUP_CPP_COMMAND)
_run_argv(ENTANGLEMENT_COMMAND)

# Package everything. The manifest is now the .toml file. This is just a legacy
# hook. Takes command line args when run directly.
# Usage: python3 ./setup.py --help
setuptools.setup(
	python_requires='>=3.7',
	packages=[
        "entanglement",
        "entanglement_example",
        "entanglement_test"
    ])

print("🐉🐉🐉")
