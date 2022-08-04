#!/bin/bash


set -e

ROOT_DIR="$(pwd)"

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

# (cd ../../ && git submodule update --init --recursive)

# exit
echo --------------------------------------------------------------
echo BUILD OBS
echo --------------------------------------------------------------

UNPACKED_DEPS_DIR="$(pwd)/obs-studio/unpacked_deps"
echo "UNPACKED_DEPS_DIR: $UNPACKED_DEPS_DIR"

cd "$ROOT_DIR" && pwd
if [ -e "obs-studio" ]; then
    echo obs-studio exists, assuming cached, reusing
else
    git clone --single-branch --branch master https://github.com/obsproject/obs-studio.git
    cd obs-studio/

    git checkout 28.0.0-beta1
    git submodule update --init --recursive

    wget -c https://github.com/obsproject/obs-deps/releases/download/2022-08-02/macos-deps-2022-08-02-universal.tar.xz
    wget -c https://github.com/obsproject/obs-deps/releases/download/2022-08-02/macos-deps-qt6-2022-08-02-universal.tar.xz

    mkdir -p unpacked_deps
    tar -k -xjvf  macos-deps-2022-08-02-universal.tar.xz -C unpacked_deps
    tar -k -xjvf  macos-deps-qt6-2022-08-02-universal.tar.xz -C unpacked_deps
    pwd && ls


    mkdir -p build
    cd build
    echo building
    pwd
    /usr/local/bin/cmake \
     -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
     -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
     -DDISABLE_PYTHON=ON \
     -DDISABLE_LUA=ON \
     -DENABLE_SCRIPTING=OFF \
     -DENABLE_SPEEXDSP=OFF \
     -DENABLE_AJA=OFF \
     -DCMAKE_PREFIX_PATH="$UNPACKED_DEPS_DIR" \
     ..

    /usr/local/bin/cmake --build .
    cd ..

    du -chd1
    cd ..
fi
cd "$ROOT_DIR" && pwd
# mkdir release
# exit 0
#
echo --------------------------------------------------------------
echo PLIBSYS
echo --------------------------------------------------------------
cd "$ROOT_DIR" && pwd

if [ -e "deps/plibsys" ]; then
    echo plibsys exists, assuming cached, leaving
else
    pwd
    cd deps
    ./clone_plibsys.sh
    cd ..
    pwd
fi

echo --------------------------------------------------------------
echo CMAKE
echo --------------------------------------------------------------
cd "$ROOT_DIR" && pwd

ls -l

OBS_ROOT="$PWD/obs-studio/"
echo OBS_ROOT: $OBS_ROOT

mkdir -p build
cd build
pwd

cmake ../../../ \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
  -DSPEECH_API_GOOGLE_HTTP_OLD=ON \
  -DOBS_SOURCE_DIR="$OBS_ROOT" \
  -DOBS_LIB_DIR="$OBS_ROOT/build" \
  -DQT_DEP_DIR="$UNPACKED_DEPS_DIR" \
  "$API_OR_UI_KEY_ARG"
cd ../

# copy the cmake processed file with version_string
# (the one time a self modifying bash script is actually useful)

# VERSION_STRING="VERSION_HERE"
cp -v build/CI/http/osx-install-script.sh ./ #TODO: uncomment!

echo --------------------------------------------------------------
echo BUILDING
echo --------------------------------------------------------------

cd build
cmake --build . --parallel
cd ../

echo --------------------------------------------------------------
echo POST INSTALL, FIX RPATHS
echo --------------------------------------------------------------

cp -vn build/libobs_google_caption_plugin.so ./
libfile="libobs_google_caption_plugin.so"

#make rpaths relative, OBS 24+
otool -L "$libfile"

install_name_tool \
-change @rpath/QtWidgets.framework/Versions/A/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/A/QtWidgets \
-change @rpath/QtGui.framework/Versions/A/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/A/QtGui \
-change @rpath/QtCore.framework/Versions/A/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore \
-change @rpath/libobs.framework/Versions/A/libobs @executable_path/../Frameworks/libobs.framework/Versions/A/libobs \
-change @rpath/libobs-frontend-api.1.dylib @executable_path/../Frameworks/libobs-frontend-api.dylib \
"$libfile"


otool -L "$libfile"

# ensure it worked
otool -L "$libfile" | grep -q '@executable_path/../Frameworks/QtGui.framework/Versions/A/QtGui'
otool -L "$libfile" | grep -q '@executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore'
otool -L "$libfile" | grep -q '@executable_path/../Frameworks/QtWidgets.framework/Versions/A/QtWidgets'
otool -L "$libfile" | grep -q '@executable_path/../Frameworks/libobs.framework/Versions/A/libobs'
otool -L "$libfile" | grep -q '@executable_path/../Frameworks/libobs-frontend-api.dylib'

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
