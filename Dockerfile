FROM ubuntu:26.04

ARG DEBIAN_FRONTEND=noninteractive
ARG QT_VERSION=6.8.1
ARG LAYERSHELLQT_VERSION=6.5.0

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential ca-certificates cmake ninja-build git pkg-config \
    g++-16-x86-64-linux-gnu gcc-16-x86-64-linux-gnu \
    g++-16-aarch64-linux-gnu gcc-16-aarch64-linux-gnu \
    gcc-16 g++-16 clang lld llvm clang-tools \
    libjpeg8 libwebp7 libwebpmux3 libwebpdemux2 libpng16-16 libtiff6 \
    unzip wget curl xz-utils file python3 perl extra-cmake-modules \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-16 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-16 100 \
    && update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-16 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-16 100

RUN ARCH=$(dpkg --print-architecture) \
    && if [ "$ARCH" = "arm64" ]; then FOREIGN="amd64"; else FOREIGN="arm64"; fi \
    && dpkg --add-architecture "$FOREIGN" \
    && rm -f /etc/apt/sources.list.d/ubuntu.sources \
    && printf '%s\n' \
        'Types: deb' \
        'URIs: http://archive.ubuntu.com/ubuntu/' \
        'Suites: resolute resolute-updates resolute-backports resolute-security' \
        'Components: main restricted universe multiverse' \
        'Architectures: amd64' \
        'Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg' \
        '' \
        'Types: deb' \
        'URIs: http://ports.ubuntu.com/ubuntu-ports/' \
        'Suites: resolute resolute-updates resolute-backports resolute-security' \
        'Components: main restricted universe multiverse' \
        'Architectures: arm64' \
        'Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg' \
        > /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        libc6-dev:amd64 libc6-dev:arm64 \
        libstdc++-16-dev:amd64 libstdc++-16-dev:arm64 \
        libglib2.0-dev:amd64 libglib2.0-dev:arm64 \
        libwayland-dev:amd64 libwayland-dev:arm64 libwayland-bin \
        wayland-protocols:amd64 wayland-protocols:arm64 \
        libxkbcommon-dev:amd64 libxkbcommon-dev:arm64 \
        libfontconfig1-dev:amd64 libfontconfig1-dev:arm64 \
        libfreetype-dev:amd64 libfreetype-dev:arm64 \
        libpng-dev:amd64 libpng-dev:arm64 \
        libjpeg-turbo8-dev:amd64 libjpeg-turbo8-dev:arm64 \
        libgl1-mesa-dev:amd64 libgl1-mesa-dev:arm64 \
        libglu1-mesa-dev:amd64 libglu1-mesa-dev:arm64 \
    && rm -rf /var/lib/apt/lists/*

RUN cd /opt && \
    git clone --filter=blob:none --branch v${QT_VERSION} https://code.qt.io/qt/qt5.git qt-src-host && \
    cd qt-src-host && \
    git submodule update --init --recursive --depth 1 qtbase qtshadertools && \
    sed -i 's|#include <stack>|#include <cstdint>\n#include <stack>|' qtshadertools/src/3rdparty/glslang/SPIRV/SpvBuilder.h && \
    ./configure -prefix /opt/qt6-host -release -opensource -confirm-license -nomake examples -nomake tests \
    && cmake --build . --parallel $(nproc) \
    && cmake --install . \
    && cd / && rm -rf /opt/qt-src-host

RUN printf '%s\n' \
    'set(CMAKE_SYSTEM_NAME Linux)' \
    'set(CMAKE_SYSTEM_PROCESSOR x86_64)' \
    'set(CMAKE_C_COMPILER x86_64-linux-gnu-gcc-16)' \
    'set(CMAKE_CXX_COMPILER x86_64-linux-gnu-g++-16)' \
    'set(CMAKE_SYSROOT /)' \
    'set(CMAKE_FIND_ROOT_PATH /usr/lib/x86_64-linux-gnu /usr/share)' \
    'set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)' \
    'set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)' \
    'set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)' \
    'set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)' \
    > /tmp/toolchain-x86_64.cmake && \
    cd /opt && \
    git clone --filter=blob:none --branch v${QT_VERSION} https://code.qt.io/qt/qt5.git qt-src-x86_64 && \
    cd qt-src-x86_64 && \
    git submodule update --init --recursive --depth 1 qtbase qtdeclarative qtwayland qtshadertools qtsvg && \
    sed -i 's|#include <stack>|#include <cstdint>\n#include <stack>|' qtshadertools/src/3rdparty/glslang/SPIRV/SpvBuilder.h && \
    PKG_CONFIG_LIBDIR=/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig PKG_CONFIG_SYSROOT_DIR=/ \
    ./configure -prefix /opt/qt6-x86_64 -release -opensource -confirm-license -nomake examples -nomake tests \
        -toolchain /tmp/toolchain-x86_64.cmake \
        -qt-host-path /opt/qt6-host \
    && cmake --build . --parallel $(nproc) \
    && cmake --install . \
    && cd / && rm -rf /opt/qt-src-x86_64

RUN printf '%s\n' \
    'set(CMAKE_SYSTEM_NAME Linux)' \
    'set(CMAKE_SYSTEM_PROCESSOR aarch64)' \
    'set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc-16)' \
    'set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++-16)' \
    'set(CMAKE_SYSROOT /)' \
    'set(CMAKE_FIND_ROOT_PATH /usr/lib/aarch64-linux-gnu /usr/share)' \
    'set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)' \
    'set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)' \
    'set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)' \
    'set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)' \
    > /tmp/toolchain-aarch64.cmake && \
    cd /opt && \
    git clone --filter=blob:none --branch v${QT_VERSION} https://code.qt.io/qt/qt5.git qt-src-aarch64 && \
    cd qt-src-aarch64 && \
    git submodule update --init --recursive --depth 1 qtbase qtdeclarative qtwayland qtshadertools qtsvg && \
    sed -i 's|#include <stack>|#include <cstdint>\n#include <stack>|' qtshadertools/src/3rdparty/glslang/SPIRV/SpvBuilder.h && \
    PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig PKG_CONFIG_SYSROOT_DIR=/ \
    ./configure -prefix /opt/qt6-arm64 -release -opensource -confirm-license -nomake examples -nomake tests \
        -toolchain /tmp/toolchain-aarch64.cmake \
        -qt-host-path /opt/qt6-host \
    && cmake --build . --parallel $(nproc) \
    && cmake --install . \
    && cd / && rm -rf /opt/qt-src-aarch64

RUN git clone --depth 1 --branch v${LAYERSHELLQT_VERSION} \
        https://invent.kde.org/plasma/layer-shell-qt.git /tmp/lsqt-x86_64 \
    && PKG_CONFIG_LIBDIR=/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig PKG_CONFIG_SYSROOT_DIR=/ \
    cmake -S /tmp/lsqt-x86_64 -B /tmp/lsqt-x86_64/build -G Ninja \
        -DCMAKE_C_COMPILER=x86_64-linux-gnu-gcc-16 \
        -DCMAKE_CXX_COMPILER=x86_64-linux-gnu-g++-16 \
        -DCMAKE_PREFIX_PATH=/opt/qt6-x86_64 \
        -DCMAKE_INSTALL_PREFIX=/opt/layershellqt-x86_64 \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
        -DCMAKE_FIND_ROOT_PATH="/usr/lib/x86_64-linux-gnu;/usr/share;/opt/qt6-x86_64" \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    && cmake --build /tmp/lsqt-x86_64/build \
    && cmake --install /tmp/lsqt-x86_64/build \
    && rm -rf /tmp/lsqt-x86_64

RUN git clone --depth 1 --branch v${LAYERSHELLQT_VERSION} \
        https://invent.kde.org/plasma/layer-shell-qt.git /tmp/lsqt-arm64 \
    && PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig PKG_CONFIG_SYSROOT_DIR=/ \
    cmake -S /tmp/lsqt-arm64 -B /tmp/lsqt-arm64/build -G Ninja \
        -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc-16 \
        -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++-16 \
        -DCMAKE_PREFIX_PATH=/opt/qt6-arm64 \
        -DCMAKE_INSTALL_PREFIX=/opt/layershellqt-arm64 \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
        -DCMAKE_FIND_ROOT_PATH="/usr/lib/aarch64-linux-gnu;/usr/share;/opt/qt6-arm64" \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    && cmake --build /tmp/lsqt-arm64/build \
    && cmake --install /tmp/lsqt-arm64/build \
    && rm -rf /tmp/lsqt-arm64

RUN apt-get update && apt-get install -y --no-install-recommends \
    sudo libc6 libc6:amd64 libc6:arm64 libstdc++6 libstdc++6:amd64 libstdc++6:arm64 \
    && rm -rf /var/lib/apt/lists/* \
    && echo "ubuntu ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/ubuntu \
    && chmod 0440 /etc/sudoers.d/ubuntu

ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

CMD ["/bin/bash"]
