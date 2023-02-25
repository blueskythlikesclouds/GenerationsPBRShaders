#include "VertexBufferHandler.h"

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
    DX_PATCH::IDirect3DVertexBuffer9* dxpVertexBuffer = nullptr;
    dxpDevice->CreateVertexBuffer(stride * count, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &dxpVertexBuffer, nullptr);

    uint8_t* outputData = nullptr;
    dxpVertexBuffer->Lock(0, 0, (void**)&outputData, 0);

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

std::unordered_map<hl::u32, D3DDECLTYPE> declTypeMap =
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
        hl::hh::mirage::raw_vertex_element* elementData;
    };

    SharedVertexBufferItem* begin = *(SharedVertexBufferItem**)(This + 24);
    SharedVertexBufferItem* end = *(SharedVertexBufferItem**)(This + 28);

    for (auto it = begin; it != end; it++)
    {
        const uint32_t vertexCount = it->meshData[4];
        const uint32_t stride = it->meshData[5];

        bool swapStates[0x100]{};

        hl::hh::mirage::raw_vertex_element* element = it->elementData;

        while (_byteswap_ushort(element->stream) != 0xFF)
        {
            const hl::u16 offset = _byteswap_ushort(element->offset);

            if (swapStates[offset])
            {
                element++;
                continue;
            }

            swapStates[offset] = true;

            const D3DDECLTYPE declType = declTypeMap[_byteswap_ulong((hl::u32)element->format)];

            for (size_t i = 0; i < vertexCount; i++)
            {
                uint8_t* data = it->vertexData + i * stride + offset;

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
    }

    return originalCreateSharedVertexBuffer(This);
}

HOOK(bool, __cdecl, CreateVertexElements, 0x7448F0, void* a1, D3DVERTEXELEMENT9* d3dElements, uint32_t count, hl::hh::mirage::raw_vertex_element* hlElements)
{
    D3DVERTEXELEMENT9* minTexCoordElement = nullptr;
    int32_t maxTexCoordUsageIndex = INT_MIN;

    size_t i;

    for (i = 0; i < count - 1; i++)
    {
        hl::hh::mirage::raw_vertex_element* hlElement = &hlElements[i];

        if (_byteswap_ushort(hlElement->stream) == 0xFF)
            break;

        D3DVERTEXELEMENT9* d3dElement = &d3dElements[i];

        d3dElement->Stream = _byteswap_ushort(hlElement->stream);
        d3dElement->Offset = _byteswap_ushort(hlElement->offset);
        d3dElement->Type = declTypeMap[_byteswap_ulong((hl::u32)hlElement->format)];
        d3dElement->Method = (BYTE)hlElement->method;
        d3dElement->Usage = (BYTE)hlElement->type;
        d3dElement->UsageIndex = hlElement->index;

        if (d3dElement->Usage != D3DDECLUSAGE_TEXCOORD)
            continue;

        if (!minTexCoordElement || d3dElement->Offset < minTexCoordElement->Offset)
            minTexCoordElement = d3dElement;

        maxTexCoordUsageIndex = std::max<int32_t>(maxTexCoordUsageIndex, d3dElement->UsageIndex);
    }

    if (minTexCoordElement != nullptr && maxTexCoordUsageIndex >= 0)
    {
        // Redirect nonexistent tex coords to the one with the smallest usage index, which will be 0 in most cases.
        for (size_t j = maxTexCoordUsageIndex + 1; j < 4 && i < count - 1; i++, j++)
        {
            d3dElements[i] = *minTexCoordElement;
            d3dElements[i].UsageIndex = (BYTE)j;
        }
    }

    d3dElements[i] = D3DDECL_END();
    return true;
}

bool VertexBufferHandler::enabled = false;

void VertexBufferHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CreateVertexBuffer);
    INSTALL_HOOK(CreateSharedVertexBuffer);
    INSTALL_HOOK(CreateVertexElements);

    // Prevent the game's own endian swapping
    WRITE_NOP(0x72E9A2, 2);
}
