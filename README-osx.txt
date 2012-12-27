# CMake command used

cmake \
    -DBOOST_ROOT=~/code/boost \
    -DBOOST_INCLUDE_DIR=/usr/local/include/boost-1_53 \
    -DBOOST_LIBRARY_DIR=/usr/local/lib \
    -DBOOST_SUFFIX=-clang-darwin-mt-1_53 \
    -DCMAKE_CXX_COMPILER=~/code/llvm-build/Release/bin/clang \
    -DCMAKE_C_COMPILER=~/code/llvm-build/Release/bin/clang \
    -DCMAKE_CXX_FLAGS="-std=c++11 -stdlib=libc++ -ftemplate-depth=256" \
    -DHPX_USE_GENERIC_COROUTINE_CONTEXT=1 \
    -DHPX_HAVE_NATIVE_TLS:BOOL=OFF \
    ../hpx
