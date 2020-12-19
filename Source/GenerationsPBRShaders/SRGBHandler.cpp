#include "SRGBHandler.h"

bool SRGBHandler::enabled = false;

Hedgehog::Mirage::SShaderPair srgbShader;

HOOK(void, __fastcall, CFxBloomGlareInitialize, Sonic::fpCFxBloomGlareInitialize, Sonic::CFxBloomGlare* This)
{
    This->m_pScheduler->GetShader(srgbShader, "FxFilterT2", "FxToSRGB");

    originalCFxBloomGlareInitialize(This);
}

HOOK(void, __fastcall, CFxBloomGlareExecute, Sonic::fpCFxBloomGlareExecute, Sonic::CFxBloomGlare* This)
{
    originalCFxBloomGlareExecute(This);

    if (!srgbShader.m_spVertexShader || !srgbShader.m_spPixelShader)
        return;

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> spDefaultTex;
    This->GetDefaultTexture(spDefaultTex);

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spSurface;
    spDefaultTex->GetSurface(spSurface, 0, 0);

    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, spSurface);
    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(srgbShader.m_spVertexShader, srgbShader.m_spPixelShader);

    This->m_pScheduler->m_pMisc->m_pDevice->SetSampler(0, spDefaultTex);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    This->m_pScheduler->m_pMisc->m_pDevice->RenderQuad(nullptr, 0, 0);
}

void SRGBHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    // R8G8B8A8 -> R16G16B16A16
    // This is necessary to reduce banding.
    WRITE_MEMORY(0x010C4FCB, uint8_t, 36);
    WRITE_MEMORY(0x010D1D11, uint8_t, 36);
    WRITE_MEMORY(0x010D1D74, uint8_t, 36);
    WRITE_MEMORY(0x010D1DD7, uint8_t, 36);
    WRITE_MEMORY(0x010D1E4E, uint8_t, 36);
    WRITE_MEMORY(0x010D1ECB, uint8_t, 36);
    WRITE_MEMORY(0x010D1F37, uint8_t, 36);

    INSTALL_HOOK(CFxBloomGlareInitialize);
    INSTALL_HOOK(CFxBloomGlareExecute);
}
