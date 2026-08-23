#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR/.." # everything runs from the variant dir (CI/http)
set -e
ROOT_DIR="$(pwd)"
source ../unix_shared.sh
source ../linux_build_obs.sh

mkdir -p CI_build && cd CI_build
CI_ROOT_DIR="$(pwd)"

BUILD_DEPS_DIR="$CI_ROOT_DIR/build_deps/"
CMAKE=cmake

# re-derive the OBS paths; skips the actual build via the done sentinel
mkdir -p "$BUILD_DEPS_DIR" && cd "$BUILD_DEPS_DIR" && pwd
build_obs

echo --------------------------------------------------------------
echo PLUGIN CMAKE
echo --------------------------------------------------------------
cd "$CI_ROOT_DIR" && pwd

api_key_arg

INSTALLED_DIR="$(pwd)/installed"
mkdir -p build && cd build && pwd

cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DOBS_BUILD_DIR="$BUILD_OBS__INSTALLED_DIR" \
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

if [ "$1" = "--package" ]; then
  "$SCRIPT_DIR/post_build.sh"
fi
