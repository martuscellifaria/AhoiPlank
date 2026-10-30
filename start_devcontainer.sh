#!/usr/bin/env bash
set -e

if [ -f /.dockerenv ]; then
    echo "You are already in a Docker container. Aborting."
    exit 1
fi

ARCH=$(uname -m)
case "$ARCH" in
    x86_64|amd64) PLATFORM="linux/amd64" ;;
    aarch64|arm64) PLATFORM="linux/arm64" ;;
    *) echo "Unsupported host arch: $ARCH"; exit 1 ;;
esac

IMAGE="ahoiplank-dev:${ARCH}"
docker build -t "$IMAGE" .
docker run -it --rm \
    -v "$(pwd):/workspace" \
    -w /workspace \
    --user "$(id -u):$(id -g)" \
    "$IMAGE"
