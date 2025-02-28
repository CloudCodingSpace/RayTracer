@type vertex
#version 330 core

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}

@type fragment
#version 330 core

const float PI = 3.141592653589793;

out vec4 FragColor;

uniform vec2 u_resolution;
uniform vec3 u_camPos;

const float EPS = 1e-4;

void main() {
    float aspectRatio = u_resolution.x / u_resolution.y;
    vec2 uv = gl_FragCoord.xy / u_resolution;
    uv = uv * 2.0 - 1.0;
    uv.x *= aspectRatio;

    FragColor = vec4(uv, 0.0, 1.0);
}