#!/usr/bin/env bash
# Install the nix package manager
# The script requires curl & gpg installed

# Constants
NIX_VERSION=2.9.2

function exit_if_command_not_available {
  if [ -z $(which $1) ]; then
    echo "Script requires the $1 command to be available"
    exit 1
  fi
}

# Pre-flight checks
exit_if_command_not_available curl
exit_if_command_not_available gpg

# Install for current user
curl -o /tmp/install-nix${NIX_VERSION} https://releases.nixos.org/nix/nix-${NIX_VERSION}/install && \
curl -o /tmp/install-nix${NIX_VERSION}.asc https://releases.nixos.org/nix/nix-${NIX_VERSION}/install.asc && \
gpg2 --keyserver hkps://keyserver.ubuntu.com --recv-keys B541D55301270E0BCF15CA5D8170B4726D7198DE && \
gpg2 --verify /tmp/install-nix${NIX_VERSION}.asc && \
sh /tmp/install-nix${NIX_VERSION} && \
mkdir -p ~/.config/nix/ && \
echo "experimental-features = nix-command flakes" >> ~/.config/nix/nix.conf && \
echo "system-features = kvm" >> ~/.config/nix/nix.conf
