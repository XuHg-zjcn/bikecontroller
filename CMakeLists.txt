# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(DATAFLOW2_FLAGS "-DWHEAD_FORCE_ONLY=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DATAFLOW2_FLAGS}")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bikecontroller)
