FROM --platform=$BUILDPLATFORM tonistiigi/xx:1.5.0 AS xx

FROM --platform=$BUILDPLATFORM ubuntu:24.04 AS build

COPY --from=xx --link / /

ARG TARGETPLATFORM

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive \
        apt-get install -y --no-install-recommends \
        git clang-18 make autoconf automake autotools-dev libtool dpkg-dev \
        pkg-config ca-certificates && \
    xx-info debian-arch && \
    DEBIAN_FRONTEND=noninteractive \
        xx-apt-get install -y --no-install-recommends \
        gcc-14 g++-14 libc6-dev binutils \
        libev-dev libjemalloc-dev && \
    ln -s /usr/bin/clang-18 /usr/bin/clang && \
    ln -s /usr/bin/clang++-18 /usr/bin/clang++

RUN if [ "$TARGETPLATFORM" = "linux/arm64" ]; then export OS_COMP="linux-aarch64"; fi && \
    git clone --depth 1 -b openssl-3.5.0-beta1 https://github.com/openssl/openssl && \
    cd openssl && \
    export CC=$(xx-info)-gcc-14 && \
    export CXX=$(xx-info)-g++-14 && \
    ./config no-shared --openssldir=/etc/ssl $OS_COMP && \
    make -j$(nproc) && make install_sw && cd .. && rm -rf openssl

RUN git clone --recursive --shallow-submodules --depth 1 https://github.com/ngtcp2/nghttp3 && \
    cd nghttp3 && \
    autoreconf -i && \
    ./configure --disable-dependency-tracking --enable-lib-only \
        --host=$(xx-clang --print-target-triple) \
        CC=$(xx-info)-gcc-14 \
        CXX=$(xx-info)-g++-14 \
        LDFLAGS="-fsanitize=address,undefined -fno-sanitize-recover=undefined" \
        CPPFLAGS="-fsanitize=address,undefined -fno-sanitize-recover=undefined -g3" && \
    make -j$(nproc) && make install && cd .. && rm -rf nghttp3

ARG NGTCP2_BRANCH=debug

RUN git clone --recursive --shallow-submodules --depth 1 -b $NGTCP2_BRANCH https://github.com/ngtcp2/ngtcp2 && \
    cd ngtcp2 && \
    autoreconf -i && \
    ./configure \
        --host=$(xx-clang --print-target-triple) \
        CC=$(xx-info)-gcc-14 \
        CXX=$(xx-info)-g++-14 \
        LIBTOOL_LDFLAGS="-static-libtool-libs" \
        LDFLAGS="-fsanitize=address,undefined -fno-sanitize-recover=undefined -static-libgcc -static-libstdc++ -static-libasan -static-libubsan" \
        CPPFLAGS="-fsanitize=address,undefined -fno-sanitize-recover=undefined -g3" \
        LIBEV_LIBS="-l:libev.a" \
        JEMALLOC_LIBS="-l:libjemalloc.a" \
        PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig \
        --disable-dependency-tracking \
        --with-openssl && \
    make -j$(nproc) && \
    cp examples/osslclient examples/osslserver \
        examples/h09osslclient examples/h09osslserver /usr/local/bin && \
    rm -rf ngtcp2

FROM martenseemann/quic-network-simulator-endpoint:latest

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        media-types

COPY --from=build --link /usr/local/bin/osslclient /usr/local/bin/osslserver \
    /usr/local/bin/h09osslclient /usr/local/bin/h09osslserver /usr/local/bin/
COPY --link --chmod=0775 run_endpoint.sh .

ENTRYPOINT [ "./run_endpoint.sh" ]
