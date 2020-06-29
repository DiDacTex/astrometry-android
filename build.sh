#!/bin/bash

###### PATHS AT BOTTOM MUST BE CHANGED FOR YOUR MACHINE

set -e

export API=28

# aarch64-linux-android i686-linux-android x86_64-linux-android

# dropped support for armv7a-linux-androideabi

for TARGET in aarch64-linux-android #$@
do
    export TARGET

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

    export CFITS_INC="-I/home/jonathan/work/build/$ABI/cfitsio/include"
    export CFITS_LIB="/home/jonathan/work/build/$ABI/cfitsio/lib/libcfitsio.a"
    #export CFITS_LIB="-L/home/jonathan/work/build/$ABI/cfitsio/lib -lcfitsio"

    #export OPTIMIZE=no

    make
    make install INSTALL_DIR="/home/jonathan/work/build/$ABI/anet-install"
    make clean
    git clean -fX

    #pushd /home/jonathan/work/build/$ABI > /dev/null

    #rm -rf package
    #rm -rf astrometry/bin
    #mkdir -p astrometry/bin
    #cp -d anet-install/bin/* astrometry/bin
    #rm -f astrometry.tar.gz
    #tar zcf astrometry.tar.gz astrometry/
    #explorer.exe .

    #popd > /dev/null
done
