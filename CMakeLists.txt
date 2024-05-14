# $Id: CMakeLists.txt $

#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.10)
project(DAQ VERSION 1.0)

# Add C++11 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

SET(CMAKE_CXX_FLAGS "-O2")
SET(CMAKE_C_FLAGS "-O2")

# Add the source and header files for your library
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/source/*.cpp)
file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/include/*.h)

# Create a shared library
add_library(DAQ SHARED source/DAQ.cpp source/DAQDevice.cpp source/DAQCommon.cpp)

# Add PIC flag if needed
set_target_properties(DAQ PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Locate Root libraries
find_package(ROOT REQUIRED COMPONENTS RIO Hist Core Graf Graf3d Tree)

include_directories(${ROOT_INCLUDE_DIRS})

# Include the drivers submodule
add_subdirectory(drivers)

# Link with Root libraries
target_link_libraries(DAQ PRIVATE ${ROOT_LIBRARIES} drivers)

# Set the output name of the library (optional)
# By default, it will be "libDAQ.so" on Linux,
# "DAQ.dll" on Windows, and "libDAQ.dylib" on macOS.
# Uncomment and modify if you want a different name
# set_target_properties(DAQ PROPERTIES OUTPUT_NAME "DAQ")

# Include the current directory for header files
target_include_directories(DAQ PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Set installation directories (optional)
# By default, it will install in the system directories
# Modify as needed
# For example:
# set(INSTALL_LIB_DIR lib)
# set(INSTALL_INCLUDE_DIR include)
# install(TARGETS DAQ DESTINATION ${INSTALL_LIB_DIR})
# install(FILES ${HEADERS} DESTINATION ${INSTALL_INCLUDE_DIR})

# Optionally, you can set additional compiler flags
# For example, to enable debug symbols
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
