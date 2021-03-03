#include "ShadowHandler.h"

constexpr uint32_t RAWZ = MAKEFOURCC('R', 'A', 'W', 'Z');

uint32_t CFxShadowMapInitializeMidAsmHookReturnAddress = 0x10C60B0;

void __declspec(naked) CFxShadowMapInitializeMidAsmHook()
{
    __asm
    {
        push RAWZ // RAWZ instead of D24S8 so GPU doesn't do depth comparison
        push 2
        push 1
        jmp [CFxShadowMapInitializeMidAsmHookReturnAddress]
    }
}

bool ShadowHandler::enabled = false;

void ShadowHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    WRITE_JUMP(0x10C60AA, CFxShadowMapInitializeMidAsmHook);
}
