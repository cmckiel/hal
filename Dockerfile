FROM ubuntu:22.04

# Install basic build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    valgrind \
    libgtest-dev \
    gcc-arm-none-eabi \
    clang \
    clang-tidy \
    cppcheck \
    doxygen \
    graphviz

# Set working directory
WORKDIR /workspace

# Default command
CMD ["/bin/bash"]
