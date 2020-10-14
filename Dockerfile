FROM raspberry-xc-toolchain

# Install tools
RUN apt-get update && \
    apt-get install -y build-essential gawk git texinfo bison file bc flex libssl-dev libncurses5-dev automake curl libtool pkg-config autoconf-archive rsync python3

# Download libraries
RUN curl https://codeload.github.com/libevent/libevent/tar.gz/release-2.1.12-stable -o /downloads/libevent.tar.gz && \
    curl https://codeload.github.com/openssl/openssl/tar.gz/OpenSSL_1_0_2s -o /downloads/openssl.tar.gz && \
    curl https://zlib.net/zlib-1.2.11.tar.gz -o /downloads/zlib.tar.gz && \
    curl https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/snapshot/libgpiod-1.4.4.tar.gz -o /downloads/libgpiod.tar.gz

# Build openssl
RUN mkdir /downloads/openssl && tar -xz -C /downloads/openssl -f /downloads/openssl.tar.gz --strip-components 1
WORKDIR /downloads/openssl
RUN ./Configure linux-generic32 shared --prefix=/opt/rpi-xc/arm-linux-gnueabihf --openssldir=/opt/rpi-xc/arm-linux-gnueabihf/openssl --cross-compile-prefix=/opt/rpi-xc/bin/arm-linux-gnueabihf- && make depend && make -j$(nproc) && make install

# Build libevent
RUN mkdir /downloads/libevent && tar -xz -C /downloads/libevent -f /downloads/libevent.tar.gz --strip-components 1
WORKDIR /downloads/libevent
RUN sh autogen.sh && ./configure --build=x86_64-linux-gnu --host=arm-linux-gnueabihf --prefix=/opt/rpi-xc/arm-linux-gnueabihf --with-sysroot=/opt/rpi-xc/arm-linux-gnueabihf ac_cv_func_malloc_0_nonnull=yes && make -j$(nproc) && make install

# Build zlib
RUN mkdir /downloads/zlib && tar -xz -C /downloads/zlib -f /downloads/zlib.tar.gz --strip-components 1
WORKDIR /downloads/zlib
RUN CHOST=arm-linux-gnueabihf ./configure --prefix=/opt/rpi-xc/arm-linux-gnueabihf && make -j$(nproc) && make install

# Build libgpiod
RUN mkdir /downloads/libgpiod && tar -xz -C /downloads/libgpiod -f /downloads/libgpiod.tar.gz --strip-components 1
WORKDIR /downloads/libgpiod
RUN sh autogen.sh --build=x86_64-linux-gnu --host=arm-linux-gnueabihf --prefix=/opt/rpi-xc/arm-linux-gnueabihf --with-sysroot=/opt/rpi-xc/arm-linux-gnueabihf ac_cv_func_malloc_0_nonnull=yes && make -j$(nproc) && make install

WORKDIR /code
CMD bash
