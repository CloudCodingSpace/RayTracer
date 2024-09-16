cmake_minimum_required(VERSION 3.5.0)
project(tinyobj)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(tinyobj)
target_sources(tinyobj PRIVATE ${SRCS})
target_include_directories(tinyobj PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")