﻿#include "LUTHandler.h"
#include "RenderDataManager.h"

bool LUTHandler::enabled = false;

hh::mr::SShaderPair fxLutShader;

HOOK(void, __fastcall, CFxRenderParticleInitialize, 0x10C7170, Sonic::CFxJob* This)
{
    This->m_pScheduler->GetShader(fxLutShader, "FxFilterT", "FxLUT");
}

HOOK(void, __fastcall, CFxRenderParticleExecute, 0x10C80A0, Sonic::CFxJob* This)
{
    if (!globalUsePBR)
        return originalCFxRenderParticleExecute(This);

    if (!fxLutShader.m_spVertexShader || !fxLutShader.m_spPixelShader)
        return;

    boost::shared_ptr<hh::ygg::CYggTexture> colorTex;
    This->GetTexture(colorTex, "colortex");

    boost::shared_ptr<hh::ygg::CYggSurface> surface;
    colorTex->GetSurface(surface, 0, 0);

    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, surface);
    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(fxLutShader.m_spVertexShader, fxLutShader.m_spPixelShader);

    This->m_pScheduler->m_pMisc->m_pDevice->SetSampler(0, colorTex);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    This->m_pScheduler->m_pMisc->m_pDevice->SetSampler(1, RenderDataManager::rgbTablePicture);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(1, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(1, D3DTADDRESS_CLAMP);

    const BOOL isEnableLUT[] = { RenderDataManager::rgbTablePicture != nullptr && 
        SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_NONE && !SceneEffect::debug.disableLUT };

    This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->SetPixelShaderConstantB(8, isEnableLUT, 1);

    This->m_pScheduler->m_pMisc->m_pDevice->RenderQuad(nullptr, 0, 0);

    This->SetDefaultTexture(colorTex);
}

void LUTHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    // R8G8B8A8 -> R16G16B16A16
    // This is necessary to reduce banding.
    WRITE_MEMORY(0x10C4FCB, uint8_t, 36);
    WRITE_MEMORY(0x10D1D11, uint8_t, 36);
    WRITE_MEMORY(0x10D1D74, uint8_t, 36);
    WRITE_MEMORY(0x10D1DD7, uint8_t, 36);
    WRITE_MEMORY(0x10D1E4E, uint8_t, 36);
    WRITE_MEMORY(0x10D1ECB, uint8_t, 36);
    WRITE_MEMORY(0x10D1F37, uint8_t, 36);
    WRITE_MEMORY(0x11B0FF6, uint8_t, 36);
    WRITE_MEMORY(0x11B105B, uint8_t, 36);
    WRITE_MEMORY(0x1228644, uint8_t, 36);
    WRITE_MEMORY(0x12286E7, uint8_t, 36);
    WRITE_MEMORY(0x10CD218, uint8_t, 36);
    WRITE_MEMORY(0x10CD27C, uint8_t, 36);
    WRITE_MEMORY(0x10D08E3, uint8_t, 36);
    WRITE_MEMORY(0x10C2CF3, uint8_t, 36);
    WRITE_MEMORY(0x10C3754, uint8_t, 36);
    WRITE_MEMORY(0x10C21C4, uint8_t, 36);

    INSTALL_HOOK(CFxRenderParticleInitialize);
    INSTALL_HOOK(CFxRenderParticleExecute);

    // Prevent gamma ramp from being set.
    WRITE_MEMORY(0xD68183, uint8_t, 0x83, 0xC4, 0x08, 0x90, 0x90);
}
