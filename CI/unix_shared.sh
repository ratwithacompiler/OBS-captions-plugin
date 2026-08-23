#!/bin/bash

function version_string() {
  VERSION_STRING="$(cat ../../CMakeLists.txt | egrep VERSION_STRING | egrep set | sed 's/ //g' | egrep -o '(VERSION_STRING"(.+)")' | egrep -o '".+"' | sed 's/"//g')"
  if [ -z "$VERSION_STRING" ]; then
    echo no VERSION_STRING found
    exit 1
  fi
  echo "VERSION_STRING: $VERSION_STRING"
}

function api_key_arg() {
  if [ -n "$GOOGLE_API_KEY" ]; then
    echo building with hardcoded compiled API key
    API_OR_UI_KEY_ARG="-DGOOGLE_API_KEY=$GOOGLE_API_KEY"
  else
    echo building with custom user API key UI
    API_OR_UI_KEY_ARG="-DENABLE_CUSTOM_API_KEY=ON"
  fi
}
