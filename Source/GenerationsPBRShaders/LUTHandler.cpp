#include "LUTHandler.h"
#include "StageId.h"

Hedgehog::Mirage::SShaderPair lutShader;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> spLutPicture;

HOOK(void, __fastcall, CFxColorCorrectionInitialize, Sonic::fpCFxColorCorrectionInitialize, Sonic::CFxColorCorrection* This)
{
    originalCFxColorCorrectionInitialize(This);

    This->m_pScheduler->GetShader(lutShader, "FxFilterT2", "FxColorCorrectionLUT");
    spLutPicture = nullptr;
}

HOOK(void, __fastcall, CFxColorCorrectionExecute, Sonic::fpCFxColorCorrectionExecute, Sonic::CFxColorCorrection* This)
{
    originalCFxColorCorrectionExecute(This);

    if (StageId::hasChanged())
        spLutPicture = nullptr;

    if (StageId::isEmpty())
        return;

    if (!spLutPicture)
    {
        This->m_pScheduler->GetPicture(spLutPicture, (StageId::get() + "_rgb_table0").c_str());

        if (!spLutPicture)
            return;
    }

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> spDefaultTex;
    This->GetDefaultTexture(spDefaultTex);

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spSurface;
    spDefaultTex->GetSurface(spSurface, 0, 0);

    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, spSurface);
    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(lutShader.m_spVertexShader, lutShader.m_spPixelShader);

    This->m_pScheduler->m_pMisc->m_pDevice->SetSampler(0, spDefaultTex);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    This->m_pScheduler->m_pMisc->m_pDevice->SetSampler(1, spLutPicture);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(1, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(1, D3DTADDRESS_WRAP);

    This->m_pScheduler->m_pMisc->m_pDevice->RenderQuad(nullptr, 0, 0);
}

bool LUTHandler::enabled = false;

void LUTHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CFxColorCorrectionInitialize);
    INSTALL_HOOK(CFxColorCorrectionExecute);
}
