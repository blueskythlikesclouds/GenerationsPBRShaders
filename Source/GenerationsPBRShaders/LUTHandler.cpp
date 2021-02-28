#include "LUTHandler.h"
#include "StageId.h"

bool LUTHandler::enabled = false;

Hedgehog::Mirage::SShaderPair s_FxLUTShader;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> s_spLutPicture;

HOOK(void, __fastcall, CFxCrossFadeInitialize, Sonic::fpCFxCrossFadeInitialize, Sonic::CFxCrossFade* This)
{
    originalCFxCrossFadeInitialize(This);

    This->m_pScheduler->GetShader(s_FxLUTShader, "FxFilterT", "FxLUT");
    s_spLutPicture.reset();
}

HOOK(void, __fastcall, CFxCrossFadeExecute, Sonic::fpCFxCrossFadeExecute, Sonic::CFxCrossFade* This)
{
    if (!s_FxLUTShader.m_spVertexShader || !s_FxLUTShader.m_spPixelShader)
        return originalCFxCrossFadeExecute(This);

    if (StageId::hasChanged())
        s_spLutPicture.reset();

    if (!s_spLutPicture)
        This->m_pScheduler->GetPicture(s_spLutPicture, (StageId::get() + "_rgb_table0").c_str());

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> spDefaultTex;
    This->GetDefaultTexture(spDefaultTex);

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spSurface;
    spDefaultTex->GetSurface(spSurface, 0, 0);

    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, spSurface);
    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(s_FxLUTShader.m_spVertexShader, s_FxLUTShader.m_spPixelShader);

    This->m_pScheduler->m_pMisc->m_pDevice->SetSampler(0, spDefaultTex);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    This->m_pScheduler->m_pMisc->m_pDevice->SetSampler(1, s_spLutPicture);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(1, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(1, D3DTADDRESS_CLAMP);

    const BOOL isEnableLUT[] = { s_spLutPicture != nullptr && SceneEffect::Debug.ViewMode == DEBUG_VIEW_MODE_NONE };
    This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->SetPixelShaderConstantB(8, isEnableLUT, 1);

    This->m_pScheduler->m_pMisc->m_pDevice->RenderQuad(nullptr, 0, 0);

    This->SetDefaultTexture(spDefaultTex);

    originalCFxCrossFadeExecute(This);
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

    INSTALL_HOOK(CFxCrossFadeInitialize);
    INSTALL_HOOK(CFxCrossFadeExecute);

    // Prevent gamma ramp from being set.
    WRITE_MEMORY(0xD68183, uint8_t, 0x83, 0xC4, 0x08, 0x90, 0x90);
}
