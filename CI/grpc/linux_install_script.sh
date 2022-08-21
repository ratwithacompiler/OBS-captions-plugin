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
echo "CLEAN_VCPKG: $CLEAN_VCPKG"
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
echo VCPKG SETUP
echo --------------------------------------------------------------
cd "$BUILD_DEPS_DIR" && pwd
echo creating vcpkg

if [ ! -d "vcpkg" ]; then
  echo "installing vcpkg"
  git clone https://github.com/Microsoft/vcpkg.git
  cd vcpkg
  git reset --hard 824c4324736156720645819abe4d48b4e740a1cd #grpc 1.48.0
  ./bootstrap-vcpkg.sh
else
  echo "vcpkg exists already, using that"
  cd vcpkg
fi

./vcpkg install --host-triplet=x64-linux-release --triplet=x64-linux-release grpc:x64-linux-release
VCPKG_DIR=$(pwd)
VCPKG_TRIPLET="x64-linux-release"

echo --------------------------------------------------------------
echo GOOGLEAPIS
echo --------------------------------------------------------------
cd "$BUILD_DEPS_DIR" && pwd

protoc_path="$VCPKG_DIR/installed/$VCPKG_TRIPLET/tools/protobuf/protoc"
protoc_include="$VCPKG_DIR/installed/$VCPKG_TRIPLET/include/"
grpc_cpp_path="$VCPKG_DIR/installed/$VCPKG_TRIPLET/tools/grpc/grpc_cpp_plugin"
echo "protoc_path: $protoc_path"
echo "protoc_include: $protoc_include"
echo "grpc_cpp_path: $grpc_cpp_path"

if [ -d "googleapis" ]; then
  echo googleapis exists already, skipping checkout
  cd googleapis
else
  echo checking out repo
  git clone --single-branch --branch master "https://github.com/googleapis/googleapis"
  cd googleapis
  git reset --hard 9f7c0ffdaa8ceb2f27982bad713a03306157a4d2
fi

if [ -e "gens/google/cloud/speech/v1/cloud_speech.grpc.pb.cc" ]; then
  echo "google apis already generated, skipping"
else
  make GRPCPLUGIN="$grpc_cpp_path" PROTOC="$protoc_path" PROTOINCLUDE="$protoc_include" LANGUAGE=cpp clean || true
  make GRPCPLUGIN="$grpc_cpp_path" PROTOC="$protoc_path" PROTOINCLUDE="$protoc_include" LANGUAGE=cpp all
fi
GOOGLE_APIS="$(pwd)"
echo GOOGLE_APIS: $GOOGLE_APIS

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

VCPKG_CMAKE_PREFIX_PATH="$VCPKG_DIR/installed/$VCPKG_TRIPLET"
echo "VCPKG_CMAKE_PREFIX_PATH: $VCPKG_CMAKE_PREFIX_PATH"

INSTALLED_DIR="$(pwd)/installed"
mkdir -p build && cd build && pwd

$CMAKE \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DOBS_BUILD_DIR="$BUILD_OBS__INSTALLED_DIR" \
  -DOBS_DEPS_DIR="$BUILD_OBS__UNPACKED_DEPS_DIR" \
  -DSPEECH_API_GOOGLE_GRPC_V1=ON \
  -DGOOGLEAPIS_DIR="$GOOGLE_APIS" \
  -DCMAKE_PREFIX_PATH="$VCPKG_CMAKE_PREFIX_PATH" \
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

if [ "$CLEAN_VCPKG" = "1" ] || [ "$CLEAN_VCPKG" = "true" ]; then
  cd "$VCPKG_DIR"
  echo "cleaning VCPKG"
  if [ -e packages ]; then
    echo "deleting unneeded vcpkg folder: packages"
    rm -r packages
  fi
  if [ -e downloads ]; then
    echo "deleting unneeded vcpkg folder: downloads"
    rm -r downloads
  fi
  if [ -e buildtrees ]; then
    echo "deleting unneeded vcpkg folder: buildtrees"
    rm -r buildtrees
  fi
fi

echo --------------------------------------------------------------
echo DONE
echo --------------------------------------------------------------
