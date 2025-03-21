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

uniform float u_SkyboxExposure;
uniform sampler2D t_Skybox;

uniform vec2 u_resolution;
uniform vec3 u_camPos;
uniform vec3 u_camFront;

uniform vec3 u_SphereCenter;
uniform vec3 u_SphereAlbedo;
uniform float u_SphereRadius;
uniform vec3 u_LightDir;
uniform float u_Metallic;

vec2 GetSkyboxTexCoord(vec3 rayDir) {
    float theta = atan(rayDir.x, rayDir.z); 
    float phi = asin(rayDir.y);

    vec2 texCoord;
    texCoord.x = (theta / (PI * 2.0)) + 0.5; 
    texCoord.y = (phi / PI) + 0.5;

    return texCoord;
}

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
    {
        color = texture(t_Skybox, GetSkyboxTexCoord(camRay.dir)).rgb * u_SkyboxExposure;
        return color;
    }

 	// float lightIntensity = max(dot(payload.worldNormal, -u_LightDir), 0.0f);
    // color = scene.sphere.albedo * lightIntensity;

    color = texture(t_Skybox, GetSkyboxTexCoord(reflect(camRay.dir, payload.worldNormal))).rgb * u_Metallic * u_SphereAlbedo;

    return color;
}

Scene PrepScene() {
    Scene scene;
    scene.sphere.albedo = u_SphereAlbedo;
    scene.sphere.center = u_SphereCenter;
    scene.sphere.radius = u_SphereRadius;

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