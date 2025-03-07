#include "Framebuffer.h"

#include <glad/glad.h>

#include "Utils/Logger.h"

void Framebuffer::Init(int32_t width, int32_t height)
{
    glGenFramebuffers(1, &m_Handle);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);

    m_Texture.Init(width, height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_Texture.GetHandle(), 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        FATAL("Failed to create the framebuffer. Status :- " + std::to_string(glCheckFramebufferStatus(GL_FRAMEBUFFER)));
        std::exit(-1);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Destroy()
{
    m_Texture.Destroy();
    glDeleteFramebuffers(1, &m_Handle);   
}

void Framebuffer::Resize(int32_t width, int32_t height)
{
    if(m_Texture.GetWidth() == width && m_Texture.GetHeight() == height)
        return;

    Destroy();

    Init(width, height);
}

void Framebuffer::Bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);
    m_Texture.Bind();
}

void Framebuffer::Unbind()
{
    m_Texture.Unbind();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}