#!/usr/bin/env bash
# SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
# SPDX-License-Identifier: MIT
# This file is licensed under the terms of the LICENSE.md file.
#
# Displays git diffs using vscode and bash.
#
# usage: %s <file> <commit_id>

set -euo pipefail

if [ $# -ne 2 ]; then
	printf 'usage: %s <file> <commit_id>\n' "$0" >&2
	exit 1
fi

FILE_ARG=$1
COMMIT_ID=$2

if ! command -v code >/dev/null 2>&1; then
	echo 'error: vscode command-line tool "code" not found in \$PATH.' >&2
	exit 1
fi

HX_DIR=$(git rev-parse --show-toplevel)
if [ -z "$HX_DIR" ]; then
	echo 'error: not inside a git repository.' >&2
	exit 1
fi

if [ ! -e "$FILE_ARG" ]; then
	printf 'error: file "%s" does not exist.\n' "$FILE_ARG" >&2
	exit 1
fi

FILE_ABSOLUTE=$(realpath -- "$FILE_ARG")
rel_file=${FILE_ABSOLUTE#"$HX_DIR/"}

tmp_file=$(mktemp ~/.cache/hx-diff.sh.XXXXXX)

if ! git -C "$HX_DIR" show "$COMMIT_ID:$rel_file" > "$tmp_file"; then
	echo 'error: unable to read file from the specified commit.' >&2
	exit 1
fi

code --diff "$tmp_file" "$FILE_ABSOLUTE"
