#include "Texture.h"

#include <glad/glad.h>

void Texture::Init(int32_t width, int32_t height, void* pixels, bool isFloat)
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

    glTexImage2D(GL_TEXTURE_2D, 0, (isFloat) ? GL_RGBA32F : GL_RGBA, width, height, 0, GL_RGBA, (isFloat) ? GL_FLOAT : GL_UNSIGNED_BYTE, pixels);

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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_Pixels);
    
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

void Texture::GetPixels(unsigned char* pixels)
{
    if(!pixels)
        return;

    Bind();
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    Unbind();
}

void Texture::SetPixels(unsigned char* pixels)
{
    delete[] m_Pixels;   
    
    m_Pixels = pixels;
    
    Bind();
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_Pixels);

    Unbind();
}