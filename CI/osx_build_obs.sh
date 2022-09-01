#!/bin/bash

function build_obs() {
  set -e

  echo "setting up src and building OBS in $(pwd)/obs-studio"
  mkdir -p obs-studio && cd obs-studio/

  BUILD_OBS__UNPACKED_DEPS_DIR="$(pwd)/unpacked_deps"
  BUILD_OBS__INSTALLED_DIR="$(pwd)/build_installed"
  BUILD_OBS__SRC_DIR="$(pwd)/src"
  BUILD_OBS__BUILD_DIR="$(pwd)/src/build"
  echo "BUILD_OBS__SRC_DIR: $BUILD_OBS__SRC_DIR"
  echo "BUILD_OBS__UNPACKED_DEPS_DIR: $BUILD_OBS__UNPACKED_DEPS_DIR"
  echo "BUILD_OBS__INSTALLED_DIR: $BUILD_OBS__INSTALLED_DIR"
  echo "BUILD_OBS__BUILD_DIR: $BUILD_OBS__BUILD_DIR"
  test -n OSX_ARCHITECTURES

  if [ -e "src/done" ]; then
    echo "obs build done,skipping"
    return
  fi

  if [ ! -e "src" ]; then
    echo getting src
    git clone --single-branch --branch master https://github.com/obsproject/obs-studio.git src
    cd src
    git checkout 28.0.0
    git submodule update --init --recursive
    cd ..
  fi

  if [ ! -e deps.tar.xz ]; then
    wget -c https://github.com/obsproject/obs-deps/releases/download/2022-08-02/macos-deps-2022-08-02-universal.tar.xz -O deps.tar.xz
  fi

  if [ ! -e deps.qt.tar.xz ]; then
    wget -c https://github.com/obsproject/obs-deps/releases/download/2022-08-02/macos-deps-qt6-2022-08-02-universal.tar.xz -O deps.qt.tar.xz
  fi

  if [ ! -d unpacked_deps ]; then
    mkdir unpacked_deps
    tar -k -xvf deps.tar.xz -C unpacked_deps
    tar -k -xvf deps.qt.tar.xz -C unpacked_deps
  fi

  echo building OBS && pwd
  mkdir -p build_installed
  cd src
  mkdir -p build && cd build && pwd
  $CMAKE \
    -DCMAKE_OSX_ARCHITECTURES="$OSX_ARCHITECTURES" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_BROWSER=OFF \
    -DENABLE_PLUGINS=OFF \
    -DENABLE_UI=OFF \
    -DENABLE_SCRIPTING=OFF \
    -DQT_VERSION=6 \
    -DCMAKE_PREFIX_PATH="$BUILD_OBS__UNPACKED_DEPS_DIR" \
    -DCMAKE_INSTALL_PREFIX:PATH="$BUILD_OBS__INSTALLED_DIR" \
    ..

  $CMAKE --build . --config Release -t obs-frontend-api
  $CMAKE --install . --config Release --component obs_libraries

  cd ../
  du -chd1 && pwd
  touch "done"
}

function build_obs_cleanup() {
  if [ "$CLEAN_OBS" = "1" ] || [ "$CLEAN_OBS" = "true" ]; then
    if [[ -n "$BUILD_OBS__BUILD_DIR" && -d "$BUILD_OBS__BUILD_DIR" ]]; then
      echo "cleaning up OBS BUILD dir: $BUILD_OBS__BUILD_DIR"
      rm -rf "$BUILD_OBS__BUILD_DIR" || true
    else
      echo "OBS BUILD dir folder not found: $BUILD_OBS__BUILD_DIR"
    fi
  else
    echo "not cleaning OBS build, CLEAN_OBS: $CLEAN_OBS"
  fi
}
