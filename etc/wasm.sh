#!/bin/bash
set -o xtrace
set -o errexit

emcc -Iinclude -O2 -c src/*.c
emcc -Iinclude -O2 *.o */*.cpp -o hello.html

echo http://localhost:9876/hello.html
python3 -m http.server 9876
