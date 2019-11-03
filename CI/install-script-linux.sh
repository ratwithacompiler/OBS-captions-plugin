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
ls -l obs_deps

OBS_ROOT=$PWD/obs_deps/obs_src
echo OBS_ROOT= $OBS_ROOT

sudo apt-get -qq update
sudo apt-get install -y \
        qtbase5-dev


mkdir build
cd build
pwd

cmake ../../  \
-DSPEECH_API_GOOGLE_HTTP_OLD=ON \
-DOBS_SOURCE_DIR=$OBS_ROOT \
-DOBS_LIB_DIR=$OBS_ROOT/build \
$API_OR_UI_KEY_ARG

cd ../
