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
const uint UINT_MAX = 4294967295U;

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

struct Material
{
    vec3 albedo; 
    float roughness;
};

struct Sphere {
    vec3 center;
    int matIdx;
    float radius;
};

struct Scene {
    int sphereIdx;
    vec3 lightPos;
    vec3 lightColor;
    uint rndmSeed;
};

uniform float u_SkyboxExposure;
uniform sampler2D t_Skybox;

uniform vec2 u_resolution;
uniform vec3 u_camPos;
uniform vec3 u_camFront;

uniform vec3 u_LightPos;
uniform vec3 u_LightColor;

uniform int u_MaxBounces;
uniform uint u_RndmSeed;
uniform int u_FrameIdx;
uniform int u_Accumulate;
uniform int u_CamActive;

uniform sampler2D t_PrevFrame;

layout(std430, binding = 0) buffer SphereData {
    Sphere spheres[];
};

layout(std430, binding = 1) buffer MaterialData {
    Material materials[];
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
    return float(RandomUint(seed))/float(UINT_MAX);
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
	if (discriminant < EPS)
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

    for(int i = 0; i < spheres.length(); i++) {
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
    vec3 contrib = vec3(1.0f);

    for (int i = 0; i < u_MaxBounces; i++) {
        HitPayload payload = TraceRay(scene, ray);

        if(payload.hitDist == INVALID)
        {
            color += texture(t_Skybox, GetSkyboxTexCoord(ray.dir)).rgb * u_SkyboxExposure;
            color *= contrib;
            break;
        }

        Sphere sphere = spheres[scene.sphereIdx];
        Material mat = materials[sphere.matIdx];

        // float lightIntensity = max(dot(payload.worldNormal, normalize(scene.lightPos - payload.worldPos)), 0.0f);
        // color += contrib * mat.albedo * lightIntensity * scene.lightColor;
        contrib *= mat.albedo * scene.lightColor;
        contrib *= 0.5f;

        ray.origin = payload.worldPos + payload.worldNormal * 0.0001f;
        // ray.dir = reflect(ray.dir, payload.worldNormal + mat.roughness * RandomVec3MinMax(scene.rndmSeed, -0.5f, 0.5f));
        ray.dir = payload.worldNormal + RandomInUnitSphere(scene.rndmSeed);
    }

    return color;
}

Scene PrepScene(uint seed) {
    Scene scene;
    scene.lightPos = u_LightPos;
    scene.lightColor = u_LightColor;
    scene.rndmSeed = seed;

    return scene;
}

void main() {
    float aspectRatio = u_resolution.x / u_resolution.y;
    vec2 uv = gl_FragCoord.xy / u_resolution;
    uv = uv * 2.0 - 1.0;
    uv.x *= aspectRatio;

    uint seed = uint(gl_FragCoord.x + gl_FragCoord.y * u_resolution.x) * u_FrameIdx;
    seed ^= pcg_hash(seed + u_RndmSeed);

    Ray camRay;
    camRay.origin = u_camPos;

    vec3 camRight = normalize(cross(vec3(0, 1, 0), u_camFront)); 
    vec3 camUp = normalize(cross(u_camFront, camRight));
    vec3 rayDir = normalize(u_camFront + camRight * uv.x + camUp * uv.y);
    camRay.dir = rayDir;

    FragColor = vec4(GetColor(PrepScene(seed), camRay), 1.0);

    if((u_Accumulate == 1) && (u_CamActive == 0)) {
        uv = gl_FragCoord.xy / u_resolution; 
        FragColor = (texture(t_PrevFrame, uv) * (u_FrameIdx - 1.0) + FragColor) / u_FrameIdx;
    }
}
