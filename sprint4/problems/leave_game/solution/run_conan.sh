#!/bin/bash

conan install .. -s compiler.libcxx=libstdc++11 -s build_type=Debug --build=missing
cmake .. -DCMAKE_BUILD_TYPE=Debug