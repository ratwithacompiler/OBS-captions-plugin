#!/bin/bash

function build_obs() {
  set -e

  echo "setting up src and building OBS in $(pwd)/obs-studio"
  mkdir -p obs-studio && cd obs-studio/

  sudo sudo apt-get update && sudo apt-get install -y \
    build-essential \
    libcurl4-openssl-dev \
    libavcodec-dev libavdevice-dev libavfilter-dev libavformat-dev libavutil-dev \
    libswresample-dev libswscale-dev \
    libjansson-dev \
    libx11-xcb-dev \
    libgles2-mesa-dev \
    libwayland-dev \
    libpulse-dev \
    qt6-base-dev libqt6svg6-dev qt6-base-private-dev

  BUILD_OBS__UNPACKED_DEPS_DIR="$(pwd)/unpacked_deps"
  BUILD_OBS__INSTALLED_DIR="$(pwd)/build_installed"
  BUILD_OBS__SRC_DIR="$(pwd)/src"
  echo "BUILD_OBS__SRC_DIR: $BUILD_OBS__SRC_DIR"
  echo "BUILD_OBS__UNPACKED_DEPS_DIR: $BUILD_OBS__UNPACKED_DEPS_DIR"
  echo "BUILD_OBS__INSTALLED_DIR: $BUILD_OBS__INSTALLED_DIR"

  if [ -e "src/done" ]; then
    echo "obs build done,skipping"
    return
  fi

  if [ ! -e "src" ]; then
    echo getting src
    git clone --single-branch --branch master https://github.com/obsproject/obs-studio.git src
    cd src
    git checkout 28.0.0-beta1
    git submodule update --init --recursive
    cd ..
  fi

  if [ ! -e deps.tar.xz ]; then
    wget -c https://github.com/obsproject/obs-deps/releases/download/2022-08-02/linux-deps-2022-08-02-x86_64.tar.xz -O deps.tar.xz
  fi

  if [ ! -d unpacked_deps ]; then
    mkdir unpacked_deps
    tar -k -xvf deps.tar.xz -C unpacked_deps
  fi

  echo building OBS && pwd
  mkdir -p build_installed
  cd src
  mkdir -p build && cd build && pwd
  $CMAKE \
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
