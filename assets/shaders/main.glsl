@type vertex
#version 450 core

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}

@type fragment
#version 450 core

const float PI = 3.141592653589793;
const float EPS = 1e-4;
const float INVALID = -1.0;

out vec4 FragColor;

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
    float radius;
    vec3 center;
    float roughness;
};

struct Scene {
    int sphereIdx;
    vec3 lightDir;
    uint rndmSeed;
};

uniform float u_SkyboxExposure;
uniform sampler2D t_Skybox;

uniform vec2 u_resolution;
uniform vec3 u_camPos;
uniform vec3 u_camFront;

uniform vec3 u_LightDir;

uniform int u_SphereCount;
uniform int u_MaxBounces;
uniform uint u_RndmSeed;
uniform float u_FltMax;

layout(std430, binding = 0) buffer SphereData {
    Sphere spheres[];
};

uint pcg_hash(uint seed)
{
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint RandomUint(inout uint seed)
{
    seed = pcg_hash(seed);
    return pcg_hash(seed);
}

float RandomFloat(inout uint seed)
{
    return float(RandomUint(seed))/float(u_FltMax);
}

vec3 RandomVec3(inout uint seed)
{
    return vec3(RandomFloat(seed), RandomFloat(seed), RandomFloat(seed));
}

vec3 RandomVec3MinMax(inout uint seed, float min, float max)
{
    return vec3(RandomFloat(seed) * (max - min) + min, RandomFloat(seed) * (max - min) + min, RandomFloat(seed) * (max - min) + min);
}

vec3 RandomInUnitSphere(inout uint seed)
{
    return normalize(RandomVec3MinMax(seed, -1.0f, 1.0f));
}

vec2 GetSkyboxTexCoord(vec3 rayDir) {
    float theta = atan(rayDir.x, rayDir.z); 
    float phi = asin(rayDir.y);

    vec2 texCoord;
    texCoord.x = (theta / (PI * 2.0)) + 0.5; 
    texCoord.y = (phi / PI) + 0.5;

    return texCoord;
}

float HitSphere(Sphere sphere, Ray ray) {
    vec3 origin = ray.origin - sphere.center;
    float a = dot(ray.dir, ray.dir);
	float b = 2.0 * dot(origin, ray.dir);
	float c = dot(origin, origin) - sphere.radius * sphere.radius;

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

    Sphere sphere = spheres[scene.sphereIdx];

    vec3 origin = ray.origin - sphere.center;
	payload.worldPos = origin + ray.dir * hitDist;
	payload.worldNormal = normalize(payload.worldPos);

	payload.worldPos += sphere.center;

    return payload;
}

HitPayload Miss(Scene scene, Ray ray) {
    HitPayload payload;
    payload.hitDist = INVALID;

    return payload;
}

HitPayload TraceRay(inout Scene scene, Ray ray) {
    float closestHitDist = 1e10;
    int sphereIdx;

    for(int i = 0; i < u_SphereCount; i++) {
        float hitDist = HitSphere(spheres[i], ray);

        if(hitDist == INVALID)
            continue;

        if(hitDist < closestHitDist) {
            closestHitDist = hitDist;
            sphereIdx = i;
        }
    }

    scene.sphereIdx = sphereIdx;

    if(closestHitDist == 1e10)
        return Miss(scene, ray);
    else
        return OnHit(scene, ray, closestHitDist);
}

vec3 GetColor(Scene scene, Ray ray) {
    vec3 color = vec3(0.0f);
    float contrib = 1.0f;

    for (int i = 0; i < u_MaxBounces; i++) {
        HitPayload payload = TraceRay(scene, ray);

        if(payload.hitDist == INVALID)
        {
            color += texture(t_Skybox, GetSkyboxTexCoord(ray.dir)).rgb * u_SkyboxExposure;
            color *= contrib;
            break;
        }

        Sphere sphere = spheres[scene.sphereIdx];

        float lightIntensity = max(dot(payload.worldNormal, -scene.lightDir), 0.0f);
        color += sphere.albedo * lightIntensity;
        color *= contrib;
        contrib *= 0.5f;

        ray.origin = payload.worldPos + payload.worldNormal * 0.0001f;
        ray.dir = reflect(ray.dir, payload.worldNormal);
    }

    return color;
}

Scene PrepScene() {
    Scene scene;
    scene.lightDir = u_LightDir;
    scene.rndmSeed = u_RndmSeed;

    return scene;
}

void main() {
    float aspectRatio = u_resolution.x / u_resolution.y;
    vec2 uv = gl_FragCoord.xy / u_resolution;
    uv = uv * 2.0 - 1.0;
    uv.x *= aspectRatio;

    Ray camRay;
    camRay.origin = u_camPos;

    vec3 camRight = normalize(cross(vec3(0, 1, 0), u_camFront)); 
    vec3 camUp = normalize(cross(u_camFront, camRight));
    vec3 rayDir = normalize(u_camFront + camRight * uv.x + camUp * uv.y);
    camRay.dir = rayDir;

    FragColor = vec4(GetColor(PrepScene(), camRay), 1.0);
}