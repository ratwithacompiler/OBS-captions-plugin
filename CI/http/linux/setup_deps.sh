#!/bin/bash

cd "$(dirname "$0")/.." # everything runs from the variant dir (CI/http)
set -e
ROOT_DIR="$(pwd)"
source ../unix_shared.sh
source ../linux_build_obs.sh

mkdir -p CI_build && cd CI_build
CI_ROOT_DIR="$(pwd)"

BUILD_DEPS_DIR="$CI_ROOT_DIR/build_deps/"
echo "BUILD_DEPS_DIR: $BUILD_DEPS_DIR"

CMAKE=cmake
echo "CMAKE: $CMAKE"

echo --------------------------------------------------------------
echo BUILD OBS
echo --------------------------------------------------------------
mkdir -p "$BUILD_DEPS_DIR" && cd "$BUILD_DEPS_DIR" && pwd
build_obs
echo "BUILD_OBS__SRC_DIR: $BUILD_OBS__SRC_DIR"
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
echo CLEANUP
echo --------------------------------------------------------------

# OBS installs into build_installed/ (separate from src/), so src/ incl. the
# build tree is safe to remove; build_installed/ + the done sentinel survive
build_obs_cleanup

echo --------------------------------------------------------------
echo DONE
echo --------------------------------------------------------------
