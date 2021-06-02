#include "BloomHandler.h"

hh::mr::SShaderPair pbrBloomShader;

HOOK(void, __fastcall, CFxBloomGlareInitialize, Sonic::fpCFxBloomGlareInitialize, Sonic::CFxBloomGlare* This)
{
    originalCFxBloomGlareInitialize(This);

    This->m_pScheduler->GetShader(pbrBloomShader, "RenderBuffer", "PBR_Bloom_BrightPassHDR");
}

uint32_t CFxBloomGlareExecuteMidAsmHookReturnAddress = 0x10D3ADD;

void __declspec(naked) CFxBloomGlareExecuteMidAsmHook()
{
    __asm
    {
        lea ebx, [globalUsePBR]
        mov bl, byte ptr [ebx]
        cmp bl, 0
        jz onFalse
        lea ebx, [pbrBloomShader]
        jmp end
    onFalse:
        lea ebx, [esi + 0xE0]
    end:
        jmp[CFxBloomGlareExecuteMidAsmHookReturnAddress]
    }
}

bool BloomHandler::enabled = false;

void BloomHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CFxBloomGlareInitialize);
    WRITE_JUMP(0x10D3AD7, CFxBloomGlareExecuteMidAsmHook);
}
