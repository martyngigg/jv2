#!/usr/bin/env bash
# Install the required libraries and packages
# to be able to build the code and run the tests

function exit_if_command_not_available {
  if [ -z $(which $1) ]; then
    echo "Script requires the $1 command to be available"
    exit 1
  fi
}

function run_elevated {
  if [ `id -u` -ne 0 ]; then
    sudo $@
  else
    $@
  fi
}

# Pre-flight checks
echo "Checking for required tools..."
exit_if_command_not_available apt

# Install base packages
echo "Installing base packages."
test `id -u` -ne 0 && echo "sudo access required."
run_elevated apt update
run_elevated apt -y install --no-install-recommends \
  cmake \
  curl \
  gpg \
  g++ \
  libgl-dev \
  make \
  ninja-build \
  pkg-config \
  python3-setuptools \
  python3-pip \
  python3-venv \
  python3-wheel \
  xz-utils

echo "Installing X dependencies for Qt"
run_elevated apt -y install --no-install-recommends \
  libx11-xcb-dev \
  libfontenc-dev \
  libice-dev \
  libsm-dev \
  libxaw7-dev \
  libxcomposite-dev \
  libxcursor-dev \
  libxdamage-dev \
  libxext-dev \
  libxfixes-dev \
  libxi-dev \
  libxinerama-dev \
  libxkbfile-dev \
  libxmu-dev \
  libxmuu-dev \
  libxpm-dev \
  libxrandr-dev \
  libxrender-dev \
  libxres-dev \
  libxss-dev \
  libxt-dev \
  libxtst-dev \
  libxv-dev \
  libxvmc-dev \
  libxxf86vm-dev \
  libxcb-render0-dev \
  libxcb-render-util0-dev \
  libxcb-xkb-dev \
  libxcb-icccm4-dev \
  libxcb-image0-dev \
  libxcb-keysyms1-dev \
  libxcb-randr0-dev \
  libxcb-shape0-dev \
  libxcb-sync-dev \
  libxcb-util-dev \
  libxcb-xfixes0-dev \
  libxcb-xinerama0-dev \
  xkb-data \
  libxcb-dri3-dev \
  uuid-dev

# Install Conan
echo "Installing and configuring Conan"
run_elevated python3 -m pip install \
  conan
conan profile new default --detect
conan profile update settings.compiler.libcxx=libstdc++11 default
