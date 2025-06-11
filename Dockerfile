FROM debian:stable-slim

# Install basic build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    valgrind \
    libgtest-dev \
    gcc-arm-none-eabi \
    gdb-arm-none-eabi \
    clang

# Set working directory
WORKDIR /workspace

# Default command
CMD ["/bin/bash"]
