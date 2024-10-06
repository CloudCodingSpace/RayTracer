#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "Ray.h"

struct Sphere
{
    float radius;
    glm::vec3 center;
    glm::vec3 albedo;

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

struct Scene
{
    std::vector<Sphere> spheres;

    inline bool IsEmpty() {
        return spheres.empty();
    }
};