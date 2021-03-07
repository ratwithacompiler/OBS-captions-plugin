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

mkdir -p /tmp/
chmod +x obs_deps/osx_deps/bin/*
mv -vn obs_deps/osx_deps /tmp/obsdeps

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
  -DQT_DEP_DIR=/tmp/obsdeps \
  "$API_OR_UI_KEY_ARG"
cd ../

# copy the cmake processed file with version_string
# (the one time a self modifying bash script is actually useful)
cp -v build/CI/http/osx-install-script.sh ./

echo --------------------------------------------------------------
echo BUILDING
echo --------------------------------------------------------------

cd build
make -j4
cd ../

echo --------------------------------------------------------------
echo POST INSTALL, FIX RPATHS
echo --------------------------------------------------------------

cp -vn build/libobs_google_caption_plugin.so ./
libfile="libobs_google_caption_plugin.so"

#make rpaths relative, OBS 24+
otool -L "$libfile"
install_name_tool -change /tmp/obsdeps/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets "$libfile"
install_name_tool -change /tmp/obsdeps/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui "$libfile"
install_name_tool -change /tmp/obsdeps/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore "$libfile"
otool -L "$libfile"

# ensure it worked
otool -L "$libfile" | grep -q '@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui'
otool -L "$libfile" | grep -q '@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore'
otool -L "$libfile" | grep -q '@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets'

echo --------------------------------------------------------------
echo POST INSTALL, BUILD ZIPS
echo --------------------------------------------------------------

if [ -z "${VERSION_STRING}" ]; then
  echo "Error, no version string, should have been inserted by cmake"
  exit 1
fi

RELEASE_NAME="Closed_Captions_Plugin__v${VERSION_STRING}_MacOS"
RELEASE_FOLDER="release/$RELEASE_NAME/cloud_captions_plugin/bin"

mkdir -p "$RELEASE_FOLDER"

cp -vn libobs_google_caption_plugin.so "$RELEASE_FOLDER"/cloud_captions_plugin.so

cd release
zip -r "$RELEASE_NAME".zip "$RELEASE_NAME"
cd ..

find ./
df -lh

echo --------------------------------------------------------------
echo DONE
echo --------------------------------------------------------------
