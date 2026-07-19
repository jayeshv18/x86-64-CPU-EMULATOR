#!/bin/bash
# Activate Emscripten environment
source emsdk/emsdk_env.sh

# Compile into WebAssembly
emcc cpu.c main_wasm_emscripten_version.c -o emulator.js \
  -s EXPORTED_FUNCTIONS='["_init_emulator", "_load_instruction", "_step_emulator", "_get_rax", "_get_rbx", "_get_rcx", "_get_rdx", "_get_rsp", "_get_rbp", "_get_rip", "_get_zf", "_get_cf", "_get_sf", "_get_of", "_get_memory_ptr"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -s NO_EXIT_RUNTIME=1

echo "WebAssembly compiled successfully!"
