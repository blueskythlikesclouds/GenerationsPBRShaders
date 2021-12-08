#include "LightShaftHandler.h"

HOOK(void, __fastcall, CFxLightShaftExecute, 0x11B1610, Sonic::CFxJob* This)
{
    bool* const isEnable = (bool*)0x1E5E333;
    Eigen::AlignedVector3f* sunPosition = (Eigen::AlignedVector3f*)0x1A57410;

    if (!*isEnable || !globalUsePBR)
    {
        originalCFxLightShaftExecute(This);
        return;
    }

    // Assume sun position to be view space and convert it to world space.
    const Eigen::AlignedVector3f sunPosBackup = *sunPosition;

    *sunPosition += 
        Sonic::CGameDocument::GetInstance()->GetWorld()->GetCamera()->m_MyCamera.m_Position;

    originalCFxLightShaftExecute(This);

    *sunPosition = sunPosBackup;
}

bool LightShaftHandler::enabled = false;

void LightShaftHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CFxLightShaftExecute);
}
