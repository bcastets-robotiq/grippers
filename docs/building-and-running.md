# Building and running the example

## Requirements

CMake ≥ 3.16, a C++17 compiler, libserialport. The commands to get
these differ enough between Windows and Linux/macOS — different shell,
different package manager — that they're kept as two separate,
self-contained tracks below rather than one shared command block:

| Platform | Track |
|----------|----------------|
| Ubuntu/Debian, macOS | [Linux and macOS](#linux-and-macos) |
| Windows | [Windows (MSYS2)](#windows-msys2) |

## Linux and macOS

```sh
sudo apt install libserialport-dev   # Ubuntu/Debian
brew install libserialport           # macOS
```

```sh
cmake -S sdk_cpp -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build            # run unit tests, no hardware needed
```

Then run the example:

```sh
./build/examples/move_gripper /dev/ttyUSB0    # macOS: /dev/tty.usbserial-XXXX
```

It activates the gripper (calibration sweep), opens, and closes — keep
the jaws clear. See [Serial port notes](#serial-port-notes) below for
port permissions and latency-timer tuning.

## CMake options

[`sdk_cpp/CMakeLists.txt`](../sdk_cpp/CMakeLists.txt) exposes five
`option()`s. The defaults are right for a normal desktop build (the
commands above don't need to touch any of them); you adjust them when
you're consuming the SDK differently — as a dependency that shouldn't
build its own examples/tests, or on a target that doesn't have a hosted
C++ runtime (see [Embedded / bare-metal builds](embedded-stm32-builds.md)
for that case in detail).

| Option | Default | What it controls |
|---|---|---|
| `GRIPPERS_BUILD_EXAMPLES` | `ON` when top-level, `OFF` via `add_subdirectory()` | Builds `examples/move_gripper`. |
| `GRIPPERS_BUILD_TESTS` | `ON` when top-level, `OFF` via `add_subdirectory()` | Builds and registers the unit tests with CTest. |
| `GRIPPERS_HOSTED` | `ON` | Whether the target has a hosted C++ runtime (`std::thread`, `iostream`). `ON` compiles the `std::thread`-backed `Platform` (`makeDefaultPlatform()`) and the stderr default logger, and links `Threads::Threads`. |
| `GRIPPERS_BUILD_FAKE` | follows `GRIPPERS_HOSTED` | Builds `makeFakeGripper()` and the fake device it drives (see [How it works → Without a gripper](how-it-works.md#without-a-gripper)). ~30 KB; only useful to hosted consumers, since the fake device needs the threaded exchange loop to run. |
| `GRIPPERS_BUILD_DEFAULT_SERIAL` | follows `GRIPPERS_HOSTED` | Builds the libserialport-backed `DefaultSerial` and the `ConnectionConfig`-based constructors that use it. |

"Top-level" means you ran `cmake -S sdk_cpp ...` directly, as in
[Linux and macOS](#linux-and-macos) or [Windows (MSYS2)](#windows-msys2)
above — that's when you get examples and tests by default. If instead your own project pulls the SDK in with
`add_subdirectory(path/to/grippers/sdk_cpp)`, both default to `OFF`
automatically, so your build doesn't also compile *this* repo's example
and test binaries; turn them back on explicitly if you want them anyway
(`-DGRIPPERS_BUILD_TESTS=ON`).

`GRIPPERS_BUILD_FAKE` and `GRIPPERS_BUILD_DEFAULT_SERIAL` both need
`GRIPPERS_HOSTED=ON` — configuring with one of them `ON` while
`GRIPPERS_HOSTED=OFF` is a `FATAL_ERROR`, not a silent downgrade, since
neither can actually be satisfied without the hosted runtime. Similarly,
turning `GRIPPERS_BUILD_DEFAULT_SERIAL` `OFF` on an otherwise hosted
build doesn't error, but examples and tests need it (they exercise the
real serial transport), so they're skipped with a `message(STATUS ...)`
rather than built:

```sh
cmake -S sdk_cpp -B build -DGRIPPERS_BUILD_DEFAULT_SERIAL=OFF
# -- grippers: examples need GRIPPERS_BUILD_DEFAULT_SERIAL; skipping them
# -- grippers: tests need GRIPPERS_BUILD_DEFAULT_SERIAL; skipping them
```

You'd flip that one off, on an otherwise-hosted desktop build, if
you're injecting your own `Serial` (e.g. talking to the gripper through
something other than libserialport) and don't want the libserialport
dependency at all.

## Windows (MSYS2)

Neither vcpkg nor Conan Center packages libserialport, so the supported
Windows toolchain is MSYS2/GCC — the same environment this repo's CI
uses. MSYS2 is a Windows distribution of Unix tooling with `pacman`
(the Arch Linux package manager) and a large repository of prebuilt
native libraries — it's the build environment, not a runtime sandbox:
what it produces is an ordinary native Windows `.exe` (GCC, no emulation
layer), the same kind Visual Studio would produce. MSVC itself isn't
supported, since libserialport ships no MSVC package — a Visual Studio
build would have to compile libserialport from source itself.

1. Install MSYS2 from [msys2.org](https://www.msys2.org)
   (or `winget install MSYS2.MSYS2`).
2. Open the **MSYS2 UCRT64** shell from the Start menu.
3. Install the toolchain and dependencies:

   ```sh
   pacman -Syu
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc \
             mingw-w64-ucrt-x86_64-cmake \
             mingw-w64-ucrt-x86_64-ninja \
             mingw-w64-ucrt-x86_64-libserialport
   ```
4. You will be prompted to close the shell to complete the install. Close it.
5. Open a new **MSYS2 UCRT64** shell. Navigate to the folder where you
   cloned the gripper repository, e.g.:

   ```sh
   cd /c/Users/username/Documents/GitHub/grippers/
   ```
6. Build and test, from that same shell:

   ```sh
   cmake -S sdk_cpp -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j
   ctest --test-dir build
   ```
7. Run the example, still from that same shell:

   ```sh
   ./build/examples/move_gripper.exe COM3   # find the port name in Device Manager
   ```

It activates the gripper (calibration sweep), opens, and closes — keep
the jaws clear.

### Running the built `.exe` outside the MSYS2 shell

The binaries this produces are dynamically linked against a handful of
MinGW/UCRT64 runtime DLLs (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`,
`libwinpthread-1.dll`) and `libserialport-0.dll`, all of which live in
`C:\msys64\ucrt64\bin`. Inside the MSYS2 UCRT64 shell that directory is
already on `PATH`, so step 7 above just works. Run the same `.exe` from
a plain `cmd.exe`/PowerShell window (or double-click it) instead, and
Windows can't find those DLLs — you'll get a
"`libwinpthread-1.dll was not found`"-style error dialog, one DLL at a
time as each missing dependency is hit. This is purely a "where are the
DLLs" problem, not a sign the `.exe` only works inside MSYS2 — it's a
perfectly ordinary native Windows program.

To run it outside MSYS2, either add `C:\msys64\ucrt64\bin` to your
Windows `PATH` (System Properties → Environment Variables), or copy
just the DLLs it needs next to the `.exe` — from the MSYS2 UCRT64
shell:

```sh
cp /ucrt64/bin/{libstdc++-6.dll,libgcc_s_seh-1.dll,libwinpthread-1.dll,libserialport-0.dll} build/examples/
```

### Building a single, portable `.exe` (no DLLs, no MSYS2 needed to run it)

If you'd rather hand someone a single file than a `.exe` plus four
DLLs, link everything statically instead — the MinGW runtime bits with
`-static`, and libserialport itself by pointing at its `.a` archive
(MSYS2 ships both; the plain build above picks the `.dll.a` import
library instead) rather than its `.dll`:

```sh
cmake -S sdk_cpp -B build-static -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DSERIALPORT_LIBRARY=C:/msys64/ucrt64/lib/libserialport.a \
  -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc -static-libstdc++" \
  -DCMAKE_CXX_STANDARD_LIBRARIES="-lsetupapi -lcfgmgr32"
cmake --build build-static -j
```

The two extra system libraries (`setupapi`, `cfgmgr32`) are what
libserialport's Windows backend uses to enumerate ports — normally
pulled in transitively through its `.dll`, so linking its static `.a`
directly needs them spelled out. They go through
`CMAKE_CXX_STANDARD_LIBRARIES` rather than `CMAKE_EXE_LINKER_FLAGS`
because CMake places that variable's contents at the very end of the
link line, after `libserialport.a` — GNU `ld` only resolves a static
archive's symbols against libraries that appear *later* on the command
line, so `-lsetupapi -lcfgmgr32` earlier wouldn't satisfy them.

The result depends on nothing but stock Windows system DLLs (`kernel32`,
`ucrtbase`, `setupapi`, `cfgmgr32`, …) — every one of them already
present on any Windows 10+ machine. Verified by running it with `PATH`
trimmed down to just `C:\Windows\System32`: it still starts up and
talks to the SDK correctly. That's the real answer to "does whoever
runs this need MSYS2 installed": no — copying the four DLLs above is
one way to avoid it, and this static build removes the need for even
that.

Either way, [`sdk_cpp/examples/move_gripper.cpp`](../sdk_cpp/examples/move_gripper.cpp)
— run above on both platforms — is a complete, runnable version of the
five-step walkthrough in [How it works → Usage](how-it-works.md#usage),
plus the argument parsing, logging, and error handling a real program
needs, which are deliberately left out of that walkthrough.

## Building from VS Code

This repo ships a workspace CMake Tools configuration — `.vscode/settings.json`,
`.vscode/cmake-kits.json`, `.vscode/launch.json` — so the editor drives the
same build described above instead of a separate one.

> This setup is for working **on this repo** — building/running/debugging
> `move_gripper` and the unit tests, with `cmake.sourceDirectory` pointing at
> `sdk_cpp/`. If you're instead writing your *own* application that merely
> links against the SDK, most of this doesn't travel with you as-is — see
> [Using the SDK in your own project](using-the-sdk-in-your-project.md)
> for the parts that do (the MSYS2 kit and the `PATH` fix) and don't
> (`cmake.sourceDirectory`, `cmake.debugConfig.args`).

1. Install the recommended extensions — VS Code should prompt for these
   on opening the repo (from `.vscode/extensions.json`): **C/C++** and
   **CMake Tools** (both publisher `ms-vscode`).
2. Command Palette → **CMake: Select a Kit** → **"MSYS2 UCRT64 GCC
   (static exe, no MSYS2 DLLs needed to run)"**. It's defined in
   `.vscode/cmake-kits.json`, pointing at
   `C:\msys64\ucrt64\bin\gcc.exe`/`g++.exe` — if your MSYS2 lives
   somewhere other than the default `C:\msys64`, edit those paths first.
   CMake Tools also auto-detects any Visual Studio install and lists it
   here too; ignore those entries, since libserialport ships no MSVC
   package. (On Linux/macOS, pick whatever kit CMake Tools finds for your
   system compiler instead — nothing here assumes MSYS2 on those platforms.)
3. **CMake: Configure**, then **CMake: Build** (or the matching buttons
   in the status bar at the bottom of the window). If Configure fails
   with "No usable generator found," `.vscode/settings.json`'s
   `cmake.environment.PATH` (which puts `C:\msys64\ucrt64\bin` — and
   `ninja.exe` with it — on the `PATH` CMake Tools itself uses) isn't
   being picked up; run **Developer: Reload Window** and try again.
4. To run `move_gripper`, once it's built: select it as the active
   target in the status bar's target picker (this also happens
   automatically the first time you build it), then click **Run** (▷)
   in the status bar, or open a terminal and run the built `.exe`
   directly — it's the same static, DLL-free binary from
   [above](#building-a-single-portable-exe-no-dlls-no-msys2-needed-to-run-it),
   so it works from a plain terminal with no MSYS2 setup either way.
   Either way it needs the port as an argument (e.g. `COM3`) — set it
   once in `cmake.debugConfig.args` in `.vscode/settings.json` and the
   status bar's Run/Debug buttons will pass it automatically, or type
   it after the `.exe` path in the terminal. Skip hardware entirely by
   pointing a small scratch target at
   [`makeFakeGripper()`](how-it-works.md#without-a-gripper) instead.
5. For breakpoint debugging (the bug-icon **Debug** button, or `F5`
   against `.vscode/launch.json`'s "Debug move_gripper" configuration),
   install `gdb` first — MSYS2 doesn't include it by default:
   ```sh
   pacman -S --needed mingw-w64-ucrt-x86_64-gdb
   ```

The kit's `cmakeSettings` (in `cmake-kits.json`) apply the same static
linking flags as [above](#building-a-single-portable-exe-no-dlls-no-msys2-needed-to-run-it),
so both running/debugging from inside VS Code and running the built
`.exe` afterward from a plain terminal work without any DLL setup.

If you're writing a separate application that uses this SDK rather
than working on the SDK itself, see
[Using the SDK in your own project](using-the-sdk-in-your-project.md)
instead — it covers wiring the SDK into your own `CMakeLists.txt` and
setting up your own project's IDE, building on the pieces above.

## Serial port notes

- **Linux**: add yourself to the `dialout` group for `/dev/ttyUSB*` access.
  The SDK sets the FTDI `latency_timer` to 1 ms automatically when it has
  permission (the kernel default of 16 ms triples Modbus latency); for
  unprivileged use, ship a udev rule that sets it at plug time.
- **Windows**: the FTDI latency timer is a driver setting (Device Manager →
  COM port → Port Settings → Advanced → Latency Timer); set it to 1 ms for
  high-rate control.
- **macOS**: the FTDI latency timer defaults to 16 ms — capping the exchange
  rate near ~60 Hz — and macOS offers no way to lower it from the SDK. To run
  faster, install [FTDI's VCP driver](https://ftdichip.com/drivers/vcp-drivers/)
  and set its `LatencyTimer` to `1` (in the driver's `Info.plist`); it then
  applies to every open, including this SDK's. On macOS 11+ also approve the
  driver in System Settings → Privacy & Security and make sure it — not
  Apple's built-in FTDI driver — binds your adapter (`kextstat | grep -i ftdi`).
  Otherwise ~60 Hz is the ceiling on the default driver.
- Factory-default link settings: 115200 baud, 8N1, Modbus slave 0x09.
- Port naming: `/dev/ttyUSB0` on Linux, `COM3` on Windows,
  `/dev/tty.usbserial-XXXX` on macOS.
- **Windows**: thread pacing is quantized by the OS timer (default tick
  ~15.6 ms), so exchange periods shorter than ~16 ms will run slower
  than configured. High-rate control on Windows is currently untuned —
  open an issue if your application needs it.
