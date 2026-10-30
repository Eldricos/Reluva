# Cross-compiling Reluva for 64-bit Windows from Linux with MinGW-w64
# (Debian/Ubuntu: `apt install mingw-w64`), e.g. to try it under Wine:
#
#   cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake
#   cmake --build build-win
#   wine build-win/reluva_tests.exe
#
# The -posix compilers are used because the -win32 ones have no std::thread.
# libgcc, libstdc++ and winpthread are linked statically, so the .exe files
# run without any MinGW DLLs beside them.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(RELUVA_MINGW_PREFIX x86_64-w64-mingw32)
set(CMAKE_C_COMPILER ${RELUVA_MINGW_PREFIX}-gcc-posix)
set(CMAKE_CXX_COMPILER ${RELUVA_MINGW_PREFIX}-g++-posix)
set(CMAKE_RC_COMPILER ${RELUVA_MINGW_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${RELUVA_MINGW_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")

# ctest runs the tests through Wine when it's installed.
find_program(RELUVA_WINE wine)
if(RELUVA_WINE)
    set(CMAKE_CROSSCOMPILING_EMULATOR ${RELUVA_WINE})
endif()
