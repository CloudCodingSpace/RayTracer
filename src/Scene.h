#pragma once

#include <vector>

#include <glm/glm.hpp>

struct Sphere
{
    glm::vec3 albedo = glm::vec3(1.0f);
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.5f;
};

struct Scene
{
    std::vector<Sphere> spheres;
};