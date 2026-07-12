FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    cmake \
    ninja-build \
    gdb \
    clang \
    clangd \
    device-tree-compiler \
    && rm -rf /var/lib/apt/lists/*