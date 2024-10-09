# RayTracer
RayTracer, this project is about me (@CloudCodingSpace), creating a software raytracer pretty much from scratch with very less number of dependencies.
The prefferred language of choice is indeed C++.

## Build
First you need to clone this repository using this command on your terminal or powershell :- 
    **git clone --recursive https://github.com/CloudCodingSpace/RayTracer**

In order to build this project, it is necessary to install CMake on your machine.
Then simply if you are **on Windows** then for simplicity, I have created a **build.bat** file at the project root which
anyone on Windows can simply double-click or run the file, then the project would be already built.
If you are on Linux/MacOS then simply type these commands on your terminal :- 
    **mkdir bin**
    **cd bin**
    **cmake ..**
    **cmake --build . --parallel**
When you are finally done with **building** the project, then find your **RayTracer** executable and run it.
When you run the executable, then it will print the time (in milleseconds) took for generating the pixel's of the image.
You will also see that it creates a directory named **output** and inside it will be the **img.png**. That is our juicy output of this project.

## Dependencies
This project uses a couple of dependencies. Namely :- 
- GLM (OpenGL Mathematics Library. But it also provides a maths template for some vital linear algebra related code)
- STB image and STB write (For writing and reading image files)
- TinyOBJLoader (For loading meshes with obj file format. Not used because this project at the time being doesn't support loading meshes)
- ImGUI (For GUI for OpenGL and GLFW. Not used because this project at the time being doesn't support real-time raytracing)
- GLFW (For creating windows. Not used because this project at the time being doesn't support real-time raytracing)
- GLAD (For loading OpenGL functions. Not used because this project at the time being doesn't support real-time raytracing)

## Features
The features this raytracer supports are :-
- Generate images with PNG format
- Can render multiple spheres
- Can render spheres but with materials on it. (Only diffuse at the time being)
- Can do lighting on spheres but with many properties like light intensity, colour, and its origin
- Has a scene based project structure
- Can do phong lighting if the option is enabled