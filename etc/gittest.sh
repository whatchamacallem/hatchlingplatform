#!/bin/bash
# Copyright 2019 Adrian Johnston

set -o errexit

reset
git pull origin master
git submodule update --init
git submodule foreach git pull origin master

echo "
$(tput rev)                                                                              
    ██╗  ██╗ █████╗ ████████╗ ██████╗██╗  ██╗██╗     ██╗███╗   ██╗ ██████╗    
    ██║  ██║██╔══██╗╚══██╔══╝██╔════╝██║  ██║██║     ██║████╗  ██║██╔════╝    
    ███████║███████║   ██║   ██║     ███████║██║     ██║██╔██╗ ██║██║  ███╗   
    ██╔══██║██╔══██║   ██║   ██║     ██╔══██║██║     ██║██║╚██╗██║██║   ██║   
    ██║  ██║██║  ██║   ██║   ╚██████╗██║  ██║███████╗██║██║ ╚████║╚██████╔╝   
    ╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝    ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝╚═╝  ╚═══╝ ╚═════╝    
     ██████╗ ██╗      █████╗ ████████╗███████╗ ██████╗ ██████╗ ███╗   ███╗    
     ██╔══██╗██║     ██╔══██╗╚══██╔══╝██╔════╝██╔═══██╗██╔══██╗████╗ ████║    
     ██████╔╝██║     ███████║   ██║   █████╗  ██║   ██║██████╔╝██╔████╔██║    
     ██╔═══╝ ██║     ██╔══██║   ██║   ██╔══╝  ██║   ██║██╔══██╗██║╚██╔╝██║    
     ██║     ███████╗██║  ██║   ██║   ██║     ╚██████╔╝██║  ██║██║ ╚═╝ ██║    
     ╚═╝     ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝      ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝    
$(tput sgr 0)                                                                              
$(tput bold)Hatchling Platform$(tput sgr 0)
"

# test configuration options that are not varied by the release level
# during normal testing.
echo "test optional feature settings.  tests will spew errors and still return successfully."
./etc/test.sh -DHX_USE_MEMORY_SCRATCH=0 -DHX_TEST_DIE_AT_THE_END=1

# normal testing.
echo "test successful execution.  no errors expected."
./etc/test.sh

# confirm working directory is clean
if [[ $(git status --porcelain) ]]; then
    echo "unexpected modifications:"
	git status --porcelain
	echo "try: git clean -f -d"
	exit 1
fi
