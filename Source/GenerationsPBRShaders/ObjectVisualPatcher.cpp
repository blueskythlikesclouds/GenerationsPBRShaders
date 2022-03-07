#include "ObjectVisualPatcher.h"

#pragma region Hook Macros

#define STRING_HOOK(name, stringPushInstrAddr, pbrStr)                           \
    constexpr uint32_t name##ObjVisualMidAsmHookInstrAddr = stringPushInstrAddr; \
    uint32_t name##ObjVisualMidAsmHookRetAddr = (stringPushInstrAddr)+5;         \
    const char* volatile name##ObjVisualStr[] =                                  \
    {                                                                            \
        *(const char**)((stringPushInstrAddr)+1),                                \
        pbrStr                                                                   \
    };                                                                           \
                                                                                 \
    void __declspec(naked) name##ObjVisualMidAsmHook()                           \
    {                                                                            \
        __asm { push eax }                                                       \
        __asm { push ebx }                                                       \
        __asm { lea eax, [globalUsePBR] }                                              \
        __asm { movzx eax, byte ptr[eax] }                                       \
        __asm { lea ebx, name##ObjVisualStr }                                    \
        __asm { mov ecx, [ebx + eax * 4] }                                       \
        __asm { pop ebx }                                                        \
        __asm { pop eax }                                                        \
        __asm { push ecx }                                                       \
        __asm { jmp[name##ObjVisualMidAsmHookRetAddr] }                          \
    }                                                                            \

#define INSTALL_STRING_HOOK(name) \
    WRITE_JUMP(name##ObjVisualMidAsmHookInstrAddr, name##ObjVisualMidAsmHook)

#pragma endregion

STRING_HOOK(UpReelRopeVertexShader, 0x102FE31, "Default2_@cd@_ConstTexCoord");
STRING_HOOK(UpReelRopePixelShader, 0x102FE74, "Common2_dp@@d_NoLight_NoGI_ConstTexCoord");
STRING_HOOK(UpReelRopeDiffuse, 0x102FEB7, "cmn_metal_ms_wire_HD_abd");
STRING_HOOK(UpReelRopeSpecular, 0x102FEF8, "cmn_metal_ms_wire_HD_prm");

STRING_HOOK(PulleyRopeShader, 0x11210AB, "Common2_dp");
STRING_HOOK(PulleyRopeDiffuse, 0x11211B3, "cmn_metal_ms_wire_HD_abd");
STRING_HOOK(PulleyRopeSpecular, 0x1121219, "cmn_metal_ms_wire_HD_prm");
STRING_HOOK(PulleyRopeSlot, 0x1120684, "specular");

HOOK(void, __fastcall, LoadArchive, 0x69AB10, void* This, void* Edx, void* A3, void* A4, const hh::base::CSharedString& name, void* archiveInfo, void* A7, void* A8)
{
    if (strstr(name.c_str(), "PBR") != nullptr)
        (*(int32_t*)archiveInfo) += 0x0BADF00D; // Priority

    return originalLoadArchive(This, Edx, A3, A4, name, archiveInfo, A7, A8);
}

HOOK(void, __fastcall, LoadArchiveList, 0x69C270, void* This, void* Edx, void* A3, void* A4, const hh::base::CSharedString& name, void* archiveInfo)
{
    if (strstr(name.c_str(), "PBR") != nullptr)
        (*(int32_t*)archiveInfo) += 0x0BADF00D; // Priority

    return originalLoadArchiveList(This, Edx, A3, A4, name, archiveInfo);
}

bool ObjectVisualPatcher::enabled = false;

void ObjectVisualPatcher::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_STRING_HOOK(UpReelRopeVertexShader);
    INSTALL_STRING_HOOK(UpReelRopePixelShader);
    INSTALL_STRING_HOOK(UpReelRopeDiffuse);
    INSTALL_STRING_HOOK(UpReelRopeSpecular);

    INSTALL_STRING_HOOK(PulleyRopeShader);
    INSTALL_STRING_HOOK(PulleyRopeDiffuse);
    INSTALL_STRING_HOOK(PulleyRopeSpecular);
    INSTALL_STRING_HOOK(PulleyRopeSlot);

    INSTALL_HOOK(LoadArchive);
    INSTALL_HOOK(LoadArchiveList);
}