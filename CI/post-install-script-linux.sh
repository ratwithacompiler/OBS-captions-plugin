#!/bin/bash

set -e
set -v

RELEASE_NAME="Closed_Captions_Plugin__v${VERSION_STRING}_Linux"
RELEASE_FOLDER="release/$RELEASE_NAME"
RELEASE_PLUGIN_FOLDER="release/$RELEASE_NAME"/libobs_google_caption_plugin/bin/64bit/

mkdir -p "$RELEASE_FOLDER"
mkdir -p "$RELEASE_PLUGIN_FOLDER"

cp -vn build/libobs_google_caption_plugin.so "$RELEASE_PLUGIN_FOLDER"/
cp -vn release_files/linux/Readme.md "$RELEASE_FOLDER"

cd release
zip -r "$RELEASE_NAME".zip "$RELEASE_NAME"
cd ..

find ./
df -lh
