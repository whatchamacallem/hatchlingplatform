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
FILE_RELATIVE=${FILE_ABSOLUTE#"$HX_DIR/"}

TMP_ID=$(git rev-parse --short $COMMIT_ID)
TMP_FILE=$(mktemp ~/.cache/$TMP_ID.XXXX)

if ! git -C "$HX_DIR" show "$COMMIT_ID:$FILE_RELATIVE" > "$TMP_FILE"; then
	echo 'error: unable to read file from the specified commit.' >&2
	exit 1
fi

code --diff "$TMP_FILE" "$FILE_ABSOLUTE"
