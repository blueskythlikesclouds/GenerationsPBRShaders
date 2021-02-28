#include "CompressionHandler.h"

struct BSCHeader
{
    char Magic[3];
    uint8_t Version;
    uint32_t CompressionType;
    uint32_t CompressedLength;
    uint32_t UncompressedLength;

    bool IsValid() const
    {
        return Magic[0] == 'B' && Magic[1] == 'S' && Magic[2] == 'C' && Version == 0;
    }
};

HOOK(uint32_t, __cdecl, DecompressCAB, 0xA92E54, void* a1, char* pBuffer, void* a3, void* a4, void* a5, void* a6, void* a7)
{
    uint8_t* pData = (uint8_t*)strtol(pBuffer, nullptr, 16);

    const BSCHeader* pHeader = *(BSCHeader**)pData;
    if (pHeader == nullptr || !pHeader->IsValid())
        return originalDecompressCAB(a1, pBuffer, a3, a4, a5, a6, a7);

    uint8_t* uncompressedData = (uint8_t*)Hedgehog::Base::fpOperatorNew(pHeader->UncompressedLength);

    hlDecompressNoAlloc((HlCompressType)pHeader->CompressionType, (uint8_t*)pHeader + sizeof(BSCHeader), 
        pHeader->CompressedLength, pHeader->UncompressedLength, uncompressedData);

    struct Output
    {
        boost::shared_ptr<uint8_t> spData;
        size_t length;
    };

    Output* pOutput = (Output*)a7;

    pOutput->spData = boost::shared_ptr<uint8_t>(uncompressedData, Hedgehog::Base::fpOperatorDelete);
    pOutput->length = pHeader->UncompressedLength;

    return true;
}

uint32_t compareMagicMidAsmHookReturnAddressOnTrue = 0x6A10AC;
uint32_t compareMagicMidAsmHookReturnAddressOnFalse = 0x6A1123;

void __declspec(naked) compareMagicMidAsmHook()
{
    __asm
    {
        cmp dword ptr[eax], 0x4643534D // MSCF
        jz onTrue

        cmp dword ptr[eax], 0x435342 // BSC\0
        jz onTrue

        jmp[compareMagicMidAsmHookReturnAddressOnFalse]

    onTrue:
        jmp[compareMagicMidAsmHookReturnAddressOnTrue]
    }
}

void CompressionHandler::applyPatches()
{
    WRITE_JUMP(0x6A10A4, compareMagicMidAsmHook);
    INSTALL_HOOK(DecompressCAB);
}
