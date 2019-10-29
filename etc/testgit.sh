#!/bin/bash
# Copyright 2019 Adrian Johnston
set -o errexit

reset
git pull origin master
git submodule update --init
git submodule foreach git pull origin master

./testdriver.sh

if [[ $(git status --porcelain) ]]; then
    echo "unexpected modifications:"
	git status --porcelain
	echo "try: git clean -f -d"
	exit 1
fi

echo "
$(tput bold)$(tput bold)testgit.sh passed.(tput sgr 0)
"
