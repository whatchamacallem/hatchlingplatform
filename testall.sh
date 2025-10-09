#!/bin/sh
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# If bin/hxtest fails, then the tests will stop at that point and it will be
# available to debug. Any shell command that fails should also stop the test
# process.

clear

echo "
$(tput rev)
.                                                                             .
.   ██╗  ██╗ █████╗ ████████╗ ██████╗██╗  ██╗██╗     ██╗███╗   ██╗ ██████╗    .
.   ██║  ██║██╔══██╗╚══██╔══╝██╔════╝██║  ██║██║     ██║████╗  ██║██╔════╝    .
.   ███████║███████║   ██║   ██║     ███████║██║     ██║██╔██╗ ██║██║  ███╗   .
.   ██╔══██║██╔══██║   ██║   ██║     ██╔══██║██║     ██║██║╚██╗██║██║   ██║   .
.   ██║  ██║██║  ██║   ██║   ╚██████╗██║  ██║███████╗██║██║ ╚████║╚██████╔╝   .
.   ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝    ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝    .
.    ██████╗ ██╗      █████╗ ████████╗███████╗ ██████╗ ██████╗ ███╗   ███╗    .
.    ██╔══██╗██║     ██╔══██╗╚══██╔══╝██╔════╝██╔═══██╗██╔══██╗████╗ ████║    .
.    ██████╔╝██║     ███████║   ██║   █████╗  ██║   ██║██████╔╝██╔████╔██║    .
.    ██╔═══╝ ██║     ██╔══██║   ██║   ██╔══╝  ██║   ██║██╔══██╗██║╚██╔╝██║    .
.    ██║     ███████╗██║  ██║   ██║   ██║     ╚██████╔╝██║  ██║██║ ╚═╝ ██║    .
.    ╚═╝     ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝      ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝    .
.                                                                             .
$(tput sgr 0)
"

# Delete files matching .gitignore and reset ccache.
./clean.sh  --headless

# The test directory should not use names ending with an underscore. Those names
# are reserved for internal symbols. Two underscores are allowed. This policy
# allows policing changes in internal/external visibility. Symbols ending with
# an underscore are not intended to be used externally and may change without
# notice.
if grep -REn --include='*.cpp' -E '(^|[^[:alnum:]_])[[:alpha:]_][[:alnum:]_]*[[:alnum:]]_([^[:alnum:]_]|$)' test 1>&2; then
  echo "error: Found a C identifier ending with '_' in test/*.cpp." >&2
  exit 1
fi

set -o errexit -o xtrace

time ./debugbuild.sh --headless
time ./testcmake.sh
time ./testcoverage.sh --headless
time ./testerrorhandling.sh
time ./testmatrix.sh
time ./testpythonbindings.sh
time ./teststrip.sh
time ./testwasm.sh --headless
time doxygen

{ set +o xtrace; } 2> /dev/null
./clean.sh
echo All test scripts done.
