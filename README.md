# RayTracer

This app uses Vulkan and C++ and does raytracing using compute shaders.  
This app is currently going through a rewrite. The earlier version used OpenGL with C++.  The source code and information about that version is in the [v1.0](http://github.com/CloudCodingSpace/RayTracer/tree/v1.0) branch.

# Goals
 - [x] Be able to render any number of spheres
 - [ ] Be able to render triangles
 - [ ] Get photorealistic images as output
 - [ ] Be able to take screenshots
 - [ ] Be able to render HDR panorama as skyboxes
 - [ ] Be able to load separate scene configs as json file format

## Prerequisites

 - CMake
 - Git (for version control)
 - A modern C++ compiler
 - VulkanSDK installed with shader compilers like `glslc`
 - A GPU supporting Vulkan 1.2 in minimum and supporting the scalar block layout feature

## Build instruction

Make sure to clone the repo like this:

```
git clone --recursive http://github.com/CloudCodingSpace/RayTracer.git
```

After that simply use CMake and get the `RayTracer` executable and run it. And there you go with the tracer working!!
