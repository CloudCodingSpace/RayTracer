#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#define FLT_MAX 3.402823466e+38
#define UINT_MAX 4294967295u
#define PI 3.141592653589793

uint g_Seed = 0;

// Taken from the internet, from stackoverflow
uint pcg_hash(uint i) {
    uint state = i * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float RandomFloat() {
    uint r = pcg_hash(g_Seed);
    g_Seed = pcg_hash(g_Seed);

    return float(r) / UINT_MAX;
}

float RandomFloatMinMax(float min, float max) {
    return RandomFloat() * (max - min) + min;
}

vec3 RandomVec3() {
    return vec3(RandomFloat(), RandomFloat(), RandomFloat());
}

vec3 RandomVec3MinMax(float min, float max) {
    return vec3(RandomFloatMinMax(min, max), RandomFloatMinMax(min, max), RandomFloatMinMax(min, max));
}

vec3 RandomInUnitSphere() {
    vec3 p;
    do {
        p = RandomVec3MinMax(-1.0, 1.0);
    } while(dot(p, p) > 1.0);

    return p;
}

vec2 RandomInUnitDisc() {
    float u = RandomFloat();
    float v = RandomFloat();

    float r = sqrt(u);
    float theta = 2 * PI * v;

    float x = r * cos(theta);
    float y = r * sin(theta);

    return vec2(x, y);
}

#endif
