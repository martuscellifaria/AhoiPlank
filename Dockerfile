FROM ubuntu:26.04

ARG DEBIAN_FRONTEND=noninteractive
ARG LLVM_MINGW_VERSION=20260616
ARG LLVM_MINGW_UBUNTU=22.04

ENV XWIN=/opt/xwin
ENV PATH="/root/.cargo/bin:${PATH}"
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    ninja-build \
    git \
    pkg-config \
    g++-16-x86-64-linux-gnu \
    gcc-16-x86-64-linux-gnu \
    g++-16-aarch64-linux-gnu \
    gcc-16-aarch64-linux-gnu \
    gcc-16 \
    g++-16 \
    clang \
    lld \
    llvm \
    clang-tools \
    cargo \
    rustc \
    libjpeg8 \
    libwebp7 \
    libwebpmux3 \
    libwebpdemux2 \
    libpng16-16 \
    libtiff6 \
    unzip \
    wget \
    curl \
    xz-utils \
    file \
    && rm -rf /var/lib/apt/lists/*

RUN ARCH=$(uname -m) && \
    echo "Detected host architecture: $ARCH" && \
    case "$ARCH" in \
        aarch64|arm64) SUFFIX="aarch64" ;; \
        x86_64|amd64) SUFFIX="x86_64" ;; \
        *) echo "Unsupported architecture: $ARCH"; exit 1 ;; \
    esac && \
    LLVM_MINGW_DIR="/opt/llvm-mingw-${LLVM_MINGW_VERSION}-ucrt-ubuntu-${LLVM_MINGW_UBUNTU}-${SUFFIX}" && \
    LLVM_MINGW_ARCHIVE="llvm-mingw-${LLVM_MINGW_VERSION}-ucrt-ubuntu-${LLVM_MINGW_UBUNTU}-${SUFFIX}.tar.xz" && \
    wget -q "https://github.com/mstorsjo/llvm-mingw/releases/download/${LLVM_MINGW_VERSION}/${LLVM_MINGW_ARCHIVE}" && \
    tar -xf "${LLVM_MINGW_ARCHIVE}" -C /opt && \
    rm "${LLVM_MINGW_ARCHIVE}" && \
    printf '%s\n' "${SUFFIX}" > /opt/llvm-mingw-arch.txt && \
    test -x "${LLVM_MINGW_DIR}/bin/clang++"

RUN SUFFIX=$(cat /opt/llvm-mingw-arch.txt) && \
    LLVM_MINGW_DIR="/opt/llvm-mingw-${LLVM_MINGW_VERSION}-ucrt-${LLVM_MINGW_UBUNTU}-${SUFFIX}" && \
    printf '#!/usr/bin/env bash\nexec "%s/bin/clang++" --target=x86_64-w64-mingw32 "$@"\n' "${LLVM_MINGW_DIR}" > /usr/local/bin/mingw-clang++ && \
    printf '#!/usr/bin/env bash\nexec "%s/bin/clang++" --target=aarch64-w64-mingw32 "$@"\n' "${LLVM_MINGW_DIR}" > /usr/local/bin/mingw-arm64-clang++ && \
    chmod +x /usr/local/bin/mingw-clang++ /usr/local/bin/mingw-arm64-clang++

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-16 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-16 100 \
    && update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-16 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-16 100

RUN cargo install xwin --locked && \
    rm -rf /opt/xwin /opt/.xwin-cache && \
    cd /opt && \
    xwin --accept-license splat --output /opt/xwin && \
    test -f /opt/xwin/crt/include/vector && \
    test -f /opt/xwin/crt/lib/x86_64/libcmt.lib && \
    test -f /opt/xwin/crt/lib/x86_64/oldnames.lib && \
    test -f /opt/xwin/sdk/lib/ucrt/x86_64/ucrt.lib && \
    find /opt/xwin -iname 'msvcrt.lib' -print -quit | grep -q .

RUN command -v clang-cl && \
    command -v lld-link && \
    command -v llvm-rc && \
    clang-cl --version && \
    lld-link --version && \
    xwin --version

CMD ["/bin/bash"]
