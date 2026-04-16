cmake_minimum_required(VERSION 3.30.0)
project(imgui)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

find_package(Vulkan REQUIRED)

add_library(imgui)
target_sources(imgui PRIVATE ${SRCS})
target_link_libraries(imgui PRIVATE Vulkan::Vulkan)
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui")
target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../glfw/include")
