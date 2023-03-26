#!/usr/bin/env bash

if [ ! -d /build ]; then
    mkdir build
fi

cd build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ../ && cmake --build .
