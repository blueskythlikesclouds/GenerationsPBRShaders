#include "VertexBufferHandler.h"

#include <hedgelib/models/hl_hh_model.h>

#define SWAP_U32() \
    { \
        *(uint32_t*)outputData = _byteswap_ulong(*(uint32_t*)data); \
        data += sizeof(uint32_t); \
        outputData += sizeof(uint32_t); \
    }

#define SWAP_U16() \
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
    case 44:
        for (size_t i = 0; i < count; i++)
        {
            SWAP_U32() // Position.X
            SWAP_U32() // Position.Y
            SWAP_U32() // Position.Z
            SWAP_U16() // Normal.X
            SWAP_U16() // Normal.Y
            SWAP_U16() // Normal.Z
            SWAP_U16() // Padding
            SWAP_U16() // Tangent.X
            SWAP_U16() // Tangent.Y
            SWAP_U16() // Tangent.Z
            SWAP_U16() // Tangent.W
            SWAP_U16() // UV0.X
            SWAP_U16() // UV0.Y
            SWAP_U32() // Color
            SWAP_U32() // Bone Indices
            SWAP_U32() // Bone Weights
        }
        break;

    default:
        for (size_t i = 0; i < (count * stride) / 4; i++)
            SWAP_U32()

        break;
    }

    dxpVertexBuffer->Unlock();

    return dxpVertexBuffer;
}

std::unordered_map<HlU32, D3DDECLTYPE> declTypeMap =
{
    {0x2C83A4, D3DDECLTYPE_FLOAT1},
    {0x2C23A5, D3DDECLTYPE_FLOAT2},
    {0x2A23B9, D3DDECLTYPE_FLOAT3},
    {0x1A23A6, D3DDECLTYPE_FLOAT4},
    {0x182886, D3DDECLTYPE_D3DCOLOR},
    {0x1A2286, D3DDECLTYPE_UBYTE4},
    {0x1A2386, D3DDECLTYPE_UBYTE4},
    {0x2C2359, D3DDECLTYPE_SHORT2},
    {0x1A235A, D3DDECLTYPE_SHORT4},
    {0x1A2086, D3DDECLTYPE_UBYTE4N},
    {0x1A2186, D3DDECLTYPE_UBYTE4N},
    {0x2C2159, D3DDECLTYPE_SHORT2N},
    {0x1A215A, D3DDECLTYPE_SHORT4N},
    {0x2C2059, D3DDECLTYPE_USHORT2N},
    {0x1A205A, D3DDECLTYPE_USHORT4N},
    {0x2A2287, D3DDECLTYPE_UDEC3},
    {0x2A2187, D3DDECLTYPE_DEC3N},
    {0x2A2190, D3DDECLTYPE_DEC3N},
    {0x2C235F, D3DDECLTYPE_FLOAT16_2},
    {0x1A2360, D3DDECLTYPE_FLOAT16_4},
};

#undef SWAP_U16
#undef SWAP_U32

#define SWAP_U16() \
    *(uint16_t*)data = _byteswap_ushort(*(uint16_t*)data); data += sizeof(uint16_t);

#define SWAP_U32() \
    *(uint32_t*)data = _byteswap_ulong(*(uint32_t*)data); data += sizeof(uint32_t);

HOOK(void*, __fastcall, CreateSharedVertexBuffer, 0x72E900, uint32_t This)
{
    struct SharedVertexBufferItem
    {
        uint32_t* meshData;
        uint8_t* vertexData;
        uint16_t* indexData;
        HlHHVertexElement* elementData;
    };

    SharedVertexBufferItem* begin = *(SharedVertexBufferItem**)(This + 24);
    SharedVertexBufferItem* end = *(SharedVertexBufferItem**)(This + 28);

    for (auto it = begin; it != end; it++)
    {
        const uint32_t vertexCount = it->meshData[4];
        const uint32_t stride = it->meshData[5];

        HlHHVertexElement* element = it->elementData;

        while (element->format != 0xFFFFFFFF)
        {
            const D3DDECLTYPE declType = declTypeMap[_byteswap_ulong(element->format)];

            for (size_t i = 0; i < vertexCount; i++)
            {
                uint8_t* data = it->vertexData + i * stride + _byteswap_ushort(element->offset);

                switch (declType)
                {
                case D3DDECLTYPE_FLOAT4:
                    SWAP_U32();

                case D3DDECLTYPE_FLOAT3:
                    SWAP_U32();

                case D3DDECLTYPE_FLOAT2:
                    SWAP_U32();

                case D3DDECLTYPE_FLOAT1:
                case D3DDECLTYPE_D3DCOLOR:
                case D3DDECLTYPE_UDEC3:
                case D3DDECLTYPE_DEC3N:
                case D3DDECLTYPE_UBYTE4:
                case D3DDECLTYPE_UBYTE4N:
                    SWAP_U32();
                    break;

                case D3DDECLTYPE_SHORT4:
                case D3DDECLTYPE_SHORT4N:
                case D3DDECLTYPE_USHORT4N:
                case D3DDECLTYPE_FLOAT16_4:
                    SWAP_U16();
                    SWAP_U16();

                case D3DDECLTYPE_SHORT2:
                case D3DDECLTYPE_SHORT2N:
                case D3DDECLTYPE_USHORT2N:
                case D3DDECLTYPE_FLOAT16_2:
                    SWAP_U16();
                    SWAP_U16();
                    break;
                }
            }

            element++;
        }

        // Swap again so it becomes valid in the original code.
        // TODO: Instead of doing this, rewrite the original function.
        for (size_t i = 0; i < vertexCount * stride; i += 4)
            *(uint32_t*)(it->vertexData + i) = _byteswap_ulong(*(uint32_t*)(it->vertexData + i));

    }

    return originalCreateSharedVertexBuffer(This);
}

bool VertexBufferHandler::enabled = false;

void VertexBufferHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CreateVertexBuffer);
    INSTALL_HOOK(CreateSharedVertexBuffer);

    // WRITE_MEMORY(0x725015, uint8_t, 0);
}
