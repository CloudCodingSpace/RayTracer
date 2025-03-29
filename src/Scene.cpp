#include "Scene.h"

std::ofstream Scene::s_Out;

void Scene::Serialize(Scene& scene, std::string path)
{
    s_Out.open(path);

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

void Scene::Deserialize(Scene& scene, std::string path)
{
    s_In.open(path);

    s_In.close();
}
