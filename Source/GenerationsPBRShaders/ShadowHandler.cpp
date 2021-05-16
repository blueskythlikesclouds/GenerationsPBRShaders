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

    // Linear -> Point
    WRITE_MEMORY(0x10C7D6B, uint8_t, 0x00);
    WRITE_MEMORY(0x10C7D6B + 2, uint8_t, 0x01);
    WRITE_MEMORY(0x10C7D6B + 4, uint8_t, 0x01);

    WRITE_MEMORY(0x10C7DC2, uint8_t, 0x00);
    WRITE_MEMORY(0x10C7DC2 + 2, uint8_t, 0x01);
    WRITE_MEMORY(0x10C7DC2 + 4, uint8_t, 0x01);

    WRITE_MEMORY(0x10C7EA9, uint8_t, 0x00);
    WRITE_MEMORY(0x10C7EA9 + 2, uint8_t, 0x01);
    WRITE_MEMORY(0x10C7EA9 + 4, uint8_t, 0x01);

    WRITE_MEMORY(0x10C7F00, uint8_t, 0x00);
    WRITE_MEMORY(0x10C7F00 + 2, uint8_t, 0x01);
    WRITE_MEMORY(0x10C7F00 + 4, uint8_t, 0x01);
}
