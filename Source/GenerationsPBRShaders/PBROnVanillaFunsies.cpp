#include "PBROnVanillaFunsies.h"

bool PBROnVanillaFunsies::enabled = false;

void PBROnVanillaFunsies::onFrame()
{
    if (!enabled)
        return;

    SceneEffect::Debug.ReflectanceOverride = 0.17f;
    SceneEffect::Debug.RoughnessOverride = 0.01f;
    SceneEffect::Debug.MetalnessOverride = 0;
    SceneEffect::RLR.Enable = true;
    SceneEffect::RLR.Brightness = 3;
    SceneEffect::RLR.MaxLod = 0;

    /*SceneEffect::SSAO.Enable = true;
    SceneEffect::SSAO.Strength = 1;*/

    *(float*)0x1A4356C = 0;
}

uint32_t trampolineFunctionAddress = 0x6621A0;
uint32_t trampolineReturnAddress = 0x74251B;

void __fastcall trampolineFunc(Hedgehog::Base::CSharedString* This, void* Edx, const char* name)
{
    if (strstr(name, "Water"))
    {
        Hedgehog::Base::fpCSharedStringCtor(This, "Water05");
    }

    else if (strstr(name, "CDRF") || strstr(name, "Ring") || strstr(name, "2_") || strstr(name, "Sky"))
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

template<typename From, typename To>
To union_cast(From value)
{
    union
    {
        From from;
        To to;
    } union_struct;

    union_struct.from = value;
    return union_struct.to;
}

HOOK(void, __cdecl, LoadLightData, 0x740920, void* A1, void* A2)
{
    uint32_t* color = (uint32_t*)((uint32_t)A2 + 0x10);

    color[0] = _byteswap_ulong(union_cast<float, uint32_t>(union_cast<uint32_t, float>(_byteswap_ulong(color[0])) * 3.1415926535897932384626433832795028f));
    color[1] = _byteswap_ulong(union_cast<float, uint32_t>(union_cast<uint32_t, float>(_byteswap_ulong(color[1])) * 3.1415926535897932384626433832795028f));
    color[2] = _byteswap_ulong(union_cast<float, uint32_t>(union_cast<uint32_t, float>(_byteswap_ulong(color[2])) * 3.1415926535897932384626433832795028f));

    originalLoadLightData(A1, A2);
}

void PBROnVanillaFunsies::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    WRITE_JUMP(0x742516, trampoline);
    //INSTALL_HOOK(LoadLightData);
}
