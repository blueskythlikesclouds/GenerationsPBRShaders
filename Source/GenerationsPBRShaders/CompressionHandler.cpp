#include "CompressionHandler.h"

HOOK(void*, __fastcall, DecompressDatabase, 0x6A1080, void* This, void* Edx, void* a2, void* a3, const boost::shared_ptr<uint8_t[]>& data, uint32_t length, uint32_t length2, void* a7, void* a8)
{
    if (!data)
        return originalDecompressDatabase(This, Edx, a2, a3, data, length, length2, a7, a8);

    if (length > 16 && *(uint32_t*)data.get() == 0x435342)
    {
        const HlCompressType compressionType = *(HlCompressType*)(data.get() + 4);
        const uint32_t compressedSize = *(uint32_t*)(data.get() + 8);
        const uint32_t uncompressedSize  = *(uint32_t*)(data.get() + 12);

        uint8_t* uncompressedData = nullptr;
        hlDecompress(compressionType, data.get() + 16, compressedSize, uncompressedSize, (void**)&uncompressedData);

        return originalDecompressDatabase(This, Edx, a2, a3, boost::shared_ptr<uint8_t[]>(uncompressedData, HlFreePtr), uncompressedSize, uncompressedSize, a7, a8);
    }

    return originalDecompressDatabase(This, Edx, a2, a3, data, length, length2, a7, a8);
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
    INSTALL_HOOK(DecompressDatabase);
}
