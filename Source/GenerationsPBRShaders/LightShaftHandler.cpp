#include "LightShaftHandler.h"

HOOK(void, __fastcall, CFxLightShaftExecute, 0x11B1610, Sonic::CFxJob* This)
{
    bool* const pIsEnable = (bool*)0x1E5E333;
    Eigen::Vector3f* pSunPosition = (Eigen::Vector3f*)0x1A57410;

    if (!*pIsEnable)
    {
        originalCFxLightShaftExecute(This);
        return;
    }

    // Assume sun position to be view space and convert it to world space.
    const Eigen::Vector3f sunPosBackup = *pSunPosition;

    *pSunPosition += 
        This->m_pScheduler->m_pMisc->m_spSceneRenderer->m_pCamera->m_Position;

    originalCFxLightShaftExecute(This);

    *pSunPosition = sunPosBackup;
}

bool LightShaftHandler::enabled = false;

void LightShaftHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CFxLightShaftExecute);
}
