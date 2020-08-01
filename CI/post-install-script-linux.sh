#!/bin/bash

set -e
set -v

RELEASE_NAME="Closed_Captions_Plugin__v${VERSION_STRING}_Linux"
RELEASE_FOLDER="release/$RELEASE_NAME"

mkdir -p "$RELEASE_FOLDER"

cp -vn build/libobs_google_caption_plugin.so "$RELEASE_FOLDER"/

cd release
zip -r "$RELEASE_NAME".zip "$RELEASE_NAME"
cd ..

find ./
df -lh
