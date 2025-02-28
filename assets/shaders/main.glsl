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
const float EPS = 1e-4;
const float INVALID = -1.0;

out vec4 FragColor;

uniform vec2 u_resolution;
uniform vec3 u_camPos;

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct HitPayload {
    float hitDist;
    vec3 worldPos;
    vec3 worldNormal;
};

struct Sphere {
    vec3 albedo;
    vec3 center;
    float radius;
};

struct Scene {
    Sphere sphere;
};

float HitSphere(Sphere sphere, Ray ray) {
    float a = dot(ray.dir, ray.dir);
	float b = 2.0 * dot(ray.origin, ray.dir);
	float c = dot(ray.origin, ray.origin) - sphere.radius * sphere.radius;

	float discriminant = b * b - 4.0f * a * c;
	if (discriminant < 0.0)
		return INVALID;

    float closestT = (-b - sqrt(discriminant)) / (2.0f * a);

    if (closestT < 0.0)
        return INVALID;

    return closestT;
}

HitPayload OnHit(Scene scene, Ray ray, float hitDist) {
    HitPayload payload;

    vec3 origin = ray.origin - scene.sphere.center;
	payload.worldPos = origin + ray.dir * hitDist;
	payload.worldNormal = normalize(payload.worldPos);

	payload.worldPos += scene.sphere.center;

    return payload;
}

HitPayload Miss(Scene scene, Ray ray) {
    HitPayload payload;
    payload.hitDist = INVALID;

    return payload;
}

HitPayload TraceRay(Scene scene, Ray ray) {
    float hitDist = HitSphere(scene.sphere, ray);

    if(hitDist == INVALID)
        return Miss(scene, ray);
    else
        return OnHit(scene, ray, hitDist);
}

vec3 GetColor(Scene scene, Ray camRay) {
    HitPayload payload = TraceRay(scene, camRay);

    vec3 color = vec3(0.0);

    if(payload.hitDist == INVALID)
        return color;

    color = scene.sphere.albedo;

    return color;
}

Scene PrepScene() {
    Scene scene;
    scene.sphere.albedo = vec3(1.0, 0.0, 0.0);
    scene.sphere.center = vec3(0.0, 0.0, 0.0);
    scene.sphere.radius = 0.5;

    return scene;
}

void main() {
    float aspectRatio = u_resolution.x / u_resolution.y;
    vec2 uv = gl_FragCoord.xy / u_resolution;
    uv = uv * 2.0 - 1.0;
    uv.x *= aspectRatio;

    Ray camRay;
    camRay.origin = u_camPos;
    camRay.dir = vec3(uv, -1.0);

    FragColor = vec4(GetColor(PrepScene(), camRay), 1.0);
}