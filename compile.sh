#!/bin/bash

if [ ! -e build ]
then
    mkdir build
fi

TARGET="quill"
SRC_DIR="./src/"
BUILD_DIR="./build/"
CFLAGS="-Wall -Wextra -Werror -std=c99"
INC_PATHS=("/usr/include/freetype2")
LIBS=("SDL2" "freetype")

for i in ${LIBS[@]}
do
    OUT_LIBS+=" -l$i"
done

for i in ${INC_PATHS[@]}
do
    OUT_PATHS+=" -I $i"
done

GCC_ARG="$CFLAGS -g $SRC_DIR/*.c -o $BUILD_DIR/$TARGET $OUT_PATHS $OUT_LIBS"
gcc $GCC_ARG
