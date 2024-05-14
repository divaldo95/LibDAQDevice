# Collect all the source files
file(GLOB SOURCES "source/*.cpp")
file(GLOB HEADERS "include/*.h")

# Add the library
add_library(drivers OBJECT ${SOURCES} ${HEADERS})

# Add PIC flag if needed
set_target_properties(drivers PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Include directories for the submodule
target_include_directories(drivers PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")