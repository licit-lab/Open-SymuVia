# Open-SymuVia

**Note**:
This repository is an archive: For recent relases and more information, please visit: [licit-lab/symuvia](https://github.com/licit-lab/symuvia)

**Open-SymuVia** is an open-source traffic simulator. 
It compounds of:
- a dynamic microscopic traffic simulator based on the kinematic wave model with a Lagrangian resolution,
- a dynamic traffic assignment model based on users' equilibrium
- several useful tools to handle your simulations

Multiples components to reproduce urban networks (intersections, lane-changing, multi-class, etc.) are embedded in **Open-SymuVia**. It computes position, speed and acceleration of each vehicle on the network with a 1 second resolution.

![alt tag](https://github.com/Ifsttar/Open-SymuVia/blob/master/Doc/img/pict1.png)

**Open-SymuVia** is developped by the [LICIT](http://www.licit.ifsttar.fr/) an University Gustave Eiffel / ENTPE joint research laboratory. 

Open-SymuVia is distributed under **[LGPL V3 license](https://github.com/licit-lab/Open-SymuVia/blob/master/lgpl-3.0.txt)**.

# Build Open-SymuVia

![](https://img.shields.io/badge/platform-osx--64-blue) ![](https://img.shields.io/badge/platform-linux-green)

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
      wget bzip2 ca-certificates \
      xz-utils \
      build-essential \ 
      curl \   
      git \
      libxqilla-dev \
      libboost-all-dev \
      aptitude \
      gdal-bin \
      rapidjson-dev \
      libgdal-dev \
      unixodbc \
      libpq-dev &&\
      aptitude search -y \
      boost \
      && rm -rf /var/lib/apt/lists/*
```

On OS X `install`

```
      brew install boost boost-python3 gdal xqilla rapidjson unixodbc # python (optional) in case anaconda is not installed 
```
