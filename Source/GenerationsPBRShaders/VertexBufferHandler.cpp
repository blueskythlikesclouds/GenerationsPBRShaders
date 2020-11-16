#include "VertexBufferHandler.h"

#define WRITE32() \
    { \
        *(uint32_t*)outputData = _byteswap_ulong(*(uint32_t*)data); \
        data += sizeof(uint32_t); \
        outputData += sizeof(uint32_t); \
    }

#define WRITE16() \
    { \
        *(uint16_t*)outputData = _byteswap_ushort(*(uint16_t*)data); \
        data += sizeof(uint16_t); \
        outputData += sizeof(uint16_t); \
    }

HOOK(DX_PATCH::IDirect3DVertexBuffer9*, __cdecl, CreateVertexBuffer, 0x7479A0,
    DX_PATCH::IDirect3DDevice9* dxpDevice, const uint8_t* data, uint32_t stride, uint32_t count)
{
    DX_PATCH::IDirect3DVertexBuffer9* dxpVertexBuffer;
    dxpDevice->CreateVertexBuffer(stride * count, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &dxpVertexBuffer, nullptr);

    uint8_t* outputData;
    dxpVertexBuffer->Lock(0, stride * count, (void**)&outputData, 0);

    // TODO: Use vertex format itself to do the swapping.
    switch (stride)
    {
    case 36:
        for (size_t i = 0; i < count; i++)
        {
            WRITE32() // Position.X
            WRITE32() // Position.Y
            WRITE32() // Position.Z
            WRITE32() // Normal
            WRITE32() // Tangent
            WRITE32() // Binormal
            WRITE16() // UV0.X
            WRITE16() // UV0.Y
            WRITE16() // UV1.X
            WRITE16() // UV1.Y
            WRITE32() // Color
        }

        break;

    case 40:
        for (size_t i = 0; i < count; i++)
        {
            WRITE32() // Position.X
            WRITE32() // Position.Y
            WRITE32() // Position.Z
            WRITE32() // Normal
            WRITE32() // Tangent
            WRITE32() // Binormal
            WRITE16() // UV0.X
            WRITE16() // UV0.Y
            WRITE32() // Color
            WRITE32() // Bone indices
            WRITE32() // Bone weights
        }
        break;

    case 52:
        for (size_t i = 0; i < count; i++)
        {
            WRITE32() // Position.X
            WRITE32() // Position.Y
            WRITE32() // Position.Z
            WRITE32() // Normal
            WRITE32() // Tangent
            WRITE32() // Binormal
            WRITE16() // UV0.X
            WRITE16() // UV0.Y
            WRITE16() // UV1.X
            WRITE16() // UV1.Y
            WRITE16() // UV2.X
            WRITE16() // UV2.Y
            WRITE16() // UV3.X
            WRITE16() // UV3.Y
            WRITE32() // Color
            WRITE32() // Bone indices
            WRITE32() // Bone weights
        }
        break;

    default:
        for (size_t i = 0; i < (count * stride) / 4; i++)
            WRITE32()

        break;
    }

    dxpVertexBuffer->Unlock();

    return dxpVertexBuffer;
}

bool VertexBufferHandler::enabled = false;

void VertexBufferHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CreateVertexBuffer);

    WRITE_MEMORY(0x13E93D8, uint32_t, 56); // Enable W in DEC3N
}
