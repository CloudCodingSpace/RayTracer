#ifndef STRUCTS_GLSL
#define STRUCTS_GLSL

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Sphere {
    vec3 center;
    float radius;
};

struct HitInfo {
    vec3 normal;
    vec3 worldPos;
    float t;
};

bool DidRayHitSphere(Ray ray, Sphere sphere, out HitInfo info) {
    float a = dot(ray.dir, ray.dir);
    float b = 2 * dot(ray.dir, ray.origin - sphere.center);
    float c = dot(ray.origin - sphere.center, ray.origin - sphere.center) - sphere.radius*sphere.radius;

    float D = b*b - 4*a*c;
    if(D < 0.0)
        return false;
    float D2 = sqrt(D);
    float t0 = (-b - D2)/(2*a);
    float t1 = (-b + D2)/(2*a);

    float t = t0;
    if(t < 0)
        t = t1;
    if(t < 0)
        return false;
    
    info.t = t;
    info.worldPos = ray.origin + t * ray.dir;
    info.normal = normalize(info.worldPos - sphere.center);

    return true;
}

#endif
