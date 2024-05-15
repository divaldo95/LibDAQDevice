# XDMA DAQ Device library and C# wrapper

DAQ Device code can be found in `source` and `include` directories and only works on Linux.
`drivers` folder contains the dependencies.
Building the library will result in a shared library (.so)


## Prerequisities
* CERN Root (tested on 6.30)
* Linux
* CMake

## Building
```
git clone https://github.com/divaldo95/LibDAQDevice.git
mkdir build && cd build
cmake ../
make -jN (where N is the number of jobs to run simultaneously)
```

## Changelog
### 2024.05.15
- Implemented new interfaces
- Event driven measurement
- Minor modifications

### 2024.05.14
- Created the library with old functions and interfaces

# C# test application

## Prerequisities
* Dotnet Core 8.0
* Library built

## How to run
1. Build C# app, this will create `CSharpTest/bin/Debug/net8.0` folder
2. Copy the library to the above mentioned folder (.so or .dylib)
3. Run and check the output (In [CSharpTest](CSharpTest) folder issue `dotnet run` command)

## Changelog
### 2024.05.15
- Implemented new interfaces
- Added stripped down version of Serial port and its devices
- Working test code

### 2024.05.14
- Test application for the library