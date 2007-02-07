project(gridcube)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")
add_subdirectory(fim-app ${gridcube_SOURCE_DIR}/bin)
add_subdirectory(src ${gridcube_SOURCE_DIR}/bin)
