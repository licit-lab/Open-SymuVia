#!/bin/bash

if [[ -d build ]]; then
    rm -rf build
fi

mkdir build
cd build

# Needed for Boost: https://github.com/VowpalWabbit/vowpal_wabbit/issues/1095#issuecomment-330327771
if [ `uname` == Linux ]
then
  export LD_LIBRARY_PATH="${PREFIX}/lib:${LD_LIBRARY_PATH}"
  export LIBRARY_PATH="${PREFIX}/lib:${LIBRARY_PATH}"
fi


cmake .. \
      -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
      -DCMAKE_PREFIX_PATH="${PREFIX}" \
      -DPython3_FIND_STRATEGY=LOCATION

make -j${CPU_COUNT}
make install
