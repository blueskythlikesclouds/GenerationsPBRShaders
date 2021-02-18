#include "SGGIHandler.h"

#define READ(type, name) \
    const type name = *(type*)data; \
    data += sizeof(type);

#define READ_STRING(name) \
    uint8_t name##Length = *(uint8_t*)data; \
    const char* name = (const char*)(data + 1); \
    data += name##Length + 1;

struct SubTextureName
{
    const char* value;
    size_t length;
};

struct SubTexture
{
    const char* textureName;
    size_t textureNameLength;
    float width;
    float height;
    float x;
    float y;
};

FUNCTION_PTR(SubTexture*, __thiscall, createSubTexture, 0x72D980, void* subTextureMap, const SubTextureName& name);

HOOK(uint32_t, __cdecl, ReadAtlasinfo, 0x7299E0, uint8_t* data, void* subTextureMap)
{
    READ(uint8_t, header);

    if (header)
        return header;

    READ(uint16_t, textureCount);

    for (size_t i = 0; i < textureCount; i++)
    {
        READ_STRING(textureName);
        READ(uint16_t, subTextureCount);

        for (size_t j = 0; j < subTextureCount; j++)
        {
            READ_STRING(subTextureName);
            READ(uint8_t, width);
            READ(uint8_t, height);
            READ(uint8_t, x);
            READ(uint8_t, y);

            SubTexture* subTexture = createSubTexture(subTextureMap, { subTextureName, subTextureNameLength });

            subTexture->textureName = textureName;
            subTexture->textureNameLength = textureNameLength;
            subTexture->width = 1.0f / (1 << width);
            subTexture->height = 1.0f / (1 << height);
            subTexture->x = x / 256.0f;
            subTexture->y = y / 256.0f;

            // HACK: Make the width negative to tell our hook that this is a SGGI texture.
            if (width & 0x80)
                subTexture->width = -1 / (float)(1 << (width & 0x7F));
        }
    }

    return header;
}

HOOK(uint32_t, __fastcall, SetGIAtlasParam, 0x6FA080, Hedgehog::Mirage::CRenderingDevice* This, void* Edx, float* value)
{
    if (value == (float*)0x13DEAB0) // Fail-safe for non-existent GI textures
        return originalSetGIAtlasParam(This, Edx, value);

    float giAtlasParam[] = { abs(value[0]), value[1], value[2], value[3] };
    float sggiAtlasParam[] = { giAtlasParam[0] * 0.5f, giAtlasParam[1] * 0.5f, value[2], value[3] };
    BOOL isSggiEnabled[] = { value[0] < 0.0f };

    DX_PATCH::IDirect3DTexture9* dxpTexture = *(DX_PATCH::IDirect3DTexture9**)((uint32_t)value + 16);

    D3DSURFACE_DESC desc;
    dxpTexture->GetLevelDesc(0, &desc);

    float giAtlasSize[] = 
        { (float)desc.Width, (float)desc.Height, 1.0f / (float)desc.Width, 1.0f / (float)desc.Height };

    This->m_pD3DDevice->SetVertexShaderConstantF(186, giAtlasParam, 1);

    This->m_pD3DDevice->SetPixelShaderConstantF(109, giAtlasParam, 1);
    This->m_pD3DDevice->SetPixelShaderConstantF(110, sggiAtlasParam, 1);
    This->m_pD3DDevice->SetPixelShaderConstantF(111, giAtlasSize, 1);

    This->m_pD3DDevice->SetPixelShaderConstantB(9, isSggiEnabled, 1);

    return 1;
}

bool SGGIHandler::enabled = false;

void SGGIHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(ReadAtlasinfo);
    INSTALL_HOOK(SetGIAtlasParam);
}
