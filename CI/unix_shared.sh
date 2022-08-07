#!/bin/bash

function version_string() {
  VERSION_STRING="$(cat ../../CMakeLists.txt | egrep VERSION_STRING | egrep set | sed 's/ //g' | egrep -o '(VERSION_STRING"(.+)")' | egrep -o '".+"' | sed 's/"//g')"
  if [ -z "$VERSION_STRING" ]; then
    echo no VERSION_STRING found
    exit 1
  fi
  echo "VERSION_STRING: $VERSION_STRING"
}
