# Using the SDK in your own project

This page is for writing an application that *uses* the SDK — as
opposed to [Building and running the example](building-and-running.md),
which is about working on the SDK itself (its own example, its own
tests, its own `.vscode/` setup). If you haven't yet, skim
[How it works](how-it-works.md) first for the API you'll actually be
calling from your code.

## 1. Bring the SDK into your build

Two ways to do this, and they trade off differently:

- **Vendor the source and `add_subdirectory()` it — the default to
  start with.** Add this repo (or just `sdk_cpp/`) as a git submodule,
  or a plain vendored copy, inside your own project, then wire it into
  your own `CMakeLists.txt`:

  ```cmake
  add_subdirectory(third_party/grippers/sdk_cpp)
  target_link_libraries(your_app PRIVATE Robotiq::grippers)
  ```

  CMake then treats the SDK as part of *your* build: one configure,
  one build, one compiler and one set of flags for both your code and
  the SDK's. That matters more than it sounds — a static library built
  with a different compiler, C++ runtime, or CMake option
  (`GRIPPERS_HOSTED`, `BUILD_SHARED_LIBS`, Debug vs. Release, …) than
  whatever links it can fail to link, or link cleanly and still
  misbehave at runtime. Vendoring removes that failure mode by
  construction — the cost is the SDK recompiling whenever you build
  from clean, which for a project this size is not a lot of time.

- **Build the SDK once, install it, and `find_package()` it —** for
  when you don't want to pay that recompile cost on every consuming
  project or CI machine (e.g. distributing one prebuilt SDK across a
  team). In exchange, you take on keeping the installed copy's build
  configuration consistent with whatever consumes it.

  ```sh
  cmake -S sdk_cpp -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j
  cmake --install build --prefix /path/to/some/install/dir
  ```

  `cmake --install` copies the compiled library, the public headers,
  and the CMake package files (`grippers-config.cmake` etc., from
  [`sdk_cpp/CMakeLists.txt`](../sdk_cpp/CMakeLists.txt)'s `install()`
  calls) into that prefix. Omit `--prefix` to use CMake's system
  default (e.g. `/usr/local` on Linux/macOS) if that's fine.

  Then, in the *consuming* project — no SDK source needs to be present
  at all:

  ```cmake
  find_package(grippers REQUIRED)
  target_link_libraries(your_app PRIVATE Robotiq::grippers)
  ```
  ```sh
  cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/some/install/dir
  ```

  `find_package(grippers)` is what resolves `grippers-config.cmake`
  from that prefix into the same `Robotiq::grippers` target either
  option gives you — `target_link_libraries` doesn't change.

Unless you already know you need the second one, start with vendoring.

See [Building and running the example → CMake options](building-and-running.md#cmake-options)
for what `GRIPPERS_HOSTED` and the other build-configuration options
actually do — they apply the same way whichever path above you take.

### Static vs. shared

By default, `add_library(grippers_sdk)` in
[`sdk_cpp/CMakeLists.txt`](../sdk_cpp/CMakeLists.txt) follows CMake's
usual default and builds a *static* library (`.a` / `.lib`) — it gets
copied into your own binary at link time, so there's no `.so`/`.dll` to
ship or find at runtime, but you need to relink (not just re-run) after
the SDK changes. Configure with `-DBUILD_SHARED_LIBS=ON` (works the
same for either option above) if you'd rather build `grippers` as a
shared library instead.

## 2. Set up your IDE

The same shape as [Building from VS Code](building-and-running.md#building-from-vs-code)
for the SDK's own repo, just pointed at your project instead of `sdk_cpp/`:

1. Same extensions: **C/C++** and **CMake Tools** (both `ms-vscode`).
2. `cmake.sourceDirectory` should point wherever *your*
   `CMakeLists.txt` lives — usually your workspace root, in which case
   you don't need to set it at all.
3. **Windows only** — you still need the MSYS2 GCC toolchain, since
   [libserialport ships no MSVC package](building-and-running.md#windows-msys2).
   Rather than committing a kit file per-project, register it once via
   Command Palette → **CMake: Edit User-Local CMake Kits**, so it's
   offered in every workspace on your machine:

   ```json
   {
     "name": "MSYS2 UCRT64 GCC (static exe, no MSYS2 DLLs needed to run)",
     "compilers": {
       "C": "C:/msys64/ucrt64/bin/gcc.exe",
       "CXX": "C:/msys64/ucrt64/bin/g++.exe"
     },
     "preferredGenerator": { "name": "Ninja" },
     "cmakeSettings": {
       "SERIALPORT_LIBRARY": "C:/msys64/ucrt64/lib/libserialport.a",
       "CMAKE_EXE_LINKER_FLAGS": "-static -static-libgcc -static-libstdc++",
       "CMAKE_CXX_STANDARD_LIBRARIES": "-lsetupapi -lcfgmgr32"
     }
   }
   ```

   Same reasoning as [Building a single, portable `.exe`](building-and-running.md#building-a-single-portable-exe-no-dlls-no-msys2-needed-to-run-it):
   this links your app statically against libserialport instead of its
   `.dll`, so what you build runs on any Windows machine with no MSYS2
   or its DLLs installed. Also add the matching `PATH` fix — this
   time in your **user** settings (Command Palette → **Preferences:
   Open User Settings (JSON)**) rather than a per-project one, since
   where MSYS2 lives is a fact about your machine, not any one project:

   ```json
   "cmake.environment": { "PATH": "C:\\msys64\\ucrt64\\bin;${env:PATH}" }
   ```

   Without it, **CMake: Configure** fails with "No usable generator
   found" — CMake Tools can't find `ninja.exe` on `PATH`.
4. Configure, build, and run/debug your own target the same way — set
   your program's own command-line arguments via
   `cmake.debugConfig.args` in *your* project's `.vscode/settings.json`.

You can develop and test without a physical gripper at all: call
[`makeFakeGripper()`](how-it-works.md#without-a-gripper) instead of
constructing a `Gripper` from a `ConnectionConfig` — same API from
there on, so swapping in a real gripper later is a one-line change.

## 3. Updating the SDK later

- **Vendored:** update the submodule pointer (or re-copy the source),
  then rebuild — no separate reinstall step.
- **Installed:** re-run the build/install steps from step 1 against
  the new version, into the same prefix.
