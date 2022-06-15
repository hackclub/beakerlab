#!/bin/bash

if [ ! -d "build" ]; then
 mkdir build
fi

cd build

clang ../main.c \
  -g \
  -Wall -Werror -Wno-error=unused-function\
  ../minifb/build/libminifb.a \
  -lm \
  -framework Cocoa -framework QuartzCore \
  -framework Metal -framework MetalKit &&
./a.out
