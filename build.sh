#!/bin/bash

set -e

export API=28

#for TARGET in aarch64-linux-android armv7a-linux-androideabi i686-linux-android x86_64-linux-android
#do
    export TARGET=$1
    case $TARGET in
        aarch64*)
            export ABI=arm64-v8a
            ;;
        armv7a*)
            export ABI=armeabi-v7a
            ;;
        i686*)
            export ABI=x86
            ;;
        x86_64*)
            export ABI=x86-64
            ;;
    esac
    export CC=$TOOLCHAIN/bin/$TARGET$API-clang
    export CXX=$TOOLCHAIN/bin/$TARGET$API-clang++
    if [[ "$TARGET" = "armv7a-linux-androideabi" ]]
    then
        export TRIPLE=arm-linux-androideabi
    else
        export TRIPLE=$TARGET
    fi
    export AR=$TOOLCHAIN/bin/$TRIPLE-ar
    export AS=$TOOLCHAIN/bin/$TRIPLE-as
    export LD=$TOOLCHAIN/bin/$TRIPLE-ld
    export RANLIB=$TOOLCHAIN/bin/$TRIPLE-ranlib
    export STRIP=$TOOLCHAIN/bin/$TRIPLE-strip

    make
#done
