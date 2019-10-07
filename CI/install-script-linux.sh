#!/bin/bash

set -e
set -v

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

set +v
cmake ../../  \
-DSPEECH_API_GOOGLE_HTTP_OLD=ON \
-DOBS_SOURCE_DIR=$OBS_ROOT \
-DOBS_LIB_DIR=$OBS_ROOT/build \
-DGOOGLE_API_KEY="$GOOGLE_API_KEY"

set -v

cd ../
