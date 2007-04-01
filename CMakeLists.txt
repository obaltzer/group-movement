project(gridcube)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -DWITH_LIMITS")
add_subdirectory(fim-app ${gridcube_SOURCE_DIR}/bin)
add_subdirectory(src ${gridcube_SOURCE_DIR}/bin)
