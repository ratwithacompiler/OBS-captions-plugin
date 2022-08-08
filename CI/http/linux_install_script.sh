#!/bin/bash

set -e
ROOT_DIR="$(pwd)"
source ../unix_shared.sh
source ../linux_build_obs.sh
version_string

mkdir -p CI_build && cd CI_build
CI_ROOT_DIR="$(pwd)"

BUILD_DEPS_DIR="$CI_ROOT_DIR/build_deps/"
echo "BUILD_DEPS_DIR: $BUILD_DEPS_DIR"

CMAKE=cmake
echo "CMAKE: $CMAKE"
echo "CLEAN_OBS: $CLEAN_OBS"

echo --------------------------------------------------------------
echo BUILD OBS
echo --------------------------------------------------------------
mkdir -p "$BUILD_DEPS_DIR" && cd "$BUILD_DEPS_DIR" && pwd
build_obs
echo "BUILD_OBS__SRC_DIR: $BUILD_OBS__SRC_DIR"
echo "BUILD_OBS__UNPACKED_DEPS_DIR: $BUILD_OBS__UNPACKED_DEPS_DIR"
echo "BUILD_OBS__INSTALLED_DIR: $BUILD_OBS__INSTALLED_DIR"
echo "BUILD_OBS__BUILD_DIR: $BUILD_OBS__BUILD_DIR"

echo --------------------------------------------------------------
echo PLUGIN PLIBSYS
echo --------------------------------------------------------------
cd "$CI_ROOT_DIR" && pwd

if [ -e "plibsys" ]; then
  echo plibsys exists, assuming cached, leaving
else
  "$ROOT_DIR"/clone_plibsys.sh
fi

echo --------------------------------------------------------------
echo PLUGIN CMAKE
echo --------------------------------------------------------------
cd "$CI_ROOT_DIR" && pwd

if [ -n "$GOOGLE_API_KEY" ] && [ "$GOOGLE_API_KEY" != '$(GOOGLE_API_KEY)' ]; then
  echo building with hardcoded compiled API key
  API_OR_UI_KEY_ARG="-DGOOGLE_API_KEY=$GOOGLE_API_KEY"
else
  echo building with custom user API key UI
  API_OR_UI_KEY_ARG="-DENABLE_CUSTOM_API_KEY=ON"
fi

INSTALLED_DIR="$(pwd)/installed"
mkdir -p build && cd build && pwd

cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DOBS_SOURCE_DIR="$BUILD_OBS__SRC_DIR" \
  -DOBS_LIB_DIR="$BUILD_OBS__INSTALLED_DIR" \
  -DOBS_DEPS_DIR="$BUILD_OBS__UNPACKED_DEPS_DIR" \
  -DSPEECH_API_GOOGLE_HTTP_OLD=ON \
  "$API_OR_UI_KEY_ARG" \
  -DCMAKE_INSTALL_PREFIX:PATH="$INSTALLED_DIR" \
  "$ROOT_DIR/../.."

echo --------------------------------------------------------------
echo PLUGIN BUILD
echo --------------------------------------------------------------

cd "$CI_ROOT_DIR" && cd build && pwd
$CMAKE --build . --config RelWithDebInfo
$CMAKE --install . --config RelWithDebInfo --verbose

echo --------------------------------------------------------------
echo POST INSTALL, BUILD ZIPS
echo --------------------------------------------------------------
cd "$CI_ROOT_DIR" && pwd

libfile="libobs_google_caption_plugin.so"
cp -v installed/lib/libobs_google_caption_plugin.so "$libfile"

RELEASE_NAME="Closed_Captions_Plugin__v""$VERSION_STRING""_Linux"
RELEASE_FOLDER="release/$RELEASE_NAME"
RELEASE_PLUGIN_FOLDER="$RELEASE_FOLDER"/libobs_google_caption_plugin/bin/64bit/

mkdir -p "$RELEASE_PLUGIN_FOLDER"
cp -vn "$libfile" "$RELEASE_PLUGIN_FOLDER"/
cp -vn "$ROOT_DIR"/../release_files/linux/Readme.md "$RELEASE_FOLDER"

(cd release && zip -r "$RELEASE_NAME".zip "$RELEASE_NAME")
du -chd1 && df -lh
ls -l "$RELEASE_FOLDER"

echo --------------------------------------------------------------
echo CLEANUP
echo --------------------------------------------------------------

build_obs_cleanup

echo --------------------------------------------------------------
echo DONE
echo --------------------------------------------------------------
