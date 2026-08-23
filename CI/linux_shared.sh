#!/bin/bash

# expects CWD = CI_build, $ROOT_DIR = the variant dir (CI/http or CI/grpc), $VERSION_STRING set
function linux_make_release_zip() {
  set -e

  local libfile="libobs_google_caption_plugin.so"
  cp -v installed/lib/libobs_google_caption_plugin.so "$libfile"

  local RELEASE_NAME="Closed_Captions_Plugin__v""$VERSION_STRING""_Linux"
  local RELEASE_FOLDER="release/$RELEASE_NAME"
  local RELEASE_PLUGIN_FOLDER="$RELEASE_FOLDER"/libobs_google_caption_plugin/bin/64bit/

  mkdir -p "$RELEASE_PLUGIN_FOLDER"
  cp -vn "$libfile" "$RELEASE_PLUGIN_FOLDER"/
  cp -vn "$ROOT_DIR"/../release_files/linux/Readme.md "$RELEASE_FOLDER"

  (cd release && zip -r "$RELEASE_NAME".zip "$RELEASE_NAME")
  du -chd1 && df -lh
  ls -l "$RELEASE_FOLDER"
}
