#include "BloomHandler.h"

constexpr size_t COLORS_BLOOM_WIDTH = 160;
constexpr size_t COLORS_BLOOM_HEIGHT = 120;
constexpr size_t COLORS_BLOOM_BUFFER_COUNT = 4;

boost::shared_ptr<hh::ygg::CYggTexture> colorsBrightPassSrcTex;
std::array<boost::shared_ptr<hh::ygg::CYggTexture>, COLORS_BLOOM_BUFFER_COUNT> colorsBloomTex;
std::array<boost::shared_ptr<hh::ygg::CYggTexture>, COLORS_BLOOM_BUFFER_COUNT> colorsBloomTmpTex;

hh::mr::SShaderPair pbrBloomShader;
hh::mr::SShaderPair colorsBloomShader;

hh::mr::SShaderPair downSampleNShader;
hh::mr::SShaderPair downSample4Shader;
hh::mr::SShaderPair blendColorShader;
hh::mr::SShaderPair bicubicFilterShader;

HOOK(void, __fastcall, CFxBloomGlareInitialize, Sonic::fpCFxBloomGlareInitialize, Sonic::CFxBloomGlare* This)
{
    originalCFxBloomGlareInitialize(This);

    colorsBrightPassSrcTex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(COLORS_BLOOM_WIDTH, COLORS_BLOOM_HEIGHT, 
        1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, nullptr);

    for (size_t i = 0; i < COLORS_BLOOM_BUFFER_COUNT; i++)
    {
        colorsBloomTex[i] = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(COLORS_BLOOM_WIDTH >> i, COLORS_BLOOM_HEIGHT >> i,
            1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, nullptr);

        colorsBloomTmpTex[i] = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(COLORS_BLOOM_WIDTH >> i, COLORS_BLOOM_HEIGHT >> i,
            1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, nullptr);
    }

    pbrBloomShader = This->m_pScheduler->GetShader("RenderBuffer", "PBR_Bloom_BrightPassHDR");
    colorsBloomShader = This->m_pScheduler->GetShader("RenderBuffer", "Colors_Bloom_BrightPassHDR");
    downSampleNShader = This->m_pScheduler->GetShader("FxFilterT", "FxDownSampleN");
    downSample4Shader = This->m_pScheduler->GetShader("RenderBuffer", "DownSample4");
    blendColorShader = This->m_pScheduler->GetShader("FxFilterT", "FxBlendColor");
    bicubicFilterShader = This->m_pScheduler->GetShader("FxFilterT", "FxBicubicFilter");
}

bool __cdecl isUsePBRBloomShader()
{
    switch (SceneEffect::bloom.type)
    {
    case BLOOM_TYPE_DEFAULT:
        return globalUsePBR;

    case BLOOM_TYPE_FORCES:
        return true;

    case BLOOM_TYPE_BFXP:
    case BLOOM_TYPE_COLORS:
        return false;
    }

    return false;
}

uint32_t CFxBloomGlareExecuteMidAsmHookReturnAddress = 0x10D3ADD;

void __declspec(naked) CFxBloomGlareExecuteMidAsmHook()
{
    __asm
    {
        push ecx
        call isUsePBRBloomShader
        pop ecx
        cmp al, 0
        jz onFalse

        lea ebx, [pbrBloomShader]
        jmp end
    onFalse:
        lea ebx, [esi + 0xE0]
    end:
        jmp[CFxBloomGlareExecuteMidAsmHookReturnAddress]
    }
}

