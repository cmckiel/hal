FROM debian:stable-slim

# Install basic build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    valgrind \
    gcc-arm-none-eabi \
    gdb-arm-none-eabi

# Install dependencies required by submodule
# Example:
# RUN apt-get install -y libfoo-dev libbar-dev

# Set working directory
WORKDIR /workspace

# Default command
CMD ["/bin/bash"]
