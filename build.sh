if [ ! -d "build" ]; then
 mkdir build
fi

jerryscript/tools/build.py \
  --builddir=$(pwd)/jerry_build \
  --debug --error-messages ON\
  --jerry-debugger=ON\
  --line-info=ON\
  --cmake-param="-DCMAKE_INSTALL_PREFIX=$(pwd)/jerry_install/"
make -C $(pwd)/jerry_build install
