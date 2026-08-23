#!/bin/bash

# expects CWD = CI_build/build_deps; leaves CWD inside the vcpkg dir
function vcpkg_clone_bootstrap() {
  if [ ! -d "vcpkg" ]; then
    echo "installing vcpkg"
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    git reset --hard 9e593bb18ea69cc5095e012465dcd675a822ed0d #grpc 1.81.1, vcpkg release 2026.07.29
    ./bootstrap-vcpkg.sh
  else
    echo "vcpkg exists already, using that"
    cd vcpkg
  fi
}

# expects CWD = CI_build/build_deps, $VCPKG_DIR and $VCPKG_TRIPLET set; sets $GOOGLE_APIS
function googleapis_codegen() {
  protoc_path="$VCPKG_DIR/installed/$VCPKG_TRIPLET/tools/protobuf/protoc"
  protoc_include="$VCPKG_DIR/installed/$VCPKG_TRIPLET/include/"
  grpc_cpp_path="$VCPKG_DIR/installed/$VCPKG_TRIPLET/tools/grpc/grpc_cpp_plugin"
  echo "protoc_path: $protoc_path"
  echo "protoc_include: $protoc_include"
  echo "grpc_cpp_path: $grpc_cpp_path"

  if [ -d "googleapis" ]; then
    echo googleapis exists already, skipping checkout
    cd googleapis
  else
    echo checking out repo
    git clone --single-branch --branch master "https://github.com/googleapis/googleapis"
    cd googleapis
    git reset --hard 9f7c0ffdaa8ceb2f27982bad713a03306157a4d2
  fi

  if [ -e "gens/google/cloud/speech/v1/cloud_speech.grpc.pb.cc" ]; then
    echo "google apis already generated, skipping"
  else
    make GRPCPLUGIN="$grpc_cpp_path" PROTOC="$protoc_path" PROTOINCLUDE="$protoc_include" LANGUAGE=cpp clean || true
    make GRPCPLUGIN="$grpc_cpp_path" PROTOC="$protoc_path" PROTOINCLUDE="$protoc_include" LANGUAGE=cpp all
  fi
  GOOGLE_APIS="$(pwd)"
  echo GOOGLE_APIS: $GOOGLE_APIS
}

# expects $VCPKG_DIR set
function vcpkg_cleanup() {
  if [ "$CLEAN_VCPKG" = "1" ] || [ "$CLEAN_VCPKG" = "true" ]; then
    cd "$VCPKG_DIR"
    echo "cleaning VCPKG"
    if [ -e packages ]; then
      echo "deleting unneeded vcpkg folder: packages"
      rm -r packages
    fi
    if [ -e downloads ]; then
      echo "deleting unneeded vcpkg folder: downloads"
      rm -r downloads
    fi
    if [ -e buildtrees ]; then
      echo "deleting unneeded vcpkg folder: buildtrees"
      rm -r buildtrees
    fi
  fi
}
