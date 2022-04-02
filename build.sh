#!/bin/bash

if [ ! -d "build" ]; then
 mkdir build
fi

cd build

clang noise.c minifb/build/libminifb.a quickjs/libquickjs.a -lm \
  -framework Cocoa -framework QuartzCore \
  -framework Metal -framework MetalKit && ./a.out
