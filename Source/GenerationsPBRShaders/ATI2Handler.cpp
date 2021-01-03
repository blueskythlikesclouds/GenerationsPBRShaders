#include "ATI2Handler.h"

HOOK(void, __cdecl, LoadPictureData, 0x743DE0, 
    Hedgehog::Mirage::CPictureData* pPictureData, const uint8_t* pData, size_t length, Hedgehog::Mirage::CRenderingInfrastructure* pRenderingInfrastructure)
{
    if (pPictureData->m_Flags & 1)
        return originalLoadPictureData(pPictureData, pData, length, pRenderingInfrastructure);

    if (pData && *(uint32_t*)pData == MAKEFOURCC('D', 'D', 'S', ' ') &&
        (*(uint32_t*)(pData + 0x54) == MAKEFOURCC('A', 'T', 'I', '2') || *(uint32_t*)(pData + 0x54) == MAKEFOURCC('B', 'C', '5', 'U')))
    {
        // ATI2 for some reason becomes YX instead of XY when read in shaders so we should do the swizzling before loading the texture.

        const uint32_t headerSize = *(uint32_t*)(pData + 4);

        uint64_t* data = (uint64_t*)(pData + 4 + headerSize);
        uint64_t* end = (uint64_t*)(pData + length);

        while (data < end)
        {
            const uint64_t r = data[0];
            const uint64_t g = data[1];

            data[0] = g;
            data[1] = r;

            data += 2;
        }
    }

    return originalLoadPictureData(pPictureData, pData, length, pRenderingInfrastructure);
}

bool ATI2Handler::enabled = false;

void ATI2Handler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(LoadPictureData);
}
