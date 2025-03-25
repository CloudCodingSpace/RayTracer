#pragma once

#include "Texture.h"

class Framebuffer
{
public:
    void Init(int32_t width, int32_t height);
    void Destroy();

    bool Resize(int32_t width, int32_t height);

    void Bind();
    void Unbind();

    inline Texture GetTexture() const { return m_Texture; }
    inline uint32_t GetHandle() const { return m_Handle; }
private:
    uint32_t m_Handle = 0;
    Texture m_Texture;
};