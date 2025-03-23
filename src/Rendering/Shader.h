#pragma once

#include <string>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
public:
    void Init(std::string path);
    void Destroy();

    void Bind();
    void Unbind();

    void PutTex(std::string name, int texIndex);
    void PutVec2(std::string name, glm::vec2 data);
    void PutVec3(std::string name, glm::vec3 data);
    void PutVec4(std::string name, glm::vec4 data);
    void PutIVec2(std::string name, glm::ivec2 data);
    void PutIVec3(std::string name, glm::ivec3 data);
    void PutIVec4(std::string name, glm::ivec4 data);
    void PutDVec2(std::string name, glm::dvec2 data);
    void PutDVec3(std::string name, glm::dvec3 data);
    void PutDVec4(std::string name, glm::dvec4 data);
    void PutMat3(std::string name, glm::mat3 data);
    void PutMat4(std::string name, glm::mat4 data);
    void PutInt(std::string name, int data);
    void PutUint(std::string name, uint32_t data);
    void PutFloat(std::string name, float data);
private:
    void ReadFile(std::string path); 

private:
    std::string vSrc, fSrc;
    uint32_t m_vID, m_fID, m_pID;
};
