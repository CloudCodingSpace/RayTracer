#pragma once

#include <vector>

#include <glm/glm.hpp>

#include <cstdalign>
#include <string>
#include <fstream>

struct alignas(16) Material 
{
    glm::vec3 albedo = glm::vec3(1.0f);
    float roughness = 0.01f;
};

struct alignas(16) Sphere
{
    glm::vec3 center = glm::vec3(0.0f);
    int materialIdx = 0.0f;
    float radius = 0.5f;
};

struct Scene
{
    std::vector<Sphere> spheres;
    std::vector<Material> materials;

    static void Serialize(Scene& scene, std::string path);
    static void Deserialize(Scene& scene, std::string path);
    static std::ifstream s_In;
    static std::ofstream s_Out;
};
