#!/bin/bash

set -e

echo --------------------------------------------------------------
echo SETUP
echo --------------------------------------------------------------

if [ -n "$GOOGLE_API_KEY" ] && [ "$GOOGLE_API_KEY" != '$(GOOGLE_API_KEY)' ]; then
  echo building with hardcoded compiled API key
  API_OR_UI_KEY_ARG="-DGOOGLE_API_KEY=$GOOGLE_API_KEY"
else
  echo building with custom user API key UI
  API_OR_UI_KEY_ARG="-DENABLE_CUSTOM_API_KEY=ON"
fi

pwd
(cd ../../ && git submodule update --init --recursive)

echo --------------------------------------------------------------
echo PLIBSYS
echo --------------------------------------------------------------

pwd
cd deps
./clone_plibsys.sh
cd ..
pwd

echo --------------------------------------------------------------
echo DEPS
echo --------------------------------------------------------------

sudo apt-get -qq update
sudo apt-get install -y qtbase5-dev

echo --------------------------------------------------------------
echo CMAKE
echo --------------------------------------------------------------

ls -l
ls -l obs_deps

OBS_ROOT="$PWD/obs_deps/obs_src"
echo OBS_ROOT: $OBS_ROOT

mkdir build
cd build
pwd

cmake ../../../ \
  -DSPEECH_API_GOOGLE_HTTP_OLD=ON \
  -DOBS_SOURCE_DIR="$OBS_ROOT" \
  -DOBS_LIB_DIR="$OBS_ROOT/build" \
  "$API_OR_UI_KEY_ARG"

cd ../

# copy the cmake processed file with version_string
# (the one time a self modifying bash script is actually useful)
cp -v build/CI/http/linux-install-script.sh ./

echo --------------------------------------------------------------
echo BUILDING
echo --------------------------------------------------------------

cd build
make -j4
cd ../

echo --------------------------------------------------------------
echo POST INSTALL, BUILD ZIPS
echo --------------------------------------------------------------

if [ -z "${VERSION_STRING}" ]; then
  echo "Error, no version string, should have been inserted by cmake"
  exit 1
fi

RELEASE_NAME="Closed_Captions_Plugin__v${VERSION_STRING}_Linux"
RELEASE_FOLDER="release/$RELEASE_NAME"
RELEASE_PLUGIN_FOLDER="release/$RELEASE_NAME"/libobs_google_caption_plugin/bin/64bit/

mkdir -p "$RELEASE_FOLDER"
mkdir -p "$RELEASE_PLUGIN_FOLDER"

cp -vn build/libobs_google_caption_plugin.so "$RELEASE_PLUGIN_FOLDER"/
cp -vn ../release_files/linux/Readme.md "$RELEASE_FOLDER"

cd release
zip -r "$RELEASE_NAME".zip "$RELEASE_NAME"
cd ..

find ./
df -lh

echo --------------------------------------------------------------
echo DONE
echo --------------------------------------------------------------
