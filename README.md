# Kestrel

A framework for running classic games natively on modern systems by
reimplementing their engines, in the spirit of ScummVM. Games are not
included: you point Kestrel at your own installed copy.

The name is a placeholder.

## Layout

```
src/common/        platform-independent core: System interface, events,
                   surfaces, config, case-insensitive file lookup, detection
src/backends/sdl3/ the only code that touches SDL (window, rendering, input, audio)
src/launcher/      Dear ImGui game list: add / remove / start games
src/engines/       one directory per engine, plus registry.cpp listing them
  testbed/         built-in test "game" that exercises the backend
  newworlds/       Star Trek: New Worlds (stub; detection table empty)
tests/             unit tests for common/
```

Engines only use `common/` and `engines/engine.h`. They never include SDL.

## Building

Needs CMake 3.24+ and a C++20 compiler. SDL3 and Dear ImGui are downloaded
and built automatically.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/kestrel
```

On Linux, install SDL's build dependencies first (X11/Wayland, audio
headers): https://wiki.libsdl.org/SDL3/README-linux#build-dependencies.
To use an installed SDL3 instead, add `-DKESTREL_USE_SYSTEM_SDL=ON`.

## Usage

- `kestrel` opens the launcher. *Add Game...* scans a folder for supported games.
- `kestrel <target>` starts a configured game directly. `kestrel testbed`
  runs the test pattern.
- `kestrel --hash <files...>` prints detection signatures for an engine's table.
- In game: **Ctrl+F5** returns to the launcher, **Alt+Enter** toggles fullscreen.

The config is saved as `kestrel.ini` in SDL's preferences folder
(`~/.local/share/kestrel/kestrel/` on Linux, `%APPDATA%\kestrel\kestrel\` on Windows).

## Adding an engine

1. Create `src/engines/<name>/` with an `Engine` and a `MetaEngine` (copy `testbed`).
2. Add a detection table built with `kestrel --hash`.
3. Register its accessor in `src/engines/registry.cpp` and add the source to
   `CMakeLists.txt`.

Read [CONTRIBUTING.md](CONTRIBUTING.md) first: only clean-room work is accepted.
