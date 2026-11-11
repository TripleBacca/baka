# Cross toolchain + user-mode emulator for the MIPS64 n64 little-endian backend.
FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc-mips64el-linux-gnuabi64 libc6-dev-mips64el-cross qemu-user \
    && rm -rf /var/lib/apt/lists/*
ENV QEMU_LD_PREFIX=/usr/mips64el-linux-gnuabi64
WORKDIR /work
