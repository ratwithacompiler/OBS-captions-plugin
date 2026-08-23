#!/bin/bash

# the only system-wide-affecting CI step: apt packages needed to build OBS and the plugin.
# separate from the other stages so local devs can review/skip it.

set -e

sudo apt-get update && sudo apt-get install -y \
  build-essential \
  extra-cmake-modules \
  libcurl4-openssl-dev \
  libavcodec-dev libavdevice-dev libavfilter-dev libavformat-dev libavutil-dev \
  libswresample-dev libswscale-dev \
  libjansson-dev \
  uthash-dev libsimde-dev uuid-dev zlib1g-dev \
  libx11-dev libxcb1-dev libx11-xcb-dev libxkbcommon-dev \
  libgles2-mesa-dev libgl1-mesa-dev libegl1-mesa-dev libdrm-dev \
  libwayland-dev \
  libpulse-dev \
  qt6-base-dev libqt6svg6-dev qt6-base-private-dev
