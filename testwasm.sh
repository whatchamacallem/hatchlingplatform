#!/bin/bash
#
# In the emsdk directory:
# ./emsdk activate latest
# source ./emsdk_env.sh

set -o xtrace
set -o errexit

emcc -Iinclude -O2 -c src/*.c
emcc -Iinclude -O2 *.o */*.cpp -o index.html

echo open http://localhost:9876/
python3 -m http.server 9876
