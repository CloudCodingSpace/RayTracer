#include "Texture.h"

#include <glad/glad.h>

void Texture::Init(int32_t width, int32_t height, uint32_t* pixels)
{
    m_Width = width;
    m_Height = height;
    m_Pixels = pixels;

    glGenTextures(1, &m_Handle);
    glBindTexture(GL_TEXTURE_2D, m_Handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_INT, pixels);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Destroy()
{
    glDeleteTextures(1, &m_Handle);
    delete[] m_Pixels;
}

void Texture::Resize(int32_t width, int32_t height)
{
    if(m_Width == width && m_Height == height)
        return;

    m_Width = width;
    m_Height = height;

    Bind();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_INT, m_Pixels);
    
    Unbind();
}

void Texture::Active(int slot)
{
    glActiveTexture(GL_TEXTURE0 + (slot - 1));
}

void Texture::Bind()
{
    glBindTexture(GL_TEXTURE_2D, m_Handle);
}

void Texture::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::SetPixels(uint32_t* pixels)
{
    delete[] m_Pixels;   
    
    m_Pixels = pixels;
    
    Bind();
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_INT, m_Pixels);

    Unbind();
}