#!/bin/bash
# Copyright 2017-2025 Adrian Johnston
set -o errexit

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

# test configuration options in ways that are not varied by the release level
# during normal testing.
echo "testing optional feature settings.  these tests will spew errors and still return successfully."
./test.sh '-DHX_MEM_DIAGNOSTIC_LEVEL=(2-HX_RELEASE)' -DHX_USE_MEMORY_SCRATCH=0 \
               -DHX_TEST_DIE_AT_THE_END=1 -DHX_TEST_MAX_CASES=300

# normal testing.
echo "now testing successful execution.  no errors expected."
./test.sh
