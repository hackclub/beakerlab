#!/bin/bash

if [ ! -d "build" ]; then
 mkdir build
fi

cd build

clang ../main.c \
  -g \
  -I../jerry_install/include \
  -Wall -Werror -Wno-error=unused-function\
  ../minifb/build/libminifb.a \
  ../jerry_build/lib/libjerry-core.a \
  ../jerry_build/lib/libjerry-ext.a \
  ../jerry_build/lib/libjerry-port.a \
  -lm \
  -framework Cocoa -framework QuartzCore \
  -framework Metal -framework MetalKit &&
./a.out
