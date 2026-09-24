#!/usr/bin/env bash
set -e

echo "[MemStore] Configuring CMake (32-bit i386)..."
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-m32" -DCMAKE_CXX_FLAGS="-m32"

echo "[MemStore] Compiling Release build..."
cmake --build build --config Release -j$(nproc)

echo "[SUCCESS] Binary compiled at: build/memstore_amxx_i386.so"
