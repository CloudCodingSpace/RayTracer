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


uniform float u_skyboxStrength;
uniform float u_skyboxCeiling;
uniform float u_skyboxGamma;
uniform vec2 u_resolution;
uniform vec3 u_camPos;

uniform sampler2D u_skyboxTexture;

const float EPS = 1e-4;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Sphere {
    vec3 origin;
    float radius;
};

struct HitInfo {
    vec3 hitPoint;
    float t;
    bool didHit;
};

vec3 RayAt(Ray ray, float t) {
    return ray.origin + ray.direction * t;
}

vec3 SampleSkybox(vec3 dir) {
	if (u_skyboxStrength == 0.0) return vec3(0.0);
	
	return min(vec3(u_skyboxCeiling), 
            u_skyboxStrength*
            pow(texture(u_skyboxTexture, 
            vec2(0.5 + atan(dir.x, dir.z)/(2*PI), 
            0.5 + asin(-dir.y)/PI)).xyz, 
            vec3(1.0/u_skyboxGamma)));
}

HitInfo RayHitSphere(Ray ray, Sphere sphere) {
    HitInfo info;
    
    vec3 o = ray.origin - sphere.origin;
    float a = dot(ray.direction, ray.direction);
    float b = 2.0f * dot(o, ray.direction);
    float c = dot(o, o) - sphere.radius * sphere.radius;

    float disc = b * b - 4.0 * a * c;
    if(disc < 0.0) {
        info.didHit = false;
    } else {
        info.didHit = true;
        info.t = (-b - sqrt(disc)) / (2.0 * a);

        if(info.t < 0.0) {
            info.didHit = false;
        } else {
            info.hitPoint = RayAt(ray, info.t);
        }
    }

    return info;
}

vec3 GetColor(Ray ray);

void main() {
    float aspectRatio = u_resolution.x / u_resolution.y;
    vec2 uv = gl_FragCoord.xy / u_resolution;
    uv = uv * 2.0 - 1.0;
    uv.x *= aspectRatio;

    Ray camRay;
    camRay.origin = u_camPos;

    camRay.direction = vec3(uv, -1.0f);

    vec3 color = GetColor(camRay);
    FragColor = vec4(color, 1.0);
}

vec3 GetColor(Ray ray) {
    vec3 finalColor = vec3(-1);

    Sphere sphere;
    sphere.origin = vec3(0.0, 0.0, -2.0);
    sphere.radius = 0.5;

    HitInfo info = RayHitSphere(ray, sphere);
    if(info.didHit) {
        vec3 normal = normalize(info.hitPoint - sphere.origin);
        finalColor = normal;
    }

    if(finalColor == vec3(-1)) {
        if(u_skyboxStrength == 0.0) {
            float x = 0.5 * (normalize(ray.direction).y + 1.0);
            finalColor = (1.0f - x) * vec3(1.0f, 1.0f, 1.0f) + x * vec3(0.0f, 0.7f, 1.0f);
        }
        else {
            finalColor = SampleSkybox(ray.direction);
        }
    }

    return finalColor;
}
