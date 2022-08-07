#!/bin/bash

function build_obs() {
  set -e

  echo "setting up src and building OBS in $(pwd)/obs-studio"
  mkdir -p obs-studio && cd obs-studio/

  BUILD_OBS__UNPACKED_DEPS_DIR="$(pwd)/unpacked_deps"
  BUILD_OBS__INSTALLED_DIR="$(pwd)/build_installed"
  BUILD_OBS__SRC_DIR="$(pwd)/src"
  echo "BUILD_OBS__SRC_DIR: $BUILD_OBS__SRC_DIR"
  echo "BUILD_OBS__UNPACKED_DEPS_DIR: $BUILD_OBS__UNPACKED_DEPS_DIR"
  echo "BUILD_OBS__INSTALLED_DIR: $BUILD_OBS__INSTALLED_DIR"
  test -n OSX_ARCHITECTURES

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

function make_release_zip() {
  set -e

  cp -v build/libobs_google_caption_plugin.so ./
  local libfile="libobs_google_caption_plugin.so"

  #make rpaths relative, OBS 28+
  otool -L "$libfile"

  install_name_tool \
    -change @rpath/QtWidgets.framework/Versions/A/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/A/QtWidgets \
    -change @rpath/QtGui.framework/Versions/A/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/A/QtGui \
    -change @rpath/QtCore.framework/Versions/A/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore \
    -change @rpath/libobs.framework/Versions/A/libobs @executable_path/../Frameworks/libobs.framework/Versions/A/libobs \
    -change @rpath/libobs-frontend-api.1.dylib @executable_path/../Frameworks/libobs-frontend-api.dylib \
    "$libfile"
  otool -L "$libfile"

  # ensure it worked
  otool -L "$libfile" | grep -q '@executable_path/../Frameworks/QtGui.framework/Versions/A/QtGui'
  otool -L "$libfile" | grep -q '@executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore'
  otool -L "$libfile" | grep -q '@executable_path/../Frameworks/QtWidgets.framework/Versions/A/QtWidgets'
  otool -L "$libfile" | grep -q '@executable_path/../Frameworks/libobs.framework/Versions/A/libobs'
  otool -L "$libfile" | grep -q '@executable_path/../Frameworks/libobs-frontend-api.dylib'

  local RELEASE_NAME="Closed_Captions_Plugin__v""$VERSION_STRING""_MacOS"
  local RELEASE_FOLDER="release/$RELEASE_NAME/cloud_captions_plugin/bin"

  mkdir -p "$RELEASE_FOLDER"
  cp -v libobs_google_caption_plugin.so "$RELEASE_FOLDER"/cloud_captions_plugin.so

  cd release
  zip -r "$RELEASE_NAME".zip "$RELEASE_NAME"
  cd ..
  du -chd1 && df -lh
  ls -l "$RELEASE_FOLDER"
}
