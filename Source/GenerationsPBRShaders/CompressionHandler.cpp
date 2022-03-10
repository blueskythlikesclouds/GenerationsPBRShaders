#include "CompressionHandler.h"

struct BSCHeader
{
    char magic[3];
    uint8_t version;
    uint32_t compressionType;
    uint32_t compressedLength;
    uint32_t uncompressedLength;

    bool isValid() const
    {
        return magic[0] == 'B' && magic[1] == 'S' && magic[2] == 'C' && version == 0;
    }
};


HOOK(uint32_t, __cdecl, DecompressCAB, 0xA92E54, void* a1, char* pBuffer, void* a3, void* a4, void* a5, void* a6, void* a7)
{
    uint8_t* pData = (uint8_t*)strtol(pBuffer, nullptr, 16);

    const BSCHeader* header = *(BSCHeader**)pData;
    if (header == nullptr || !header->isValid())
        return originalDecompressCAB(a1, pBuffer, a3, a4, a5, a6, a7);


    const auto uncompressedData = boost::make_shared<uint8_t[]>(header->uncompressedLength);

    hl::decompress_no_alloc((hl::compress_type)(header->compressionType & 0xFFFF), header->compressedLength, (uint8_t*)header + sizeof(BSCHeader),
        header->uncompressedLength, uncompressedData.get());

    struct Output
    {
        boost::shared_ptr<uint8_t[]> data;
        size_t length;
    };

    Output* output = (Output*)a7;

    output->data = uncompressedData;
    output->length = header->uncompressedLength;

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
