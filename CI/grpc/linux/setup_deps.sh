#!/bin/bash

cd "$(dirname "$0")/.." # everything runs from the variant dir (CI/grpc)
set -e
ROOT_DIR="$(pwd)"
source ../unix_shared.sh
source ../linux_build_obs.sh
source ./grpc_shared.sh

CLEAN_VCPKG="${CLEAN_VCPKG:-1}" # clean by default to keep the dir cacheable/small
echo "CLEAN_VCPKG: $CLEAN_VCPKG"

mkdir -p CI_build && cd CI_build
CI_ROOT_DIR="$(pwd)"

BUILD_DEPS_DIR="$CI_ROOT_DIR/build_deps/"
echo "BUILD_DEPS_DIR: $BUILD_DEPS_DIR"

CMAKE=cmake

echo --------------------------------------------------------------
echo BUILD OBS
echo --------------------------------------------------------------
mkdir -p "$BUILD_DEPS_DIR" && cd "$BUILD_DEPS_DIR" && pwd
build_obs
echo "BUILD_OBS__SRC_DIR: $BUILD_OBS__SRC_DIR"
echo "BUILD_OBS__INSTALLED_DIR: $BUILD_OBS__INSTALLED_DIR"
echo "BUILD_OBS__BUILD_DIR: $BUILD_OBS__BUILD_DIR"

echo --------------------------------------------------------------
echo VCPKG SETUP
echo --------------------------------------------------------------
cd "$BUILD_DEPS_DIR" && pwd
vcpkg_clone_bootstrap

./vcpkg install --host-triplet=x64-linux-release --triplet=x64-linux-release grpc:x64-linux-release
VCPKG_DIR=$(pwd)
VCPKG_TRIPLET="x64-linux-release"

echo --------------------------------------------------------------
echo GOOGLEAPIS
echo --------------------------------------------------------------
cd "$BUILD_DEPS_DIR" && pwd
googleapis_codegen

echo --------------------------------------------------------------
echo CLEANUP
echo --------------------------------------------------------------

# OBS installs into build_installed/ (separate from src/), so src/ incl. the
# build tree is safe to remove; build_installed/ + the done sentinel survive
build_obs_cleanup

vcpkg_cleanup

echo --------------------------------------------------------------
echo DONE
echo --------------------------------------------------------------
