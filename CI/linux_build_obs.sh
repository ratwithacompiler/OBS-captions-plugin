#!/bin/bash

function build_obs() {
  set -e

  echo "setting up src and building OBS in $(pwd)/obs-studio"
  mkdir -p obs-studio && cd obs-studio/

  BUILD_OBS__SRC_DIR="$(pwd)/src"
  BUILD_OBS__BUILD_DIR="$(pwd)/src/build"
  BUILD_OBS__INSTALLED_DIR="$(pwd)/build_installed"
  echo "BUILD_OBS__SRC_DIR: $BUILD_OBS__SRC_DIR"
  echo "BUILD_OBS__BUILD_DIR: $BUILD_OBS__BUILD_DIR"
  echo "BUILD_OBS__INSTALLED_DIR: $BUILD_OBS__INSTALLED_DIR"

  # sentinel lives at the obs-studio/ level (not inside src/) so src/ can be
  # cleaned away entirely after install, see build_obs_cleanup
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

  # OBS 32: obs-deps publishes no linux tarballs anymore; all OBS build deps
  # come from apt via CI/linux_install_syswide_tools.sh (build scripts
  # themselves never install anything system-wide)

  echo building OBS && pwd
  mkdir -p build_installed
  cd src
  mkdir -p build && cd build && pwd
  $CMAKE \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_PLUGINS:BOOL=OFF \
    -DENABLE_FRONTEND:BOOL=OFF \
    -DENABLE_SCRIPTING:BOOL=OFF \
    -DOBS_VERSION_OVERRIDE:STRING=32.0.0 \
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
    # src/ (source + build tree) is only needed to build OBS itself; the plugin
    # builds against build_installed/ alone, which survives together with the
    # done sentinel
    if [[ -n "$BUILD_OBS__SRC_DIR" && -d "$BUILD_OBS__SRC_DIR" ]]; then
      echo "cleaning up OBS src+build dir: $BUILD_OBS__SRC_DIR"
      rm -rf "$BUILD_OBS__SRC_DIR" || true
    else
      echo "OBS src dir folder not found: $BUILD_OBS__SRC_DIR"
    fi
  else
    echo "not cleaning OBS src+build, CLEAN_OBS: $CLEAN_OBS"
  fi
}
