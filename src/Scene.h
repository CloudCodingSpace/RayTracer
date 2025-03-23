#pragma once

#include <vector>

#include <glm/glm.hpp>

#include <cstdalign>

struct alignas(16) Sphere
{
    glm::vec3 albedo = glm::vec3(1.0f);
    float radius = 0.5f;
    glm::vec3 center = glm::vec3(0.0f);
    float roughness = 0.0f;
};

struct Scene
{
    std::vector<Sphere> spheres;
};