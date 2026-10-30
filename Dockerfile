FROM ubuntu:26.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential ca-certificates cmake ninja-build git pkg-config \
    gcc-16 g++-16 unzip wget curl xz-utils file python3 perl \
    qt6-base-dev qt6-declarative-dev qt6-wayland qt6-wayland-dev libqt6svg6-dev \
    liblayershellqtinterface-dev libwayland-dev wayland-protocols libwayland-bin \
    libxkbcommon-dev libfontconfig1-dev libfreetype-dev libpng-dev \
    libjpeg-turbo8-dev libgl1-mesa-dev libglu1-mesa-dev libglib2.0-dev \
    extra-cmake-modules sudo \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-16 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-16 100 \
    && update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-16 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-16 100

RUN echo "ubuntu ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/ubuntu \
    && chmod 0440 /etc/sudoers.d/ubuntu

WORKDIR /workspace
CMD ["/bin/bash"]
