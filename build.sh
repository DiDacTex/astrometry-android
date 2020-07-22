#!/bin/bash

###### PATHS AT BOTTOM MUST BE CHANGED FOR YOUR MACHINE

set -e

export API=28

# ABIs are:
# arm64-v8a armeabi-v7a x86 x86-64

# ***Only arm64-v8a is confirmed to work!***

for ABI in arm64-v8a
do
    # Set architecture variables
    case $ABI in
        arm64-v8a)
            export COMPILER_TRIPLE=aarch64-linux-android
            LIB_TRIPLE=$COMPILER_TRIPLE
            ;;
        armeabi-v7a)
            export COMPILER_TRIPLE=armv7a-linux-androideabi
            LIB_TRIPLE=arm-linux-androideabi
            ;;
        x86)
            export COMPILER_TRIPLE=i686-linux-android
            LIB_TRIPLE=$COMPILER_TRIPLE
            ;;
        x86-64)
            export COMPILER_TRIPLE=x86_64-linux-android
            LIB_TRIPLE=$COMPILER_TRIPLE
            ;;
    esac

    # Identify tools for make
    export CC=$TOOLCHAIN/bin/$COMPILER_TRIPLE$API-clang
    export CXX=$TOOLCHAIN/bin/$COMPILER_TRIPLE$API-clang++
    export AR=$TOOLCHAIN/bin/$LIB_TRIPLE-ar
    export AS=$TOOLCHAIN/bin/$LIB_TRIPLE-as
    export LD=$TOOLCHAIN/bin/$LIB_TRIPLE-ld
    export RANLIB=$TOOLCHAIN/bin/$LIB_TRIPLE-ranlib
    export STRIP=$TOOLCHAIN/bin/$LIB_TRIPLE-strip

    # Installation prefix
    PREFIX="$PWD/build/$ABI"
    mkdir -p $PREFIX

    # Build cfitsio
    pushd cfitsio > /dev/null
    ./configure --host=$COMPILER_TRIPLE --prefix="$PREFIX/cfitsio"
    make
    make install
    make distclean
    popd > /dev/null

    # Set path to cfitsio for astrometry.net
    CFITS_PATH="$PREFIX/cfitsio"
    export CFITS_INC="-I$CFITS_PATH/include"
    # Note: astrometry.net documentation says to use CFITS_SLIB for static
    # linking, but that does not work. CFITS_LIB is required to get astrometry
    # to statically link with cfitsio.
    export CFITS_LIB="$CFITS_PATH/lib/libcfitsio.a"

    # Build astrometry.net
    make
    make install INSTALL_DIR="$PREFIX/astrometry"
    make clean
    git clean -fx

    # Collect files for Android
    pushd $PREFIX > /dev/null
    JNILIBS="android/jniLibs/$ABI"
    mkdir -p "$JNILIBS"
    cp astrometry/lib/libastrometry.so $JNILIBS
    #cp astrometry/bin/solve-field "$JNILIBS/lib..solve-field..so"
    #cp astrometry/bin/astrometry-engine "$JNILIBS/lib..astrometry-engine..so"
    # Index files are architecture-independent
    # You need to provide index files in the data directory
    # Get them from http://data.astrometry.net/4100/
    # Recommended set: 4115 to 4119
    mkdir -p "android/assets/data"

    popd > /dev/null
done
