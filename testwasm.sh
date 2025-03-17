#!/bin/bash
#
# In the emsdk directory:
# ./emsdk activate latest
# source ./emsdk_env.sh

set -o xtrace
set -o errexit

emcc -Iinclude -O2 -c src/*.c
emcc -Iinclude -O2 *.o */*.cpp -o hello.html

echo http://localhost:9876/hello.html
python3 -m http.server 9876
