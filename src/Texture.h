#pragma once

#include <cstdint>

class Texture
{
public:
    void Init(int32_t width, int32_t height, unsigned char* pixels = nullptr);
    void Destroy();

    void Resize(int32_t width, int32_t height);

    void Active(int slot);
    void Bind();
    void Unbind();

    void GetPixels(unsigned char* pixels);

    void SetPixels(unsigned char* pixels);

    inline uint32_t GetHandle() const { return m_Handle; }
    inline int32_t GetWidth() const { return m_Width; }
    inline int32_t GetHeight() const { return m_Height; }
    inline const unsigned char* GetPixels() const { return m_Pixels; }
private:
    int32_t m_Width = 0, m_Height = 0;
    uint32_t m_Handle = 0;
    unsigned char* m_Pixels = nullptr;
};