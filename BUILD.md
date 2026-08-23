# Building

Local builds use the exact same scripts as CI. Everything lives under `CI/` and builds into `CI/<variant>/CI_build/` — no system-wide installs except the Linux apt step below.

## Variants

- `CI/http` — default; the older full-duplex HTTP speech API.
- `CI/grpc` — Google Cloud Speech over gRPC (vcpkg-built grpc stack, much longer first-time setup).

Both work the same way, just run the scripts from the variant dir you want.

## Stages

Each variant has per-OS scripts in `linux/`, `macos/`, `windows/`:

- `setup_deps` — downloads and builds all dependencies (OBS 32.0.0 libs — plus obs-deps tarballs on macOS/Windows, apt packages on Linux; a pinned plibsys clone for HTTP (patched on Windows only, `CI/http/plibsys-socket-wakeup.patch`); vcpkg grpc + googleapis codegen for GRPC). Run once; re-runs skip everything already built.

  Upgrading from an older checkout that built OBS 30? Delete `CI/<variant>/CI_build/build_deps/obs-studio` once — a stale sentinel there would silently keep the old OBS.
- `build` — configures and builds the plugin itself. This is the dev loop: edit code, rerun `build`.
- `post_build` — packages the release zip into `CI_build/release/`. `build --package` runs both.

The scripts can be run from any directory; they change to the right place themselves.

## Prerequisites

The plugin builds OBS 32.0.0 from source as part of `setup_deps`, so the machine needs whatever OBS itself needs to compile. The quick per-platform version:

- **Linux**: Ubuntu 24.04 or newer (or equivalent — OBS 32 needs FFmpeg >= 6.1, CMake >= 3.28, so older LTS releases won't work). On Ubuntu/Debian, `CI/linux_install_syswide_tools.sh` installs everything in one go: compilers, CMake helpers (extra-cmake-modules), the ffmpeg/x11/wayland/GL dev libraries OBS needs, and the Qt6 dev libraries the plugin needs. It is the only system-wide-affecting step; nothing else touches the system. On other distros, install the equivalent packages by hand (package names are in that script).
- **macOS**: full Xcode >= 16 with the macOS 15 SDK (OBS 32 hard-requires the Xcode generator and that SDK — Command Line Tools alone are not enough), plus `cmake`, `git`, `wget` (e.g. via Homebrew). OBS's own library dependencies and Qt6 are NOT needed system-wide — `setup_deps` downloads the prebuilt obs-deps tarballs locally into `CI_build/`.
- **Windows**: Visual Studio 2022 (C++ workload), CMake, git, Python 3, 7-Zip on PATH. Like macOS, all OBS libraries and Qt6 come from prebuilt obs-deps zips downloaded into `CI_build/` — nothing system-wide.

For background on OBS's own build requirements (full dependency lists per platform), see the official OBS build instructions: [Linux](https://github.com/obsproject/obs-studio/wiki/build-instructions-for-linux), [macOS](https://github.com/obsproject/obs-studio/wiki/Build-Instructions-For-Mac), [Windows](https://github.com/obsproject/obs-studio/wiki/build-instructions-for-windows). Note we build far less than full OBS (`libobs` + `obs-frontend-api` only, no frontend/plugins/browser), so the lists there are a superset of what's actually required here.

## Commands

HTTP variant shown; use `cd CI/grpc` for GRPC.

```bash
# Linux
cd CI/http
../linux_install_syswide_tools.sh   # once per machine
./linux/setup_deps.sh               # once, slow
./linux/build.sh --package

# macOS (universal x86_64+arm64 by default; TARGET_ARCH=arm64 for single-arch)
cd CI/http
./macos/setup_deps.sh
./macos/build.sh --package
```

```bat
:: Windows
cd CI\http
python windows\setup_deps.py
python windows\build.py --package
```

Day-to-day development after the one-time `setup_deps`: just rerun `./linux/build.sh` (or the macos/windows equivalent) — it skips the dependency builds and only recompiles the plugin.

## Environment variables

- `GOOGLE_API_KEY` — set to bake the key into the binary and hide the UI field; leave empty to build with the in-UI API key field.
- `TARGET_ARCH` — macOS only, e.g. `arm64` to skip the universal build.
- `CLEAN_OBS=0` / `CLEAN_VCPKG=0` — keep intermediate build dirs that `setup_deps` deletes by default to save space. (OBS never needs rebuilding after the first `setup_deps`: the cleanup keeps the small installed OBS libs and removes only the OBS source/build tree.)

## Output

- `CI/<variant>/CI_build/release/` — packaged zips, same layout as CI artifacts.
- `CI/<variant>/CI_build/installed/` — the raw built plugin.
