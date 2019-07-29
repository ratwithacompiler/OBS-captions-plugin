#!/bin/bash

set -e
set -v

pwd
find ./
unzip obs_deps_macos/archive_osx.zip -d ./obs_deps/
find ./

mkdir -p /usr/local/Cellar/qt/
mv -vn obs_deps/qt_dep /usr/local/Cellar/qt/5.10.1

OBS_ROOT=$PWD/obs_deps/obs_src
echo OBS_ROOT= $OBS_ROOT

mkdir build
cd build
pwd

set +v
cmake ..  \
-DOBS_SOURCE_DIR=$OBS_ROOT \
-DOBS_LIB_DIR=$OBS_ROOT/build \
-DQT_DIR=/usr/local/Cellar/qt/5.10.1 \
-DGOOGLE_API_KEY="$GOOGLE_API_KEY"
set -v

cd ../
