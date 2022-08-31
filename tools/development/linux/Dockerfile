# See here for image contents: https://github.com/microsoft/vscode-dev-containers/tree/v0.245.0/containers/ubuntu/.devcontainer/base.Dockerfile

# [Choice] Ubuntu version (use ubuntu-22.04 or ubuntu-18.04 on local arm64/Apple Silicon): ubuntu-22.04, ubuntu-20.04, ubuntu-18.04
ARG VARIANT="jammy"
FROM mcr.microsoft.com/vscode/devcontainers/base:0-${VARIANT}

# Install base tools
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    curl \
    freeglut3-dev \
    gpg \
    mesa-utils-extra \
    libgl1-mesa-dev \
    ninja-build \
    python3-setuptools \
    python3-pip \
    xz-utils

RUN pip3 install \
    lxml \
    flask \
    h5py \
    pyinstaller \
    wheel

# Install nix as the user
USER vscode
COPY install-and-configure-nix.sh /tmp/install-and-configure-nix.sh
RUN /tmp/install-and-configure-nix.sh
