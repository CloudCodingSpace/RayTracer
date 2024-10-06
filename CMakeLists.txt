cmake_minimum_required(VERSION 3.8.0)
cmake_policy(SET CMP0069 NEW)
project(RayTracer)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

set(CMAKE_BUILD_TYPE Debug)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(CMAKE_CXX_STANDARD 20)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    add_compile_options(/arch:AVX2)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

# add_subdirectory(third-party/imgui)
# add_subdirectory(third-party/glfw)
# add_subdirectory(third-party/glad)
add_subdirectory(third-party/stb)
add_subdirectory(third-party/tinyobj)
add_subdirectory(third-party/glm)

# find_package(OpenGL REQUIRED)

add_executable(RayTracer ${SRCS})

target_link_libraries(RayTracer PRIVATE stb tinyobj glm)
# target_link_libraries(RayTracer PRIVATE imgui glfw stb glad tinyobj glm OpenGL::GL)