HOOK(void, __fastcall, CFxBloomGlareExecute, Sonic::fpCFxBloomGlareExecute, Sonic::CFxBloomGlare* This)
{
    if (SceneEffect::bloom.type != BLOOM_TYPE_COLORS || !*(bool*)0x1A4323D || !*(bool*)0x1A4358E) // ms_Enable
    {
        originalCFxBloomGlareExecute(This);
        return;
    }

    //
    // Down Sample N
    //
    const auto hdrTex = This->GetTexture("hdr");

    const float rcpWidth = 1.0f / (float)hdrTex->m_CreationParams.Width;
    const float rcpHeight = 1.0f / (float)hdrTex->m_CreationParams.Height;

    const float blockWidth = std::max(1.0f, (float)hdrTex->m_CreationParams.Width / (float)colorsBrightPassSrcTex->m_CreationParams.Width);
    const float blockHeight = std::max(1.0f, (float)hdrTex->m_CreationParams.Height / (float)colorsBrightPassSrcTex->m_CreationParams.Height);

    float downSampleParam[] =
    {
        (blockWidth / 2.0f + 0.5f) * rcpWidth,
        rcpWidth,
        (blockHeight / 2.0f + 0.5f) * rcpHeight,
        rcpHeight
    };

    This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->SetPixelShaderConstantF(150, downSampleParam, 1);

    This->m_pScheduler->m_pMisc->m_pDevice->SetTexture(0, hdrTex);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(downSampleNShader);
    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, colorsBrightPassSrcTex->GetSurface());
    This->m_pScheduler->m_pMisc->m_pDevice->DrawQuad2D(nullptr, 0, 0);

    //
    // Bright Pass
    //
    This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->SetPixelShaderConstantF(150, (const float*)0x1A572D0, 1);
    This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->SetPixelShaderConstantF(151, (const float*)0x1A57360, 1);

    This->m_pScheduler->m_pMisc->m_pDevice->SetTexture(0, colorsBrightPassSrcTex);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);

    This->m_pScheduler->m_pMisc->m_pDevice->SetTexture(1, This->GetTexture("luavg"));
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(1, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(1, D3DTADDRESS_CLAMP);

    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(colorsBloomShader);
    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, colorsBloomTex[0]->GetSurface());
    This->m_pScheduler->m_pMisc->m_pDevice->DrawQuad2D(nullptr, 0, 0);

    //
    // Down Sample
    //
    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(downSample4Shader);

    for (size_t i = 1; i < COLORS_BLOOM_BUFFER_COUNT; i++)
    {
        This->m_pScheduler->m_pMisc->m_pDevice->SetTexture(0, colorsBloomTex[i - 1]);
        This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, colorsBloomTex[i]->GetSurface());
        This->m_pScheduler->m_pMisc->m_pDevice->DrawQuad2D(nullptr, 0, 0);
    }

    //
    // Blur
    //
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.LockRenderState(D3DRS_ALPHABLENDENABLE);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.LockRenderState(D3DRS_SRCBLEND);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.LockRenderState(D3DRS_DESTBLEND);

    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(blendColorShader);

    for (size_t i = 0; i < COLORS_BLOOM_BUFFER_COUNT; i++)
    {
        for (size_t j = 0; j < 2; j++)
        {
            This->m_pScheduler->m_pMisc->m_pDevice->SetTexture(0, j == 0 ? colorsBloomTex[i] : colorsBloomTmpTex[i]);
            This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, (j == 0 ? colorsBloomTmpTex[i] : colorsBloomTex[i])->GetSurface());

            for (int k = 0; k < 1 + (1 << (i + 1)); k++)
            {
                float x = 0.0f;
                float y = 0.0f;

                if (j == 0)
                    x = (float)((k + 1) / 2);
                else
                    y = (float)((k + 1) / 2);

                if ((k % 2) == 0)
                {
                    x *= -1;
                    y *= -1;
                }

                float srcAlphaDstAlpha[] = { 1.0f, 1.0f / (k + 1.0f), 0, 0 };
                This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->SetPixelShaderConstantF(150, srcAlphaDstAlpha, 1);

                This->m_pScheduler->m_pMisc->m_pDevice->DrawQuad2D(nullptr, x, y);
            }
        }
    }

    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.UnlockRenderState(D3DRS_SRCBLEND);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.UnlockRenderState(D3DRS_DESTBLEND);

    //
    // Combine
    //
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.LockRenderState(D3DRS_ALPHABLENDENABLE);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.LockRenderState(D3DRS_SRCBLEND);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.LockRenderState(D3DRS_DESTBLEND);

    const auto defaultTex = This->GetDefaultTexture();

    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(bicubicFilterShader);
    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, defaultTex->GetSurface());

    for (size_t i = 0; i < COLORS_BLOOM_BUFFER_COUNT; i++)
    {
        float textureSize[] =
        {
            (float)colorsBloomTex[i]->m_CreationParams.Width,
            (float)colorsBloomTex[i]->m_CreationParams.Height,
            1.0f / (float)colorsBloomTex[i]->m_CreationParams.Width,
            1.0f / (float)colorsBloomTex[i]->m_CreationParams.Height,
        };

        This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->SetPixelShaderConstantF(150, textureSize, 1);

        This->m_pScheduler->m_pMisc->m_pDevice->SetTexture(0, colorsBloomTex[i]);
        This->m_pScheduler->m_pMisc->m_pDevice->DrawQuad2D(nullptr, 0, 0);
    }

    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.UnlockRenderState(D3DRS_SRCBLEND);
    This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice.UnlockRenderState(D3DRS_DESTBLEND);

    This->SetDefaultTexture(defaultTex);
}

bool BloomHandler::enabled = false;

void BloomHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CFxBloomGlareInitialize);
    WRITE_JUMP(0x10D3AD7, CFxBloomGlareExecuteMidAsmHook);
    INSTALL_HOOK(CFxBloomGlareExecute);
}
