#include "PBROnVanillaFunsies.h"

bool PBROnVanillaFunsies::enabled = false;

void PBROnVanillaFunsies::onFrame()
{
    if (!enabled)
        return;

    FUNCTION_PTR(void, __cdecl, funD887D0, 0xD887D0);

    *(float*)0x1A489EC = 0;
    funD887D0();

    SceneEffect::Debug.FresnelFactorOverride = 0.17f;
    SceneEffect::Debug.RoughnessOverride = 0.01f;
    SceneEffect::Debug.MetalnessOverride = 0;
    SceneEffect::RLR.Enable = true;
    SceneEffect::RLR.Brightness = 2;
}

uint32_t trampolineFunctionAddress = 0x6621A0;
uint32_t trampolineReturnAddress = 0x74251B;

void __fastcall trampolineFunc(Hedgehog::Base::CSharedString* This, void* Edx, const char* name)
{
    if (strstr(name, "Sky_"))
    {
        Hedgehog::Base::fpCSharedStringCtor(This, "Sky2_d");
    }

    else if (strstr(name, "Water"))
    {
        Hedgehog::Base::fpCSharedStringCtor(This, "Water05");
    }

    else if (strstr(name, "CDRF") || strstr(name, "Ring") || strstr(name, "2_"))
    {
        Hedgehog::Base::fpCSharedStringCtor(This, name);
    }

    else if (strstr(name, "pn") || strstr(name, "sn") || strstr(name, "_dn"))
    {
        if (strstr(name, "Blend"))
            Hedgehog::Base::fpCSharedStringCtor(This, "Blend2_dndn");
        else
            Hedgehog::Base::fpCSharedStringCtor(This, "Common2_dn");
    }

    else
    {
        if (strstr(name, "Blend"))
            Hedgehog::Base::fpCSharedStringCtor(This, "Blend2_dd");
        else
            Hedgehog::Base::fpCSharedStringCtor(This, "Common2_d");
    }
}

void __declspec(naked) trampoline()
{
    __asm
    {
        call trampolineFunc
        jmp[trampolineReturnAddress]
    }
}

void PBROnVanillaFunsies::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    WRITE_JUMP(0x742516, trampoline);
}
