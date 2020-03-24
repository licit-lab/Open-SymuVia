# Build SymuCore - SymuVia

![](https://img.shields.io/badge/platform-osx--64-blue) 

A simple project structure to Build `SymuVia`. 

1. Get the repo 
  `git clone https://github.com/becarie/symudev.git && cd build-symuvia`
2. Create the directory to build e.g `mkdir build`
3. Go to the coresonding directory `cd build` 
4. Generate config pointing to the place where the file `CMakeLists.txt` is placed `cmake ..`
5. Build your target via `cmake -build .` or `make`

## Dependencies 

On Linux `install` 

```
      apt-get update && apt-get install -y \
      cmake \
      xqilla \
      libboost-all-dev \
      aptitude \
      gdal-bin \
      rapidjson-dev  &&\
      aptitude search -y \
      boost 
```

On OS X `install`

```
      brew install boost boost-python3 gdal xqilla rapidjson # python (optional) in case anaconda is not installed 
```

LICIT - 2020 