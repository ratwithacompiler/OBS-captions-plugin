#!/bin/bash

# SDK probe + arch + cmake selection shared by the macos setup_deps/build stages.
# sets SDKROOT, OSX_ARCHITECTURES and CMAKE
function macos_stage_env() {
  ls -ltrgh /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/ || true
  if [ -z "$SDKROOT" ]; then
    # CMake >= 4 no longer sets CMAKE_OSX_SYSROOT by default, but OBS's cmake
    # parses the SDK version out of that path, so pick a versioned SDK explicitly.
    SDK_CANDIDATE="$(ls -d "$(xcode-select -p)"/Platforms/MacOSX.platform/Developer/SDKs/MacOSX[0-9]*.[0-9]*.sdk 2>/dev/null | sort -V | tail -n 1)"
    if [ -n "$SDK_CANDIDATE" ]; then
      export SDKROOT="$SDK_CANDIDATE"
    fi
  fi
  echo "SDKROOT: $SDKROOT"

  OSX_ARCHITECTURES="x86_64;arm64"
  if [ -n "$TARGET_ARCH" ]; then
    echo "using env arch: $TARGET_ARCH"
    OSX_ARCHITECTURES="$TARGET_ARCH"
  fi
  echo "OSX_ARCHITECTURES: $OSX_ARCHITECTURES"

  CMAKE=cmake
  if [ -e /usr/local/bin/cmake ]; then
    CMAKE=/usr/local/bin/cmake
  fi
  echo "CMAKE: $CMAKE"
}

# expects CWD = CI_build, $VERSION_STRING set
function macos_make_release_zip() {
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
