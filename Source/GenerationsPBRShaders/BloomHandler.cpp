#include "BloomHandler.h"

Hedgehog::Mirage::SShaderPair s_PBR_Bloom_BrightPassHDRShader;

HOOK(void, __fastcall, CFxBloomGlareInitialize, Sonic::fpCFxBloomGlareInitialize, Sonic::CFxBloomGlare* This)
{
    originalCFxBloomGlareInitialize(This);

    This->m_pScheduler->GetShader(s_PBR_Bloom_BrightPassHDRShader, "RenderBuffer", "PBR_Bloom_BrightPassHDR");
}

uint32_t pCFxBloomGlareExecuteMidAsmHookReturnAddress = 0x10D3ADD;

void __declspec(naked) fCFxBloomGlareExecuteMidAsmHook()
{
    __asm
    {
        lea ebx, [g_UsePBR]
        mov bl, byte ptr [ebx]
        cmp bl, 0
        jz onFalse
        lea ebx, [s_PBR_Bloom_BrightPassHDRShader]
        jmp end
    onFalse:
        lea ebx, [esi + 0xE0]
    end:
        jmp[pCFxBloomGlareExecuteMidAsmHookReturnAddress]
    }
}

bool BloomHandler::enabled = false;

void BloomHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CFxBloomGlareInitialize);
    WRITE_JUMP(0x10D3AD7, fCFxBloomGlareExecuteMidAsmHook);
}
