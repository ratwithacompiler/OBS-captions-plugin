#!/bin/bash

set -e

if [ -n "$GOOGLE_API_KEY" ] && [ "$GOOGLE_API_KEY" != '$(GOOGLE_API_KEY)' ]; then
    echo building with hardcoded compiled API key
    API_OR_UI_KEY_ARG="-DGOOGLE_API_KEY=$GOOGLE_API_KEY"
else
    echo building with custom user API key UI
    API_OR_UI_KEY_ARG="-DENABLE_CUSTOM_API_KEY=ON"
fi

cd deps
./clone_plibsys.sh
cd ..

pwd
ls -l

pwd
ls -l obs_deps

mkdir -p /usr/local/Cellar/qt/
chmod +x obs_deps/qt_dep/bin/*
mv -vn obs_deps/qt_dep /usr/local/Cellar/qt/5.10.1

OBS_ROOT=$PWD/obs_deps/obs_src
echo OBS_ROOT= $OBS_ROOT

mkdir build
cd build
pwd

cmake ../../  \
-DSPEECH_API_GOOGLE_HTTP_OLD=ON \
-DOBS_SOURCE_DIR=$OBS_ROOT \
-DOBS_LIB_DIR=$OBS_ROOT/build \
-DQT_DIR=/usr/local/Cellar/qt/5.10.1 \
$API_OR_UI_KEY_ARG

cd ../

# copy the cmake processed file with version_string
cp -v build/CI/post-install-script-osx.sh ./
