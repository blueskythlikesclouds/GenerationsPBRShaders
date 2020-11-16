#include "YggdrasillPatcher.h"

uint32_t setRenderTargetsMidAsmHookReturnAddress = 0x78E1EF;
uint32_t setRenderTargetsMidAsmHookFunctionAddress = 0x6F7950;

void __declspec(naked) setRenderTargetsMidAsmHook()
{
    __asm
    {
        // YggSurface
        test ecx, ecx
        jz setTarget

        mov ecx, [ecx + 0x1C] // DX_PATCH IDirect3DSurface9

    setTarget:
        push ecx
        push edi
        mov ecx, [esi + 0x98]
        mov ecx, [ecx]
        add ecx, 0x60
        call [setRenderTargetsMidAsmHookFunctionAddress]

        jmp[setRenderTargetsMidAsmHookReturnAddress]
    }
}

bool YggdrasillPatcher::enabled = false;

void YggdrasillPatcher::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    // Yggdrasill doesn't handle NULL render targets,
    // so we have to handle it ourselves.
    WRITE_JUMP(0x78E1D0, setRenderTargetsMidAsmHook);
}
