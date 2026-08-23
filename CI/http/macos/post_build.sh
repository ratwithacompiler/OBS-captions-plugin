#!/bin/bash

cd "$(dirname "$0")/.." # everything runs from the variant dir (CI/http)
set -e
ROOT_DIR="$(pwd)"
source ../unix_shared.sh
source ../macos_shared.sh
version_string

cd CI_build

echo --------------------------------------------------------------
echo POST INSTALL, FIX RPATHS, BUILD ZIP
echo --------------------------------------------------------------

macos_make_release_zip
