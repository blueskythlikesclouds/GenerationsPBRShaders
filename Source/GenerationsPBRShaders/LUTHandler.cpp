#include "LUTHandler.h"
#include "RenderDataManager.h"

bool LUTHandler::enabled = false;

Hedgehog::Mirage::SShaderPair s_FxLUTShader;

HOOK(void, __fastcall, CFxRenderParticleInitialize, 0x10C7170, Sonic::CFxJob* This)
{
    This->m_pScheduler->GetShader(s_FxLUTShader, "FxFilterT", "FxLUT");
}

HOOK(void, __fastcall, CFxRenderParticleExecute, 0x10C80A0, Sonic::CFxJob* This)
{
    if (!s_FxLUTShader.m_spVertexShader || !s_FxLUTShader.m_spPixelShader)
        return;

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> spColorTex;
    This->GetTexture(spColorTex, "colortex");

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spSurface;
    spColorTex->GetSurface(spSurface, 0, 0);

    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, spSurface);
    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(s_FxLUTShader.m_spVertexShader, s_FxLUTShader.m_spPixelShader);

    This->m_pScheduler->m_pMisc->m_pDevice->SetSampler(0, spColorTex);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    This->m_pScheduler->m_pMisc->m_pDevice->SetSampler(1, RenderDataManager::ms_spRgbTablePicture);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(1, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(1, D3DTADDRESS_CLAMP);

    const BOOL isEnableLUT[] = { RenderDataManager::ms_spRgbTablePicture != nullptr && 
        SceneEffect::Debug.ViewMode == DEBUG_VIEW_MODE_NONE && !SceneEffect::Debug.DisableLUT };

    This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->SetPixelShaderConstantB(8, isEnableLUT, 1);

    This->m_pScheduler->m_pMisc->m_pDevice->RenderQuad(nullptr, 0, 0);

    This->SetDefaultTexture(spColorTex);
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
