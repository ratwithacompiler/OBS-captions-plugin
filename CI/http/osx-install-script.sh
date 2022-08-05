#!/bin/bash

set -e
ROOT_DIR="$(pwd)"
VERSION_STRING="$(cat ../../CMakeLists.txt | egrep VERSION_STRING | egrep set | sed 's/ //g' | egrep -o '(VERSION_STRING"(.+)")' | egrep -o '".+"' | sed 's/"//g')"
echo "VERSION_STRING: $VERSION_STRING"
if [ -z "$VERSION_STRING" ]; then
  echo no VERSION_STRING found
  exit 1
fi

echo --------------------------------------------------------------
echo BUILD OBS
echo --------------------------------------------------------------

UNPACKED_DEPS_DIR="$(pwd)/obs-studio/unpacked_deps"
echo "UNPACKED_DEPS_DIR: $UNPACKED_DEPS_DIR"

cd "$ROOT_DIR" && pwd
mkdir -p obs-studio
cd obs-studio/

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

CMAKE=cmake
if [ -e /usr/local/bin/cmake ]; then
  CMAKE=/usr/local/bin/cmake
fi
echo "CMAKE: $CMAKE"

OSX_ARCHITECTURES="x86_64;arm64"
if [ -n "$TARGET_ARCH" ]; then
  echo "using env arch: $TARGET_ARCH"
  OSX_ARCHITECTURES="$TARGET_ARCH"
fi
echo "OSX_ARCHITECTURES: $OSX_ARCHITECTURES"

pwd && ls

mkdir -p build_installed
BUILD_INSTALLED_DIR="$(pwd)/build_installed"

cd src
echo building
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
  -DCMAKE_PREFIX_PATH="$UNPACKED_DEPS_DIR" \
  -DCMAKE_INSTALL_PREFIX:PATH="$BUILD_INSTALLED_DIR" \
  ..

$CMAKE --build . --config Release -t obs-frontend-api
$CMAKE --install . --config Release --component obs_libraries

cd ..
du -chd1

echo --------------------------------------------------------------
echo PLUGIN PLIBSYS
echo --------------------------------------------------------------
cd "$ROOT_DIR" && pwd

if [ -e "deps/plibsys" ]; then
  echo plibsys exists, assuming cached, leaving
else
  pwd
  cd deps
  ./clone_plibsys.sh
  cd ..
  pwd
fi

echo --------------------------------------------------------------
echo PLUGIN CMAKE
echo --------------------------------------------------------------
cd "$ROOT_DIR" && pwd

if [ -n "$GOOGLE_API_KEY" ] && [ "$GOOGLE_API_KEY" != '$(GOOGLE_API_KEY)' ]; then
  echo building with hardcoded compiled API key
  API_OR_UI_KEY_ARG="-DGOOGLE_API_KEY=$GOOGLE_API_KEY"
else
  echo building with custom user API key UI
  API_OR_UI_KEY_ARG="-DENABLE_CUSTOM_API_KEY=ON"
fi

ls -l

OBS_ROOT="$PWD/obs-studio/"
echo OBS_ROOT: $OBS_ROOT

mkdir -p build
cd build
pwd

cmake ../../../ \
  -DCMAKE_OSX_ARCHITECTURES="$OSX_ARCHITECTURES" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DOBS_SOURCE_DIR="$OBS_ROOT" \
  -DOBS_LIB_DIR="$BUILD_INSTALLED_DIR" \
  -DOBS_DEPS_DIR="$UNPACKED_DEPS_DIR" \
  -DSPEECH_API_GOOGLE_HTTP_OLD=ON \
  "$API_OR_UI_KEY_ARG"
cd ../

echo --------------------------------------------------------------
echo PLUGIN BUILD
echo --------------------------------------------------------------

cd build
cmake --build . --config RelWithDebInfo
cd ../

echo --------------------------------------------------------------
echo POST INSTALL, FIX RPATHS
echo --------------------------------------------------------------

cp -v build/libobs_google_caption_plugin.so ./
libfile="libobs_google_caption_plugin.so"

#make rpaths relative, OBS 24+
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

echo --------------------------------------------------------------
echo POST INSTALL, BUILD ZIPS
echo --------------------------------------------------------------

RELEASE_NAME="Closed_Captions_Plugin__v""$VERSION_STRING""_MacOS"
RELEASE_FOLDER="release/$RELEASE_NAME/cloud_captions_plugin/bin"

mkdir -p "$RELEASE_FOLDER"
cp -v libobs_google_caption_plugin.so "$RELEASE_FOLDER"/cloud_captions_plugin.so

cd release
zip -r "$RELEASE_NAME".zip "$RELEASE_NAME"
cd ..

#find ./
df -lh

echo --------------------------------------------------------------
echo DONE
echo --------------------------------------------------------------
