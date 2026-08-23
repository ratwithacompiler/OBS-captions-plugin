#!/bin/bash

function build_obs() {
  set -e

  echo "setting up src and building OBS in $(pwd)/obs-studio"
  mkdir -p obs-studio && cd obs-studio/

  BUILD_OBS__UNPACKED_DEPS_DIR="$(pwd)/unpacked_deps"
  BUILD_OBS__SRC_DIR="$(pwd)/src"
  BUILD_OBS__BUILD_DIR="$(pwd)/src/build"
  BUILD_OBS__INSTALLED_DIR="$(pwd)/build_installed"
  echo "BUILD_OBS__SRC_DIR: $BUILD_OBS__SRC_DIR"
  echo "BUILD_OBS__UNPACKED_DEPS_DIR: $BUILD_OBS__UNPACKED_DEPS_DIR"
  echo "BUILD_OBS__BUILD_DIR: $BUILD_OBS__BUILD_DIR"
  echo "BUILD_OBS__INSTALLED_DIR: $BUILD_OBS__INSTALLED_DIR"
  test -n OSX_ARCHITECTURES

  # sentinel lives at the obs-studio/ level (not inside src/) so src/ and the
  # deps tarballs can be cleaned away after install, see build_obs_cleanup
  if [ -e "done" ]; then
    echo "obs build done,skipping"
    return
  fi

  if [ ! -e "src" ]; then
    echo getting src
    # shallow tag clone; submodules are plugin-only (browser/websocket/dshow)
    # and never reached with ENABLE_PLUGINS=OFF
    git clone --depth 1 --branch 32.0.0 https://github.com/obsproject/obs-studio.git src
  fi

  if [ ! -e deps.tar.xz ]; then
    wget -c https://github.com/obsproject/obs-deps/releases/download/2025-08-23/macos-deps-2025-08-23-universal.tar.xz -O deps.tar.xz
  fi

  if [ ! -e deps.qt.tar.xz ]; then
    wget -c https://github.com/obsproject/obs-deps/releases/download/2025-08-23/macos-deps-qt6-2025-08-23-universal.tar.xz -O deps.qt.tar.xz
  fi

  if [ ! -d unpacked_deps ]; then
    mkdir unpacked_deps
    tar -k -xvf deps.tar.xz -C unpacked_deps
    tar -k -xvf deps.qt.tar.xz -C unpacked_deps
  fi

  echo building OBS && pwd
  mkdir -p build_installed

  # the 32.x tags only enable_language(Swift) inside the mac-virtualcam plugin
  # (fixed upstream post-32.x by moving it to compilerconfig.cmake), so with
  # ENABLE_PLUGINS=OFF the Swift-only libobs-metal target fails the generate
  # step - inject the language right after project() instead
  echo 'enable_language(Swift)' > enable_swift.cmake
  ENABLE_SWIFT_CMAKE="$(pwd)/enable_swift.cmake"

  cd src
  mkdir -p build && cd build && pwd
  # OBS 32 requires the Xcode generator on macOS (hard configure error otherwise)
  $CMAKE \
    -G Xcode \
    -DCMAKE_PROJECT_INCLUDE="$ENABLE_SWIFT_CMAKE" \
    -DCMAKE_OSX_ARCHITECTURES="$OSX_ARCHITECTURES" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="13.0" \
    -DENABLE_PLUGINS:BOOL=OFF \
    -DENABLE_FRONTEND:BOOL=OFF \
    -DENABLE_SCRIPTING:BOOL=OFF \
    -DOBS_VERSION_OVERRIDE:STRING=32.0.0 \
    -DCMAKE_PREFIX_PATH="$BUILD_OBS__UNPACKED_DEPS_DIR" \
    -DCMAKE_INSTALL_PREFIX:PATH="$BUILD_OBS__INSTALLED_DIR" \
    ..

  $CMAKE --build . --config Release -t obs-frontend-api
  $CMAKE --install . --config Release --component Development

  cd ../..
  du -chd1 && pwd
  touch "done"
}

function build_obs_cleanup() {
  if [ "$CLEAN_OBS" = "1" ] || [ "$CLEAN_OBS" = "true" ]; then
    # src/ (source + build tree) and the deps tarballs are only needed to
    # build OBS itself; the plugin builds against build_installed/ plus
    # unpacked_deps/ (Qt), which survive together with the done sentinel
    if [[ -n "$BUILD_OBS__SRC_DIR" && -d "$BUILD_OBS__SRC_DIR" ]]; then
      echo "cleaning up OBS src+build dir: $BUILD_OBS__SRC_DIR"
      rm -rf "$BUILD_OBS__SRC_DIR" || true
      rm -f "$(dirname "$BUILD_OBS__SRC_DIR")/deps.tar.xz" "$(dirname "$BUILD_OBS__SRC_DIR")/deps.qt.tar.xz" || true
    else
      echo "OBS src dir folder not found: $BUILD_OBS__SRC_DIR"
    fi
  else
    echo "not cleaning OBS src+build, CLEAN_OBS: $CLEAN_OBS"
  fi
}
