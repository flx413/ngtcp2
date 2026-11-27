编译命令：
./configure PKG_CONFIG_PATH=/Users/guangguang/Code/quic/nghttp3/build/lib/pkgconfig \
       BORINGSSL_LIBS="-L/Users/guangguang/Code/boringssl/boringssl-0.20261105.0/build -lssl -lcrypto" \
       BORINGSSL_CFLAGS="-I/Users/guangguang/Code/boringssl/boringssl-0.20261105.0/include" \
       --with-boringssl LIBEV_CFLAGS="-I/opt/homebrew/include" LIBEV_LIBS="-L/opt/homebrew/lib -lev"