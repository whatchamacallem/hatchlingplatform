#!/bin/sh
#
# sudo apt install python3 python3-clang nanobind-dev
#

set -o errexit

# Smoke test.
if python3 -c 'import clang'; then
    echo "tested python3-clang..."
else
    echo "unable to load python module clang.  see script for instructions."
    exit 1
fi

PY_CFLAGS="$(python3-config --cflags)"
PY_LDFLAGS="$(python3-config --ldflags) -lpython3.12" # wtf this library was missing.

PY_BIND="/usr/share/nanobind"
HX_DIR=`pwd`

# Only nuke the bin dir if it is not being used for binding already.
if [ ! -f "bin/python_bindings.cpp" ]; then
	rm -rf ./bin; mkdir ./bin
fi
cd ./bin

set -x

# Check timestamps and regenerate the bindings if they have changed.
python3 $HX_DIR/py/generate_bindings.py $HX_DIR/include/hx/hatchling_pch.hpp python_bindings.cpp

clang -I$HX_DIR/include -DHX_RELEASE=0 -std=c17 -I$PY_BIND/include $PY_CFLAGS \
    -fdiagnostics-absolute-paths -Wfatal-errors -c $HX_DIR/src/*.c $HX_DIR/test/*.c

# picks up python_bindings.cpp automatically from ./*/*.cpp.
clang++ -I$HX_DIR/include -I$PY_BIND/include -I. $PY_CFLAGS $PY_LDFLAGS \
    -DHX_RELEASE=0 -pthread -lpthread -std=c++17 -lstdc++ \
    -fdiagnostics-absolute-paths -Wfatal-errors \
    $PY_BIND/src/*.cpp $HX_DIR/*/*.cpp *.o -o hxtest

+ clang++ -I/home/t/Documents/hatchlingplatform/include -I/usr/share/nanobind/include -I. -I/usr/include/python3.12 -I/usr/include/python3.12 -fno-strict-overflow -Wsign-compare -DNDEBUG -g -O2 -Wall -L/usr/lib/python3.12/config-3.12-x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu -ldl -lm -lpython3.12 -DHX_RELEASE=0 -pthread -lpthread -std=c++17 -lstdc++ -fdiagnostics-absolute-paths -Wfatal-errors /usr/share/nanobind/src/common.cpp /usr/share/nanobind/src/error.cpp /usr/share/nanobind/src/implicit.cpp /usr/share/nanobind/src/nb_enum.cpp /usr/share/nanobind/src/nb_func.cpp /usr/share/nanobind/src/nb_internals.cpp /usr/share/nanobind/src/nb_ndarray.cpp /usr/share/nanobind/src/nb_static_property.cpp /usr/share/nanobind/src/nb_type.cpp /usr/share/nanobind/src/trampoline.cpp /home/t/Documents/hatchlingplatform/bin/python_bindings.cpp /home/t/Documents/hatchlingplatform/src/hatchling.cpp /home/t/Documents/hatchlingplatform/src/hxconsole.cpp /home/t/Documents/hatchlingplatform/src/hxfile.cpp /home/t/Documents/hatchlingplatform/src/hxmemory_manager.cpp /home/t/Documents/hatchlingplatform/src/hxprofiler.cpp /home/t/Documents/hatchlingplatform/src/hxsettings.cpp /home/t/Documents/hatchlingplatform/src/hxsort.cpp /home/t/Documents/hatchlingplatform/src/hxstring_literal_hash.cpp /home/t/Documents/hatchlingplatform/src/hxtask_queue.cpp /home/t/Documents/hatchlingplatform/test/hxarray_test.cpp /home/t/Documents/hatchlingplatform/test/hxconsole_test.cpp /home/t/Documents/hatchlingplatform/test/hxfile_test.cpp /home/t/Documents/hatchlingplatform/test/hxhash_table_test.cpp /home/t/Documents/hatchlingplatform/test/hxmemory_manager_test.cpp /home/t/Documents/hatchlingplatform/test/hxprofiler_test.cpp /home/t/Documents/hatchlingplatform/test/hxrandom_test.cpp /home/t/Documents/hatchlingplatform/test/hxsort_test.cpp /home/t/Documents/hatchlingplatform/test/hxstring_hash_test.cpp /home/t/Documents/hatchlingplatform/test/hxtask_queue_test.cpp /home/t/Documents/hatchlingplatform/test/hxtest_main.cpp /home/t/Documents/hatchlingplatform/test/hxthread_test.cpp hxc_utils.o hxctest.o -o hxtest
