#!/bin/bash

set -e
ROOT_DIR="$(pwd)"
source ../unix_shared.sh
source ../osx_build_obs.sh
version_string

mkdir -p CI_build && cd CI_build
CI_ROOT_DIR="$(pwd)"

BUILD_DEPS_DIR="$CI_ROOT_DIR/build_deps/"
echo "BUILD_DEPS_DIR: $BUILD_DEPS_DIR"

OSX_ARCHITECTURES="x86_64;arm64"
if [ -n "$TARGET_ARCH" ]; then
  echo "using env arch: $TARGET_ARCH"
  OSX_ARCHITECTURES="$TARGET_ARCH"
fi
echo "OSX_ARCHITECTURES: $OSX_ARCHITECTURES"

CMAKE=cmake
if [ -e /usr/local/bin/cmake ]; then
  CMAKE=/usr/local/bin/cmake
fi
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

$CMAKE \
  -DCMAKE_OSX_ARCHITECTURES="$OSX_ARCHITECTURES" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DOBS_BUILD_DIR="$BUILD_OBS__INSTALLED_DIR" \
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
echo POST INSTALL, FIX RPATHS, BUILD ZIP
echo --------------------------------------------------------------

cd "$CI_ROOT_DIR" && pwd
osx_make_release_zip

echo --------------------------------------------------------------
echo CLEANUP
echo --------------------------------------------------------------

build_obs_cleanup

echo --------------------------------------------------------------
echo DONE
echo --------------------------------------------------------------
