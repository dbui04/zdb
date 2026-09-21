#!/bin/bash
cmake -S . -B build
cmake --build build --target server client --parallel
