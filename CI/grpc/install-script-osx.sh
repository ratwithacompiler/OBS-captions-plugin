#!/bin/bash

set -e

if [ -n "$GOOGLE_API_KEY" ] && [ "$GOOGLE_API_KEY" != '$(GOOGLE_API_KEY)' ]; then
    echo building with hardcoded compiled API key
    API_OR_UI_KEY_ARG="-DGOOGLE_API_KEY=$GOOGLE_API_KEY"
else
    echo building with custom user API key UI
    API_OR_UI_KEY_ARG="-DENABLE_CUSTOM_API_KEY=ON"
fi

pwd
ls -l

pwd
ls -l obs_deps

mkdir -p /usr/local/Cellar/qt/
chmod +x obs_deps/qt_dep/bin/*
mv -vn obs_deps/qt_dep /usr/local/Cellar/qt/5.10.1

DEPS_ROOT=$PWD/obs_deps/
echo DEPS_ROOT= $DEPS_ROOT

GRPC_DEPS_ROOT=$PWD/grpc_deps/
echo GRPC_DEPS_ROOT= $GRPC_DEPS_ROOT

mkdir build
cd build
pwd

cmake ../../  \
-DSPEECH_API_GOOGLE_GRPC_V1=ON \
-DOBS_SOURCE_DIR=$DEPS_ROOT/obs_src \
-DOBS_LIB_DIR=$DEPS_ROOT/obs_src/build \
-DQT_DIR=/usr/local/Cellar/qt/5.10.1 \
-DGRPC_ROOT_DIR="$GRPC_DEPS_ROOT"/x64-osx \
-DPROTOBUF_ROOT_DIR="$GRPC_DEPS_ROOT"/x64-osx \
-DGOOGLEAPIS_DIR="$GRPC_DEPS_ROOT"/googleapis \
$API_OR_UI_KEY_ARG

cd ../
