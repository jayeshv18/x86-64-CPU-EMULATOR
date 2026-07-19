cmake_minimum_required(VERSION 4.2)
project(x86_64_Emulator C)

set(CMAKE_C_STANDARD 11)

add_executable(x86_64_Emulator main_wasm_version.c
        include/cpu.h
        cpu.c
        main_terminal_hardcoded_version.c
        emsdk/main_wasm_emscripten_version.c)
