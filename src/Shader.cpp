#include "Shader.h"

#include <fstream>
#include <iostream>
#include <map>

#include <Utils/Utils.h>
#include <Utils/Logger.h>

#include <glad/glad.h>

void Shader::Init(std::string path)
{
    ReadFile(path);
    const char* vs = vSrc.c_str();
    const char* fs = fSrc.c_str();

    char log[1024];
    int success = 0;

    m_vID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(m_vID, 1, &vs, 0);
    glCompileShader(m_vID);
    glGetShaderiv(m_vID, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(m_vID, 1024, 0, log);
        ERROR((const char*)log)
    }
    
    m_fID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(m_fID, 1, &fs, 0);
    glCompileShader(m_fID);
    glGetShaderiv(m_fID, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(m_fID, 1024, 0, log);
        ERROR((const char*)log)
    }

    m_pID = glCreateProgram();
    glAttachShader(m_pID, m_vID);
    glAttachShader(m_pID, m_fID);
    glLinkProgram(m_pID);
    glValidateProgram(m_pID);
    glGetProgramiv(m_pID, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(m_pID, 1024, 0, log);
        ERROR((const char*)log)
    }
    glGetProgramiv(m_pID, GL_VALIDATE_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(m_pID, 1024, 0, log);
        ERROR((const char*)log)
    }

    glDeleteShader(m_vID);
    glDeleteShader(m_fID);
}

void Shader::Destroy()
{
    glDeleteProgram(m_pID);
}

void Shader::ReadFile(std::string path)
{
    std::ifstream in(path);
    if(!in)
    {
        std::cerr << "Can't open the shader file!" << std::endl;
        std::exit(-1);
    }
    
    std::string line = "";

    bool v = false, f = false, t = false;
    while(getline(in, line))
    {
        line += "\n";

        std::string l = Utils::toLowerCase(line);
        if (l.find("type vert") != std::string::npos) {
            f = false;
            v = true;
            t = true;
        } else if (l.find("type frag") != std::string::npos) {
            t = true;
            f = true;
            v = false;
        } else {
            t = false;
        }

        if (v && !t) {
            vSrc += line;
        }
        if (f && !t) {
            fSrc += line;
        }

        line = "";
    }

    in.close();
}

void Shader::Bind()
{
    glUseProgram(m_pID);
}

void Shader::Unbind()
{
    glUseProgram(0);
}

void Shader::PutTex(std::string name, int texIndex)
{
    Bind();
    glActiveTexture(GL_TEXTURE0 + texIndex);
    glUniform1i(glGetUniformLocation(m_pID, name.c_str()), texIndex);
}

void Shader::PutVec2(std::string name, glm::vec2 data)
{
    Bind();
    glUniform2f(glGetUniformLocation(m_pID, name.c_str()), data.x, data.y);
}

void Shader::PutVec3(std::string name, glm::vec3 data)
{
    Bind();
    glUniform3f(glGetUniformLocation(m_pID, name.c_str()), data.x, data.y, data.z);
}

void Shader::PutVec4(std::string name, glm::vec4 data)
{
    Bind();
    glUniform4f(glGetUniformLocation(m_pID, name.c_str()), data.x, data.y, data.z, data.w);
}

void Shader::PutIVec2(std::string name, glm::ivec2 data)
{
    Bind();
    glUniform2i(glGetUniformLocation(m_pID, name.c_str()), data.x, data.y);
}

void Shader::PutIVec3(std::string name, glm::ivec3 data)
{
    Bind();
    glUniform3i(glGetUniformLocation(m_pID, name.c_str()), data.x, data.y, data.z);
}

void Shader::PutIVec4(std::string name, glm::ivec4 data)
{
    Bind();
    glUniform4i(glGetUniformLocation(m_pID, name.c_str()), data.x, data.y, data.z, data.w);
}

void Shader::PutDVec2(std::string name, glm::dvec2 data)
{
    Bind();
    glUniform2d(glGetUniformLocation(m_pID, name.c_str()), data.x, data.y);
}

void Shader::PutDVec3(std::string name, glm::dvec3 data)
{
    Bind();
    glUniform3d(glGetUniformLocation(m_pID, name.c_str()), data.x, data.y, data.z);
}

void Shader::PutDVec4(std::string name, glm::dvec4 data)
{
    Bind();
    glUniform4d(glGetUniformLocation(m_pID, name.c_str()), data.x, data.y, data.z, data.w);
}

void Shader::PutMat3(std::string name, glm::mat3 data)
{
    Bind();
    glUniformMatrix3fv(glGetUniformLocation(m_pID, name.c_str()), 1, false, glm::value_ptr(data));
}

void Shader::PutMat4(std::string name, glm::mat4 data)
{
    Bind();
    glUniformMatrix4fv(glGetUniformLocation(m_pID, name.c_str()), 1, false, glm::value_ptr(data));
}

void Shader::PutInt(std::string name, int data)
{
    Bind();
    glUniform1i(glGetUniformLocation(m_pID, name.c_str()), data);
}

void Shader::PutFloat(std::string name, float data)
{
    Bind();
    glUniform1f(glGetUniformLocation(m_pID, name.c_str()), data);    
}