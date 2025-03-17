#!/bin/bash
# Copyright 2019 Adrian Johnston
set -o errexit

./testdriver.sh

if [[ $(git status --porcelain) ]]; then
    echo "unexpected modifications:"
	git status --porcelain
	echo "try: git clean -f -d"
	exit 1
fi

echo testgit.sh passed.
