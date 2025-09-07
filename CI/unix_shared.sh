#!/bin/bash

function version_string() {
  VERSION_STRING="$(cat ../../CMakeLists.txt | egrep VERSION_STRING | egrep set | sed 's/ //g' | egrep -o '(VERSION_STRING"(.+)")' | egrep -o '".+"' | sed 's/"//g')"
  if [ -z "$VERSION_STRING" ]; then
    echo no VERSION_STRING found
    exit 1
  fi
  echo "VERSION_STRING: $VERSION_STRING"
}

function osx_make_release_zip() {
  set -e

  local libfile="libobs_google_caption_plugin.so"
  cp -v installed/lib/libobs_google_caption_plugin.so "$libfile"

  #make rpaths relative, OBS 28+
  otool -L "$libfile"
  otool -l "$libfile" | egrep /

  echo change rpaths
  install_name_tool -change obs-frontend-api.dylib @executable_path/../Frameworks/obs-frontend-api.dylib \
    -change libobs.framework/Versions/A/libobs @executable_path/../Frameworks/libobs.framework/Versions/A/libobs \
    "$libfile"

  otool -L "$libfile"
  otool -l "$libfile"

  plugin_dir="cloud-closed-captions.plugin"
  if [ -d "$plugin_dir" ]; then
    echo "deleting $plugin_dir"
    rm -rf "$plugin_dir"
  fi

  mkdir -p "$plugin_dir"
  cd "$plugin_dir"
  mkdir -p Contents/MacOS Contents/Resources
  cp -v "../$libfile" Contents/MacOS/cloud-closed-captions

  cat >Contents/Info.plist <<EOFMARK
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleName</key>
	<string>cloud-closed-captions</string>
	<key>CFBundleIdentifier</key>
	<string>com.ratcaptions.cloud-closed-captions</string>
	<key>CFBundleVersion</key>
	<string></string>
	<key>CFBundleShortVersionString</key>
	<string></string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleExecutable</key>
	<string>cloud-closed-captions</string>
	<key>CFBundlePackageType</key>
	<string>BNDL</string>
	<key>CFBundleSupportedPlatforms</key>
	<array>
		<string>MacOSX</string>
	</array>
	<key>LSMinimumSystemVersion</key>
	<string>10.13</string>
</dict>
</plist>
EOFMARK
  cd ..

  local RELEASE_NAME="Closed_Captions_Plugin__v""$VERSION_STRING""_MacOS"
  local RELEASE_FOLDER="release/$RELEASE_NAME/"

  mkdir -p "$RELEASE_FOLDER"
  mv -vn "$plugin_dir" "$RELEASE_FOLDER/"

  cd release
  zip -r "$RELEASE_NAME".zip "$RELEASE_NAME"
  find .
  cd ..

  du -chd1 && df -lh
  ls -l "$RELEASE_FOLDER"
}
