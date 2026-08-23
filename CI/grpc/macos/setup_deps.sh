#!/bin/bash

cd "$(dirname "$0")/.." # everything runs from the variant dir (CI/grpc)
set -e
ROOT_DIR="$(pwd)"
source ../unix_shared.sh
source ../macos_shared.sh
source ../macos_build_obs.sh
source ./grpc_shared.sh

macos_stage_env
CLEAN_VCPKG="${CLEAN_VCPKG:-1}" # clean by default to keep the dir cacheable/small
echo "CLEAN_VCPKG: $CLEAN_VCPKG"
echo "CLEAN_OBS: $CLEAN_OBS"

mkdir -p CI_build && cd CI_build
CI_ROOT_DIR="$(pwd)"

BUILD_DEPS_DIR="$CI_ROOT_DIR/build_deps/"
echo "BUILD_DEPS_DIR: $BUILD_DEPS_DIR"

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
echo VCPKG SETUP
echo --------------------------------------------------------------
cd "$BUILD_DEPS_DIR" && pwd
vcpkg_clone_bootstrap

if [ ! -e triplets/community/universal-osx-release.cmake ]; then
  cp -vn triplets/community/x64-osx-release.cmake triplets/community/universal-osx-release.cmake
  sed -i "" 's/x86_64/"x86_64;arm64"/' triplets/community/universal-osx-release.cmake
fi

if [ ! -e triplets/community/arm64-osx-release.cmake ]; then
  cp -vn triplets/arm64-osx.cmake triplets/community/arm64-osx-release.cmake
  echo -ne '\nset(VCPKG_BUILD_TYPE release)\n' >>triplets/community/arm64-osx-release.cmake
fi

./vcpkg install --host-triplet=universal-osx-release --triplet=universal-osx-release grpc:universal-osx-release
./vcpkg install --host-triplet=x64-osx-release --triplet=x64-osx-release openssl:x64-osx-release
./vcpkg install --host-triplet=arm64-osx-release --triplet=arm64-osx-release openssl:arm64-osx-release
VCPKG_DIR=$(pwd)

mv -vn installed/universal-osx-release/lib/libcrypto.a installed/universal-osx-release/lib/libcrypto.a.bak || true
mv -vn installed/universal-osx-release/lib/libssl.a installed/universal-osx-release/lib/libssl.a.bak || true
lipo -create installed/arm64-osx-release/lib/libcrypto.a installed/x64-osx-release/lib/libcrypto.a -output installed/universal-osx-release/lib/libcrypto.a
lipo -create installed/arm64-osx-release/lib/libssl.a installed/x64-osx-release/lib/libssl.a -output installed/universal-osx-release/lib/libssl.a
VCPKG_TRIPLET="universal-osx-release"

echo --------------------------------------------------------------
echo GOOGLEAPIS
echo --------------------------------------------------------------
cd "$BUILD_DEPS_DIR" && pwd
googleapis_codegen

echo --------------------------------------------------------------
echo CLEANUP
echo --------------------------------------------------------------

# OBS installs into build_installed/ (separate from src/), so src/ incl. the
# build tree is safe to remove; build_installed/, unpacked_deps/ (Qt) and the
# done sentinel survive
build_obs_cleanup

vcpkg_cleanup

echo --------------------------------------------------------------
echo DONE
echo --------------------------------------------------------------
