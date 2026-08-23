#!/bin/bash

cd "$(dirname "$0")/.." # everything runs from the variant dir (CI/grpc)
set -e
ROOT_DIR="$(pwd)"
source ../unix_shared.sh
source ../linux_shared.sh
version_string

cd CI_build

echo --------------------------------------------------------------
echo POST INSTALL, BUILD ZIPS
echo --------------------------------------------------------------

linux_make_release_zip
