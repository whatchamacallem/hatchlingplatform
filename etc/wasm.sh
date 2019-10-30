#!/bin/bash
set -o xtrace
set -o errexit

source ../emsdk/emsdk_env.sh --build=Release

emcc -s WASM=1 -Iinclude -O2 -std=c99 -c src/*.c
emcc -s WASM=1 -Iinclude -O2 -std=c++98 *.o */*.cpp -o hello.html

python -m SimpleHTTPServer
