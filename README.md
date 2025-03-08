# RayTracer
RayTracer, this project is about me (@CloudCodingSpace), creating a real-time raytracer pretty much from scratch with very less number of dependencies.
The prefferred language of choice is indeed C++. And the graphics API is OpenGL.

## Build
First you need to clone this repository using this command on your terminal or powershell :- 
    

    git clone --recursive https://github.com/CloudCodingSpace/RayTracer



Then simply run these commands on your terminal :-  
    

    mkdir bin
    cd bin  
    cmake ..  
    cmake --build . --parallel


When you are finally done with **building** the project, then find your executable and run it.
When you run the executable, then it will open a window and render the scene in real-time.

## Dependencies
This project uses a couple of dependencies. Namely :- 
- GLM
- TinyOBJLoader
- ImGUI
- GLFW
- GLAD
