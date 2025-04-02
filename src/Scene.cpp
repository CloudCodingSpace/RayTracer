#include "Scene.h"

std::ofstream Scene::s_Out;
std::ifstream Scene::s_In;

void Scene::Serialize(Scene& scene, std::filesystem::path path)
{
    s_Out.open(path);

    s_Out << scene.spheres.size() << "\n";
    s_Out << scene.materials.size() << "\n";

    for(auto& sphere : scene.spheres)
    {
        s_Out << sphere.center.x << " " << sphere.center.y << " " << sphere.center.z << "\n";
        s_Out << sphere.radius << "\n";
        s_Out << sphere.materialIdx << "\n";
    }

    for(auto& mat : scene.materials)
    {
        s_Out << mat.albedo.x << " " << mat.albedo.y << " " << mat.albedo.z << "\n";
        s_Out << mat.roughness << "\n";
    }

    s_Out.close();
}

void Scene::Deserialize(Scene& scene, std::filesystem::path path)
{
    s_In.open(path);

    if (!s_In.good() || !s_In.is_open())
    {
        scene.spheres.push_back(Sphere{});
        scene.materials.push_back(Material{});
        s_In.close();
        return;
    }

    size_t sphereCount, matCount;

    s_In >> sphereCount;
    s_In >> matCount;

    scene.spheres.resize(sphereCount);
    scene.materials.resize(matCount);

    for (auto& sphere : scene.spheres)
    {
        s_In >> sphere.center.x >> sphere.center.y >> sphere.center.z;
        s_In >> sphere.radius;
        s_In >> sphere.materialIdx;
    }

    for (auto& mat : scene.materials)
    {
        s_In >> mat.albedo.x >> mat.albedo.y >> mat.albedo.z;
        s_In >> mat.roughness;
    }

    s_In.close();
}
