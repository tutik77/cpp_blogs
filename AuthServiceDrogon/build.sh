cd ./build &&
  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib .. &&
  make
