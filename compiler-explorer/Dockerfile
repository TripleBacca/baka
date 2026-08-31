# Self-contained Compiler Explorer image (built from source) that can drive
# baka, a parse-only compiler. Adapted from madduci/docker-compiler-explorer.
#
# The baka binary itself is NOT baked in: docker-compose bind-mounts
# ../build/baka so rebuilding the compiler doesn't rebuild this image.
# node:22-trixie provides a libstdc++ (GCC 14) new enough to run baka built on
# the host, whose distro must be at most as old as Debian 13 toolchain-wise.
FROM node:22-trixie-slim

LABEL org.opencontainers.image.source="https://github.com/compiler-explorer/compiler-explorer"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates git make rsync python3 build-essential \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 https://github.com/compiler-explorer/compiler-explorer.git /compiler-explorer

COPY config/c.local.properties /compiler-explorer/etc/config/c.local.properties

RUN mkdir -p /opt/baka/bin
COPY baka-ce /opt/baka/bin/baka-ce
RUN chmod +x /opt/baka/bin/baka-ce

WORKDIR /compiler-explorer

# npm clean-install + disassembler scripts + webpack frontend + tsc backend
RUN npm ci \
    && npm run webpack \
    && npm run ts-compile

EXPOSE 10240

ENTRYPOINT ["make"]
CMD ["run-only"]