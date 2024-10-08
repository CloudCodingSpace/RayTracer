#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "Ray.h"

enum MaterialType
{
    MATERIAL_TYPE_DIFFUSE
};

struct Material
{
    glm::vec3 albedo;
    float ambient;
    MaterialType type;
};

struct Sphere
{
    float radius;
    glm::vec3 center;
    Material mat;

    float Hit(Ray& ray) {
        glm::vec3 o = ray.GetOrigin() - center;

        float a = glm::dot(ray.GetDir(), ray.GetDir());
        float b = 2.0f * glm::dot(o, ray.GetDir());
        float c = glm::dot(o, o) - radius * radius;

        float disc = b * b - 4.0f * a * c;

        if(disc < 0.0f) {
            return -1.0f;
        } else {
            float t = (-b - glm::sqrt(disc)) / (2.0f * a);
            return t;
        }
    }
};

struct LightSource
{
    glm::vec3 origin;
    glm::vec3 color;
    float intensity;
};

struct Scene
{
    std::vector<Sphere> spheres;
    std::vector<LightSource> lightSources;
};