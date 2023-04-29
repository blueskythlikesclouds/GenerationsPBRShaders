#include "LUTHandler.h"
#include "ConstantBuffer.h"
#include "RenderDataManager.h"

bool LUTHandler::enabled = false;

hh::mr::SShaderPair fxLutShader;
boost::shared_ptr<hh::ygg::CYggTexture> lutTex;

HOOK(void, __fastcall, CFxRenderParticleInitialize, 0x10C7170, Sonic::CFxJob* This)
{
    fxLutShader = This->m_pScheduler->GetShader("FxFilterT", "FxLUT");
    lutTex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, NULL);
}

HOOK(void, __fastcall, CFxRenderParticleExecute, 0x10C80A0, Sonic::CFxJob* This)
{
    if (!globalUsePBR)
        return originalCFxRenderParticleExecute(This);

    if (!fxLutShader.m_spVertexShader || !fxLutShader.m_spPixelShader)
        return;

    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, lutTex->GetSurface());
    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(fxLutShader.m_spVertexShader, fxLutShader.m_spPixelShader);
    This->m_pScheduler->m_pMisc->m_pDevice->SetTexture(0, This->GetTexture("colortex"));
    This->m_pScheduler->m_pMisc->m_pDevice->SetTexture(1, RenderDataManager::rgbTablePicture);

    filterCB.lut.enable = RenderDataManager::rgbTablePicture && RenderDataManager::rgbTablePicture->m_spPictureData->IsMadeAll() &&
        SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_NONE && !SceneEffect::debug.disableLUT;

    filterCB.upload(This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice);

    This->m_pScheduler->m_pMisc->m_pDevice->DrawQuad2D(nullptr, 0, 0);

    This->SetDefaultTexture(lutTex);
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

    // Override the unused tonemap shader with GT tonemap.
    WRITE_MEMORY(0x16A1388, char*, "ToneMap_GranTurismo");
}
