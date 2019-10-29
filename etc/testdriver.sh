#!/bin/bash
# Copyright 2019 Adrian Johnston
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
./etc/test.sh '-DHX_MEM_DIAGNOSTIC_LEVEL=(2-HX_RELEASE)' -DHX_USE_MEMORY_SCRATCH=0 -DHX_TEST_DIE_AT_THE_END=1

# normal testing.
echo "now testing successful execution.  no errors expected."
./etc/test.sh

echo "
$(tput bold)$(tput bold)testdriver.sh passed.(tput sgr 0)
"
