#include "ShaderHandler.h"
#include "RenderDataManager.h"

//
// TODO: Split literally everything here to multiple files.
//

std::array<hh::mr::SShaderPair, 1 + 32> fxDeferredPassLightShaders;
hh::mr::SShaderPair fxRLRShader;
std::array<hh::mr::SShaderPair, 1 + 8> fxDeferredPassIBLCombineShaders;
hh::mr::SShaderPair fxDeferredPassIBLProbeShader;
hh::mr::SShaderPair fxConvolutionFilterShader;

hh::mr::SShaderPair fxCopyColorShader;
hh::mr::SShaderPair fxCopyColorDepthShader;

hh::mr::SShaderPair fxSSAOShader;

hh::mr::SShaderPair fxVolumetricLightingShader;
hh::mr::SShaderPair fxVolumetricLightingIgnoreSkyShader;

hh::mr::SShaderPair fxBoxBlurShader;

boost::shared_ptr<hh::ygg::CYggTexture> luAvgTex;

boost::shared_ptr<hh::ygg::CYggTexture> gBuffer0Tex;
boost::shared_ptr<hh::ygg::CYggTexture> gBuffer1Tex;
boost::shared_ptr<hh::ygg::CYggTexture> gBuffer2Tex;
boost::shared_ptr<hh::ygg::CYggTexture> gBuffer3Tex;

boost::shared_ptr<hh::ygg::CYggTexture> rlrTex;
boost::shared_ptr<hh::ygg::CYggTexture> rlrTempTex;

boost::shared_ptr<hh::ygg::CYggTexture> prevIBLTex;

boost::shared_ptr<hh::ygg::CYggTexture> ssaoTex;
boost::shared_ptr<hh::ygg::CYggTexture> ssaoBlurredTex;

boost::shared_ptr<hh::ygg::CYggTexture> volumetricLightTex;

boost::shared_ptr<hh::ygg::CYggPicture> envBrdfPicture;
boost::shared_ptr<hh::ygg::CYggPicture> blueNoisePicture;

constexpr uint32_t CalculateMipCount(uint32_t width, uint32_t height)
{
    uint32_t mipLevels = 1;

    while (height > 1 || width > 1)
    {
        if (height > 1)
            height >>= 1;

        if (width > 1)
            width >>= 1;

        ++mipLevels;
    }

    return mipLevels;
}

HOOK(void, __fastcall, CFxRenderGameSceneInitialize, Sonic::fpCFxRenderGameSceneInitialize, Sonic::CFxRenderGameScene* This)
{
    originalCFxRenderGameSceneInitialize(This);

    for (size_t i = 0; i < 1 + 32; i++)
    {
        char name[32];
        sprintf(name, "FxDeferredPassLight_%d", i);

        This->m_pScheduler->GetShader(fxDeferredPassLightShaders[i], "FxFilterPT", name);
    }

    This->m_pScheduler->GetShader(fxRLRShader, "FxFilterPT", "FxRLR");

    for (size_t i = 0; i < 1 + 8; i++)
    {
        char name[32];
        sprintf(name, "FxDeferredPassIBLCombine_%d", i);

        This->m_pScheduler->GetShader(fxDeferredPassIBLCombineShaders[i], "FxFilterPT", name);
    }

    This->m_pScheduler->GetShader(fxDeferredPassIBLProbeShader, "FxFilterPT", "FxDeferredPassIBLProbe");

    This->m_pScheduler->GetShader(fxConvolutionFilterShader, "FxFilterT", "FxConvolutionFilter");

    This->m_pScheduler->GetShader(fxCopyColorShader, "FxFilterT", "FxCopyColor");
    This->m_pScheduler->GetShader(fxCopyColorDepthShader, "FxFilterT", "FxCopyColorDepth");

    This->m_pScheduler->GetShader(fxSSAOShader, "FxFilterPT", "FxSSAO");

    This->m_pScheduler->GetShader(fxVolumetricLightingShader, "FxFilterPT", "FxVolumetricLighting");
    This->m_pScheduler->GetShader(fxVolumetricLightingIgnoreSkyShader, "FxFilterPT", "FxVolumetricLighting_IgnoreSky");

    This->m_pScheduler->GetShader(fxBoxBlurShader, "FxFilterT", "FxBoxBlur");

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(luAvgTex, 1u, 1u, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R16F, D3DPOOL_DEFAULT, NULL);

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(gBuffer0Tex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(gBuffer1Tex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(gBuffer2Tex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(gBuffer3Tex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT, NULL);

    const uint32_t RLR_WIDTH = This->m_spColorTex->m_CreationParams.Width >> Configuration::rlrResolution;
    const uint32_t RLR_HEIGHT = This->m_spColorTex->m_CreationParams.Height >> Configuration::rlrResolution;

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(rlrTex, RLR_WIDTH, RLR_HEIGHT, CalculateMipCount(RLR_WIDTH, RLR_HEIGHT), D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(rlrTempTex, RLR_WIDTH, RLR_HEIGHT, CalculateMipCount(RLR_WIDTH, RLR_HEIGHT), D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(prevIBLTex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(ssaoTex, 0.5f, 0.5f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_L16, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(ssaoBlurredTex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_L16, D3DPOOL_DEFAULT, NULL);

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(volumetricLightTex, 0.5f, 0.5f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);

    luAvgTex->m_AutoReset = false;
    envBrdfPicture.reset();
    blueNoisePicture.reset();
}

HOOK(void, __fastcall, CFxRenderGameSceneExecute, Sonic::fpCFxRenderGameSceneExecute, Sonic::CFxRenderGameScene* This)
{
    // Get shadowmaps.
    boost::shared_ptr<hh::ygg::CYggTexture> shadowMapNoTerrain;
    This->GetTexture(shadowMapNoTerrain, "shadowmap_noterrain");

    boost::shared_ptr<hh::ygg::CYggTexture> shadowMap;
    This->GetTexture(shadowMap, "shadowmap");

    // Cache variables because it gets verbose if I don't.
    DX_PATCH::IDirect3DDevice9* d3dDevice = This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice;
    hh::mr::CRenderingDevice* renderingDevice = &This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice;
    hh::ygg::CYggDevice* device = This->m_pScheduler->m_pMisc->m_pDevice;
    Sonic::CFxSceneRenderer* sceneRenderer = (Sonic::CFxSceneRenderer*)This->m_pScheduler->m_pMisc->m_spSceneRenderer.get();

    // Set g_DebugParam
    const bool overrideGIColor = SceneEffect::debug.giColorOverride.minCoeff() >= 0.0f;
    const bool giOnly = SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_GI_ONLY;
    const bool iblOnly = SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_IBL_ONLY;

    float debugParam[] =
    {
        SceneEffect::debug.useWhiteAlbedo || giOnly ? 1.0f : -1.0f,
        iblOnly || SceneEffect::debug.useFlatNormal ? 1.0f : -1.0f,
        iblOnly ? 1.0f : std::min<float>(1.0f, SceneEffect::debug.reflectanceOverride),
        iblOnly ? 0.0f : SceneEffect::debug.smoothnessOverride >= 0.0f ? std::max<float>(0.01f, 1 - SceneEffect::debug.smoothnessOverride) : -1.0f,

        giOnly || iblOnly ? 1.0f : std::min<float>(1.0f, SceneEffect::debug.ambientOcclusionOverride),
        giOnly || iblOnly ? 0.0f : std::min<float>(1.0f, SceneEffect::debug.metalnessOverride),
        overrideGIColor ? SceneEffect::debug.giColorOverride.x() : -1.0f,
        overrideGIColor ? SceneEffect::debug.giColorOverride.y() : -1.0f,

        overrideGIColor ? SceneEffect::debug.giColorOverride.z() : -1.0f,
        std::min<float>(1.0f, SceneEffect::debug.giShadowMapOverride),
        0,
        0
    };
    d3dDevice->SetPixelShaderConstantF(106, debugParam, 3);

    // Set g_HDRParam_SGGIParam
    float sggiScale = 1.0f / std::max<float>(0.001f, SceneEffect::sggi.startSmoothness - SceneEffect::sggi.endSmoothness);
    float hdrParam_sggiParam[] = { 1.0f / (*(float*)0x1A572D0 + 0.001f), 0, -(1.0f - SceneEffect::sggi.startSmoothness) * sggiScale, sggiScale };
    d3dDevice->SetPixelShaderConstantF(109, hdrParam_sggiParam, 1);

    float esmParam[] = 
        { (float)shadowMap->m_CreationParams.Width, 1.0f / (float)shadowMap->m_CreationParams.Width, SceneEffect::esm.factor, 0 };

    d3dDevice->SetPixelShaderConstantF(110, esmParam, 1);

    // Set g_UsePBR
    BOOL usePBR[] = { globalUsePBR };
    d3dDevice->SetPixelShaderConstantB(6, usePBR, 1);

    if (!globalUsePBR)
    {
        // Unset mrgIsUseDeferred
        BOOL isUseDeferred[] = { false };
        d3dDevice->SetPixelShaderConstantB(7, isUseDeferred, 1);
        d3dDevice->SetVertexShaderConstantB(7, isUseDeferred, 1);

        return originalCFxRenderGameSceneExecute(This);
    }

    // Prepare and set render targets. Clear their contents.
    boost::shared_ptr<hh::ygg::CYggSurface> gBuffer0Surface;
    boost::shared_ptr<hh::ygg::CYggSurface> gBuffer1Surface;
    boost::shared_ptr<hh::ygg::CYggSurface> gBuffer2Surface;
    boost::shared_ptr<hh::ygg::CYggSurface> gBuffer3Surface;

    gBuffer0Tex->GetSurface(gBuffer0Surface, 0, 0);
    gBuffer1Tex->GetSurface(gBuffer1Surface, 0, 0);
    gBuffer2Tex->GetSurface(gBuffer2Surface, 0, 0);
    gBuffer3Tex->GetSurface(gBuffer3Surface, 0, 0);

    device->SetRenderTarget(0, This->m_spColorSurface);
    device->SetRenderTarget(1, gBuffer1Surface);
    device->SetRenderTarget(2, gBuffer2Surface);
    device->SetRenderTarget(3, gBuffer3Surface);
    device->SetDepthStencil(This->m_spDepthSurface);

    device->Clear({
        D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL,
        D3DCOLOR_ARGB(
            sceneRenderer->m_BackgroundColor[0], 
            sceneRenderer->m_BackgroundColor[1],
            sceneRenderer->m_BackgroundColor[2],
            sceneRenderer->m_BackgroundColor[3]
        ),
        1.0f,
        0
    });

    //***********************//
    // Pre-pass: Render sky. //
    //***********************//

    if (SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_NONE)
    {
        // Disable Z buffer.
        renderingDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        renderingDevice->LockRenderState(D3DRS_ZENABLE);

        // Disable alpha testing.
        renderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        renderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

        // Enable alpha blending.
        renderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        renderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

        This->RenderScene(hh::ygg::eRenderType_Sky, -1);
    }

    // Unlock render states we're done with.
    renderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ZENABLE);

    //************************************************************//
    // Pre-pass: Render opaque/punch-through objects and terrain. //
    //************************************************************//

    // Set g_LuminanceSampler
    device->SetSampler(8, luAvgTex);
    device->SetSamplerFilter(8, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(8, D3DTADDRESS_CLAMP);

    // Enable mrgIsUseDeferred so shaders output data to GBuffer render targets.
    BOOL isUseDeferred[] = { true };
    d3dDevice->SetPixelShaderConstantB(7, isUseDeferred, 1);
    d3dDevice->SetVertexShaderConstantB(7, isUseDeferred, 1);

    // Setup GI & occlusion flags.
    device->SetSamplerFilter(9, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    device->SetSamplerAddressMode(9, D3DTADDRESS_CLAMP);
    device->SetSamplerFilter(10, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    device->SetSamplerAddressMode(10, D3DTADDRESS_CLAMP);

    // Enable Z buffer.
    renderingDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    renderingDevice->LockRenderState(D3DRS_ZENABLE);

    renderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    renderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    renderingDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    renderingDevice->LockRenderState(D3DRS_ZFUNC);

    // Disable alpha blending since we're rendering opaque/punch-through meshes.
    renderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    renderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

    // Set Default IBL and Env BRDF
    if (!envBrdfPicture)
        This->m_pScheduler->GetPicture(envBrdfPicture, "env_brdf");

    device->SetSampler(14, RenderDataManager::defaultIBLPicture);
    device->SetSamplerFilter(14, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    device->SetSamplerAddressMode(14, D3DTADDRESS_CLAMP);

    device->SetSampler(15, envBrdfPicture);
    device->SetSamplerFilter(15, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    device->SetSamplerAddressMode(15, D3DTADDRESS_CLAMP);

    // We're rendering opaque meshes first, so disable alpha testing.
    renderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    renderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    // Render objects & player separately from terrain so it gets culled better.
    This->RenderScene(
        hh::ygg::eRenderType_Object | 
        hh::ygg::eRenderType_ObjectOverlay | hh::ygg::eRenderType_Player,
        hh::ygg::eRenderSlot_Opaque);

    This->RenderScene(hh::ygg::eRenderType_Terrain,
        hh::ygg::eRenderSlot_Opaque);

    // Done with D3DRS_ALPHATESTENABLE FALSE, unlock it.
    renderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Render punch-through meshes this time and enable alpha testing.
    renderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    renderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    This->RenderScene(
        hh::ygg::eRenderType_Object | 
        hh::ygg::eRenderType_ObjectOverlay | hh::ygg::eRenderType_Player,
        hh::ygg::eRenderSlot_PunchThrough);

    This->RenderScene(hh::ygg::eRenderType_Terrain,
        hh::ygg::eRenderSlot_PunchThrough);

    // Done with D3DRS_ALPHATESTENABLE TRUE, unlock it.
    renderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Done with D3DRS_ZENABLE/D3DRS_ZWRITEENABLE TRUE, unlock them.
    renderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);

    // We're done rendering opaque/punch-through terrain and objects!
    device->UnsetRenderTarget(1);
    device->UnsetRenderTarget(2);
    device->UnsetRenderTarget(3);
    device->UnsetDepthStencil();

    //***************//
    // Deferred pass //
    //***************//

    // Since we're going to be rendering to quads, disable Z buffer and alpha test entirely.
    renderingDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    renderingDevice->LockRenderState(D3DRS_ZENABLE);

    renderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    renderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    renderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    renderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    device->SetSampler(0, This->m_spColorTex);
    device->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    device->SetSampler(1, gBuffer1Tex);
    device->SetSamplerFilter(1, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(1, D3DTADDRESS_CLAMP);

    device->SetSampler(2, gBuffer2Tex);
    device->SetSamplerFilter(2, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(2, D3DTADDRESS_CLAMP);

    device->SetSampler(3, gBuffer3Tex);
    device->SetSamplerFilter(3, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(3, D3DTADDRESS_CLAMP);

    device->SetSampler(11, This->m_spColorTex);
    device->SetSamplerFilter(11, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(11, D3DTADDRESS_CLAMP);

    device->SetSampler(12, This->m_spDepthTex);
    device->SetSamplerFilter(12, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(12, D3DTADDRESS_CLAMP);

    if (SceneEffect::ssao.enable)
    {
        if (!blueNoisePicture)
            This->m_pScheduler->GetPicture(blueNoisePicture, "blue_noise");
        
        boost::shared_ptr<hh::ygg::CYggSurface> ssaoSurface;
        ssaoTex->GetSurface(ssaoSurface, 0, 0);

        float blueNoiseTileSize[] =
        {
            (float)ssaoTex->m_CreationParams.Width / 64,
            (float)ssaoTex->m_CreationParams.Height / 64,
            0,
            0
        };

        float sampleCount_invSampleCount_radius_distanceFade[] =
        {
            (float)SceneEffect::ssao.sampleCount,
            1.0f / (float)SceneEffect::ssao.sampleCount,
            SceneEffect::ssao.radius,
            SceneEffect::ssao.distanceFade
        };

        float strength[] =
        {
            SceneEffect::ssao.strength,
            0,
            0,
            0
        };

        d3dDevice->SetPixelShaderConstantF(150, blueNoiseTileSize, 1);
        d3dDevice->SetPixelShaderConstantF(151, sampleCount_invSampleCount_radius_distanceFade, 1);
        d3dDevice->SetPixelShaderConstantF(152, strength, 1);

        device->SetSamplerFilter(4, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
        device->SetSamplerAddressMode(4, D3DTADDRESS_WRAP);
        device->SetSampler(4, blueNoisePicture);

        device->SetShader(fxSSAOShader);

        device->SetRenderTarget(0, ssaoSurface);
        device->RenderQuad(nullptr, 0, 0);

        // Apply blur
        boost::shared_ptr<hh::ygg::CYggSurface> ssaoBlurredSurface;
        ssaoBlurredTex->GetSurface(ssaoBlurredSurface, 0, 0);

        float sourceSize_depthThreshold[] = 
        {
            1.0f / (float)ssaoTex->m_CreationParams.Width,
            1.0f / (float)ssaoTex->m_CreationParams.Height,
            SceneEffect::ssao.depthThreshold,
            0
        };

        d3dDevice->SetPixelShaderConstantF(150, sourceSize_depthThreshold, 1);

        device->SetRenderTarget(0, ssaoBlurredSurface);
        device->SetSampler(4, ssaoTex);
        device->SetSamplerAddressMode(4, D3DTADDRESS_CLAMP);
        device->SetShader(fxBoxBlurShader);
        device->RenderQuad(nullptr, 0, 0);
    }

    // Pass 32 omni lights from the view to shaders.
    size_t localLightCount = 0;

    float localLightData[8 * 32];
        
    localLightCount = std::min<size_t>(32, RenderDataManager::localLightsInFrustum.size());
    
    for (size_t i = 0; i < localLightCount; i++)
    {
        const LocalLightData* data = RenderDataManager::localLightsInFrustum[i];

        const float rangeSqr = data->range.w() * data->range.w();

        localLightData[i * 8 + 0] = data->position.x();
        localLightData[i * 8 + 1] = data->position.y();
        localLightData[i * 8 + 2] = data->position.z();
        localLightData[i * 8 + 3] = rangeSqr;
        localLightData[i * 8 + 4] = data->color.x() / (float)(4 * M_PI);
        localLightData[i * 8 + 5] = data->color.y() / (float)(4 * M_PI);
        localLightData[i * 8 + 6] = data->color.z() / (float)(4 * M_PI);
        localLightData[i * 8 + 7] = 1.0f / rangeSqr;
    }
    
    if (localLightCount > 0)
        d3dDevice->SetPixelShaderConstantF(111, (const float*)localLightData, 2 * localLightCount);

    if (SceneEffect::debug.disableLocalLight || SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_GI_ONLY)
        localLightCount = 0;

    //***************************************************//
    // Deferred light pass: Add direct lighting and SHLF //
    // to meshes through deferred rendering.             //
    //***************************************************//

    // Set the corresponding shader.
    device->SetShader(fxDeferredPassLightShaders[localLightCount]);

    // Set shadowmaps.
    device->SetSampler(7, shadowMapNoTerrain);
    device->SetSamplerFilter(7, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(7, D3DTADDRESS_BORDER);

    device->SetSampler(13, shadowMap);
    device->SetSamplerFilter(13, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(13, D3DTADDRESS_BORDER);

    // Set SHLFs in the frustum.

    auto shlfIterator = RenderDataManager::shlfsInFrustum.begin();

    for (size_t i = 0; i < 3; i++)
    {
        const SHLightFieldData* cache = !RenderDataManager::shlfsInFrustum.empty() ? 
            RenderDataManager::shlfsInFrustum[std::min(i, RenderDataManager::shlfsInFrustum.size() - 1)] : nullptr;

        if (cache == nullptr || SceneEffect::debug.disableSHLightField)
        {
            device->UnsetSampler(4 + i);
            continue;
        }

        device->SetSampler(4 + i, cache->picture);
        device->SetSamplerFilter(4 + i, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
        device->SetSamplerAddressMode(4 + i, D3DTADDRESS_CLAMP);

        d3dDevice->SetPixelShaderConstantF(175 + i * 3, cache->inverseMatrix.data(), 3);

        float shlfParam[] = { (1.0f / cache->probeCounts[0]) * 0.5f, (1.0f / cache->probeCounts[1]) * 0.5f, (1.0f / cache->probeCounts[2]) * 0.5f, 0 };
        d3dDevice->SetPixelShaderConstantF(184 + i, shlfParam, 1);
    }

    // Set SSAO.
    BOOL enableSSAO[] = { SceneEffect::ssao.enable };
    d3dDevice->SetPixelShaderConstantB(7, enableSSAO, 1);

    if (SceneEffect::ssao.enable)
    {
        float ssaoSize[] =
        {
            (float)ssaoBlurredTex->m_CreationParams.Width,
            (float)ssaoBlurredTex->m_CreationParams.Height,

            1.0f / (float)ssaoBlurredTex->m_CreationParams.Width,
            1.0f / (float)ssaoBlurredTex->m_CreationParams.Height,
        };

        d3dDevice->SetPixelShaderConstantF(187, ssaoSize, 1);

        device->SetSampler(8, ssaoBlurredTex);
        device->SetSamplerFilter(8, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
        device->SetSamplerAddressMode(8, D3DTADDRESS_CLAMP);

        device->SetRenderTarget(1, gBuffer2Surface); // To update AO in the GBuffer
    }

    if (SceneEffect::debug.disableDirectLight || SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_GI_ONLY)
    {
        float globalLightColor[] = { 0, 0, 0, 0 };
        d3dDevice->SetPixelShaderConstantF(36, globalLightColor, 1);
        d3dDevice->SetPixelShaderConstantF(37, globalLightColor, 1);
    }

    device->SetRenderTarget(0, gBuffer0Surface);
    device->RenderQuad(nullptr, 0, 0);

    if (SceneEffect::ssao.enable)
        device->UnsetRenderTarget(1);

    //*****************************//
    // Real-time Local Reflections //
    //*****************************//

    if (SceneEffect::rlr.enable && Configuration::rlrEnable)
    {
        boost::shared_ptr<hh::ygg::CYggSurface> rlrSurface;
        rlrTex->GetSurface(rlrSurface, 0, 0);

        device->SetShader(fxRLRShader);
        device->SetRenderTarget(0, rlrSurface);
        device->Clear(D3DCLEAR_TARGET, 0, 1.0f, 0);

        device->SetSampler(0, gBuffer0Tex);
        device->SetSamplerFilter(0, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);

        // Set parameters
        float framebufferSize[] = {
            (float)This->m_spColorTex->m_CreationParams.Width,
            (float)This->m_spColorTex->m_CreationParams.Height,
            1.0f / (float)This->m_spColorTex->m_CreationParams.Width,
            1.0f / (float)This->m_spColorTex->m_CreationParams.Height,
        };

        float stepCount_maxRoughness_rayLength_fade[] = {
            (float)SceneEffect::rlr.stepCount,
            SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_IBL_ONLY ? -1.0f : SceneEffect::rlr.maxRoughness,
            SceneEffect::rlr.rayLength,
            1.0f / SceneEffect::rlr.fade };

        float accuracyThreshold_saturation_brightness[] = {
            SceneEffect::rlr.accuracyThreshold,
            std::min<float>(1.0f, std::max<float>(0.0f, SceneEffect::rlr.saturation)),
            SceneEffect::rlr.brightness,
            0 };

        d3dDevice->SetPixelShaderConstantF(150, framebufferSize, 1);
        d3dDevice->SetPixelShaderConstantF(151, stepCount_maxRoughness_rayLength_fade, 1);
        d3dDevice->SetPixelShaderConstantF(152, accuracyThreshold_saturation_brightness, 1);

        device->RenderQuad(nullptr, 0, 0);

        // Apply gaussian blur to it because yes.
        device->SetShader(fxConvolutionFilterShader);

        device->SetSamplerFilter(0, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_POINT);
        device->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

        for (uint32_t i = 1; i < rlrTex->m_CreationParams.Levels; i++)
        {
            boost::shared_ptr<hh::ygg::CYggSurface> srcSurface, dstSurface, dstSurfaceTemp;

            rlrTex->GetSurface(srcSurface, i - 1, 0);
            rlrTex->GetSurface(dstSurface, i, 0);
            rlrTempTex->GetSurface(dstSurfaceTemp, i, 0);

            D3DSURFACE_DESC desc;
            dstSurface->m_pD3DSurface->GetDesc(&desc);

            // Downscale first.
            d3dDevice->StretchRect(srcSurface->m_pD3DSurface, nullptr, dstSurface->m_pD3DSurface, nullptr, D3DTEXF_LINEAR);

            // Now apply gaussian filter to it.
            const float param[] = { 1.0f / desc.Width, 1.0f / desc.Height, exp2f((float)i), (float)i };
            d3dDevice->SetPixelShaderConstantF(150, param, 1);

            // Horizontal
            device->SetRenderTarget(0, dstSurfaceTemp);

            float direction[] = { -1, 0, 0, 0 };
            d3dDevice->SetPixelShaderConstantF(151, direction, 1);

            device->SetSampler(0, rlrTex);

            device->RenderQuad(nullptr, 0, 0);

            // Vertical
            device->SetRenderTarget(0, dstSurface);

            direction[0] = 0;
            direction[1] = -1;
            d3dDevice->SetPixelShaderConstantF(151, direction, 1);

            device->SetSampler(0, rlrTempTex);

            device->RenderQuad(nullptr, 0, 0);
        }

        // Revert sampler filters.
        device->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    }

    device->SetSampler(0, gBuffer0Tex);

    //****************************************************//
    // Deferred specular pass: Add specular lighting to   //
    // terrain and objects.                               //
    //****************************************************//

    // Set probes in the frustum.
    // Process every 8 probes in a shader where no indirect lighting computation is applied.
    // Process remaining probes and raw IBL framebuffer in the main shader.

    size_t currProbeIndex = 0;

    size_t iblCountInFrustum = std::min<size_t>(RenderDataManager::iblProbesInFrustum.size(), 
        std::min<size_t>(Configuration::maxProbeCount, SceneEffect::debug.maxProbeCount));

    if (SceneEffect::debug.disableIBLProbe)
        iblCountInFrustum = 0;

    const int32_t iblProbePassCount = std::max<int32_t>(0, ((int32_t)iblCountInFrustum - 1) / 8);

    // Set Default IBL and Env BRDF
    device->SetSampler(14, RenderDataManager::defaultIBLPicture);
    device->SetSamplerFilter(14, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    device->SetSamplerAddressMode(14, D3DTADDRESS_CLAMP);

    device->SetSampler(15, envBrdfPicture);
    device->SetSamplerFilter(15, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    device->SetSamplerAddressMode(15, D3DTADDRESS_CLAMP);

    if (SceneEffect::debug.disableDefaultIBL)
        device->UnsetSampler(14);

    if (SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_GI_ONLY)
        device->UnsetSampler(15);

    // Initialize IBL parameter.
    float iblLodParam[] = { 0, 0, 0, 0 };

    if (RenderDataManager::defaultIBLPicture &&
        RenderDataManager::defaultIBLPicture->m_spPictureData &&
        RenderDataManager::defaultIBLPicture->m_spPictureData->m_pD3DTexture)
    {
        iblLodParam[0] = (float)RenderDataManager::defaultIBLPicture->m_spPictureData->m_pD3DTexture->GetLevelCount();
    }

    iblLodParam[1] = (float)(SceneEffect::rlr.maxLod >= 0 ?
        std::min<int32_t>(SceneEffect::rlr.maxLod, rlrTex->m_CreationParams.Levels) : rlrTex->m_CreationParams.Levels);

    d3dDevice->SetPixelShaderConstantF(145, iblLodParam, 1);

    boost::shared_ptr<hh::ygg::CYggSurface> prevIBLSurface;
    prevIBLTex->GetSurface(prevIBLSurface, 0, 0);

    for (int32_t i = 0; i < iblProbePassCount + 1; i++)
    {
        const bool isCombinePass = i == iblProbePassCount;
        const size_t iblCount = isCombinePass ? iblCountInFrustum - iblProbePassCount * 8 : 8;

        float probeParams[32];
        float probeLodParams[8];

        for (size_t j = 0; j < iblCount; j++)
        {
            const IBLProbeData* cache = RenderDataManager::iblProbesInFrustum[currProbeIndex++];

            d3dDevice->SetPixelShaderConstantF(111 + j * 3, cache->inverseMatrix.data(), 3);

            probeParams[j * 4 + 0] = cache->position.x();
            probeParams[j * 4 + 1] = cache->position.y();
            probeParams[j * 4 + 2] = cache->position.z();
            probeParams[j * 4 + 3] = cache->bias;

            if (cache->picture && cache->picture->m_spPictureData && cache->picture->m_spPictureData->m_pD3DTexture)
                probeLodParams[j] = (float)cache->picture->m_spPictureData->m_pD3DTexture->GetLevelCount();
            else
                probeLodParams[j] = 0.0f;

            device->SetSampler(4 + j, cache->picture);
            device->SetSamplerFilter(4 + j, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
            device->SetSamplerAddressMode(4 + j, D3DTADDRESS_CLAMP);
        }

        // Set the corresponding shader.
        device->SetShader(isCombinePass ? fxDeferredPassIBLCombineShaders[iblCount] : fxDeferredPassIBLProbeShader);

        if (iblCount > 0)
        {
            d3dDevice->SetPixelShaderConstantF(135, probeParams, iblCount);
            d3dDevice->SetPixelShaderConstantF(143, probeLodParams, (iblCount + 3) / 4);
        }

        BOOL isEnablePrevIBL[] = { true };

        if (i == 0)
        {
            if (SceneEffect::rlr.enable && Configuration::rlrEnable)
            {
                device->SetSampler(13, rlrTex);
                device->SetSamplerFilter(13, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
                device->SetSamplerAddressMode(13, D3DTADDRESS_CLAMP);
            }
            else
            {
                isEnablePrevIBL[0] = false;
            }
        }

        else
        {
            device->SetSampler(13, prevIBLTex);
            device->SetSamplerFilter(13, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
            device->SetSamplerAddressMode(13, D3DTADDRESS_CLAMP);
        }

        // Set g_IsEnablePrevIBL
        // Looks like setting sampler to null returns 0, 0, 0, 1 instead of 0, 0, 0, 0
        d3dDevice->SetPixelShaderConstantB(7, isEnablePrevIBL, 1);

        // Set render target.
        device->SetRenderTarget(0, isCombinePass ? This->m_spColorSurface : prevIBLSurface);
        device->RenderQuad(nullptr, 0, 0);
    }

    //*********//
    // Capture //
    //*********//

    // Enable Z writing, but always do it.
    renderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    renderingDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    renderingDevice->LockRenderState(D3DRS_ZENABLE);

    renderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);
    renderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    renderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    renderingDevice->UnlockRenderState(D3DRS_ZFUNC);
    renderingDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    renderingDevice->LockRenderState(D3DRS_ZFUNC);

    // Capture the current view to use in transparent objects and water
    boost::shared_ptr<hh::ygg::CYggSurface> capturedColorSurface;
    This->m_spCapturedColorTex->GetSurface(capturedColorSurface, 0, 0);

    boost::shared_ptr<hh::ygg::CYggSurface> capturedDepthSurface;
    This->m_spCapturedDepthTex->GetSurface(capturedDepthSurface, 0, 0);

    device->SetRenderTarget(0, capturedColorSurface);
    device->SetDepthStencil(capturedDepthSurface);

    device->SetShader(fxCopyColorDepthShader);

    device->SetSampler(0, This->m_spColorTex);
    device->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    device->SetSampler(1, This->m_spDepthTex);
    device->SetSamplerFilter(1, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(1, D3DTADDRESS_CLAMP);

    device->RenderQuad(nullptr, 0, 0);

    //***************************************************************//
    // Forward rendering: Render transparent objects and water. //
    //***************************************************************//

    // We're doing forward rendering now. Set the deferred bool to false.
    isUseDeferred[0] = false;
    d3dDevice->SetPixelShaderConstantB(7, isUseDeferred, 1);
    d3dDevice->SetVertexShaderConstantB(7, isUseDeferred, 1);

    // Set g_LuminanceSampler
    device->SetSampler(8, luAvgTex);
    device->SetSamplerFilter(8, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(8, D3DTADDRESS_CLAMP);

    // Setup GI & occlusion flags.
    device->SetSamplerFilter(9, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    device->SetSamplerAddressMode(9, D3DTADDRESS_CLAMP);
    device->SetSamplerFilter(10, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    device->SetSamplerAddressMode(10, D3DTADDRESS_CLAMP);

    device->SetRenderTarget(0, This->m_spColorSurface);
    device->SetDepthStencil(This->m_spDepthSurface);

    // We don't want transparent objects and water to write to Z buffer. It should do comparisons, though.
    renderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);
    renderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    renderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    renderingDevice->UnlockRenderState(D3DRS_ZFUNC);
    renderingDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    renderingDevice->LockRenderState(D3DRS_ZFUNC);

    // Set the framebuffer and depth samplers.
    device->SetSampler(11, This->m_spCapturedColorTex);
    device->SetSamplerFilter(11, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(11, D3DTADDRESS_CLAMP);

    device->SetSampler(12, This->m_spCapturedDepthTex);
    device->SetSamplerFilter(12, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(12, D3DTADDRESS_CLAMP);

    // Transparent objects should do alpha blending.
    renderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    renderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    renderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

    // Terrain
    device->SetSampler(13, shadowMapNoTerrain);

    This->RenderScene(hh::ygg::eRenderType_Terrain, 
        hh::ygg::eRenderSlot_Transparent);

    // Objects
    device->SetSampler(13, shadowMap);
    device->SetSamplerFilter(13, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(13, D3DTADDRESS_BORDER);

    This->RenderScene(
        hh::ygg::eRenderType_Object | 
        hh::ygg::eRenderType_ObjectOverlay | hh::ygg::eRenderType_Player,
        hh::ygg::eRenderSlot_Transparent);

    // XLU Object
    renderingDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_SRCALPHA);
    renderingDevice->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);

    renderingDevice->LockRenderState(D3DRS_SRCBLENDALPHA);
    renderingDevice->LockRenderState(D3DRS_DESTBLENDALPHA);

    This->RenderScene(hh::ygg::eRenderType_ObjectXlu,
        hh::ygg::eRenderSlot_Opaque | hh::ygg::eRenderSlot_PunchThrough | hh::ygg::eRenderSlot_Transparent);

    renderingDevice->UnlockRenderState(D3DRS_SRCBLENDALPHA);
    renderingDevice->UnlockRenderState(D3DRS_DESTBLENDALPHA);

    // Done with transparency, revert render states.
    renderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    renderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    renderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

    //*******//
    // Water //
    //*******//

    // Water doesn't need alpha blending so we keep it false.
    This->RenderScene(hh::ygg::eRenderType_Terrain, hh::ygg::eRenderSlot_Water);

    // Unlock render states that we're done with.
    renderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ZFUNC);

    //***********//
    // Particles //
    //***********//

    if (SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_NONE)
    {
        renderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        renderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        renderingDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
        renderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

        This->RenderScene(hh::ygg::eRenderType_Effect | hh::ygg::eRenderType_SparkleObject | hh::ygg::eRenderType_SparkleFramebuffer,
            hh::ygg::eRenderSlot_Opaque | hh::ygg::eRenderSlot_PunchThrough | hh::ygg::eRenderSlot_Transparent);
    }

    //*********************//
    // Volumetric Lighting //
    //*********************//
    if (SceneEffect::volumetricLighting.enable || SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_VOLUMETRIC_LIGHTING)
    {
        if (!blueNoisePicture)
            This->m_pScheduler->GetPicture(blueNoisePicture, "blue_noise");

        renderingDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        renderingDevice->LockRenderState(D3DRS_ZENABLE);

        renderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        renderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

        renderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        renderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

        float sampleCount_invSampleCount_g_inScatteringScale[] =
        {
            (float)SceneEffect::volumetricLighting.sampleCount,
            1.0f / (float)SceneEffect::volumetricLighting.sampleCount,
            SceneEffect::volumetricLighting.g,
            SceneEffect::volumetricLighting.inScatteringScale
        };

        d3dDevice->SetPixelShaderConstantF(150, sampleCount_invSampleCount_g_inScatteringScale, 1);

        device->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
        device->SetSamplerAddressMode(0, D3DTADDRESS_WRAP);
        device->SetSampler(0, blueNoisePicture);

        boost::shared_ptr<hh::ygg::CYggSurface> volumetricLightSurface;
        volumetricLightTex->GetSurface(volumetricLightSurface, 0, 0);

        device->UnsetDepthStencil();
        device->SetRenderTarget(0, volumetricLightSurface);
        device->SetShader(SceneEffect::volumetricLighting.ignoreSky ? fxVolumetricLightingIgnoreSkyShader : fxVolumetricLightingShader);
        device->RenderQuad(nullptr, 0, 0);

        // Apply blur
        renderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
        renderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        renderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

        renderingDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
        renderingDevice->LockRenderState(D3DRS_SRCBLEND);

        renderingDevice->SetRenderState(D3DRS_DESTBLEND, SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_VOLUMETRIC_LIGHTING ? D3DBLEND_ZERO : D3DBLEND_ONE);
        renderingDevice->LockRenderState(D3DRS_DESTBLEND);

        float sourceSize_depthThreshold[] =
        {
            1.0f / (float)volumetricLightTex->m_CreationParams.Width,
            1.0f / (float)volumetricLightTex->m_CreationParams.Height,
            SceneEffect::volumetricLighting.depthThreshold,
            0
        };

        d3dDevice->SetPixelShaderConstantF(150, sourceSize_depthThreshold, 1);

        device->SetSampler(4, volumetricLightTex);
        device->SetSamplerFilter(4, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
        device->SetSamplerAddressMode(4, D3DTADDRESS_CLAMP);       

        device->SetSampler(12, This->m_spDepthTex);
        device->SetSamplerFilter(12, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
        device->SetSamplerAddressMode(12, D3DTADDRESS_CLAMP);

        device->SetRenderTarget(0, This->m_spColorSurface);
        device->SetShader(fxBoxBlurShader);
        device->RenderQuad(nullptr, 0, 0);

        // Unlock render states we are done with.
        renderingDevice->UnlockRenderState(D3DRS_DESTBLEND);
        renderingDevice->UnlockRenderState(D3DRS_SRCBLEND);
        renderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
        renderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);
        renderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    }

    boost::shared_ptr<hh::ygg::CYggTexture> colorTex;
    boost::shared_ptr<hh::ygg::CYggTexture> capturedColorTex;

    switch (SceneEffect::debug.viewMode)
    {
    case DEBUG_VIEW_MODE_GBUFFER1:
        colorTex = gBuffer1Tex;
        break;

    case DEBUG_VIEW_MODE_GBUFFER2:
        colorTex = gBuffer2Tex;
        break;

    case DEBUG_VIEW_MODE_GBUFFER3:
        colorTex = gBuffer3Tex;
        break;

    case DEBUG_VIEW_MODE_RLR:
        colorTex = rlrTex;
        break;

    case DEBUG_VIEW_MODE_SSAO:
        colorTex = ssaoBlurredTex;
        break;

    case DEBUG_VIEW_MODE_SHADOW_MAP:
        colorTex = shadowMap;
        break;

    case DEBUG_VIEW_MODE_SHADOW_MAP_NO_TERRAIN:
        colorTex = shadowMapNoTerrain;
        break;

    case DEBUG_VIEW_MODE_NONE:
    case DEBUG_VIEW_MODE_GI_ONLY:
    case DEBUG_VIEW_MODE_VOLUMETRIC_LIGHTING:
    default:
        colorTex = This->m_spColorTex;
        capturedColorTex = This->m_spCapturedColorTex;
        break;
    }

    This->SetDefaultTexture(colorTex);
    This->SetBuffer("colortex", colorTex);
    This->SetBuffer("captured_colortex", capturedColorTex == nullptr ? colorTex : capturedColorTex);
    This->SetBuffer("depthtex", This->m_spDepthTex);
    This->SetBuffer("captured_depthtex", This->m_spCapturedDepthTex);
}

HOOK(void, __fastcall, CRenderingDeviceSetViewMatrix, hh::mr::fpCRenderingDeviceSetViewMatrix,
    hh::mr::CRenderingDevice* This, void* Edx, const Eigen::Matrix4f& viewMatrix)
{
    const Eigen::Matrix4f inverseViewMatrix = viewMatrix.inverse();
    This->m_pD3DDevice->SetPixelShaderConstantF(94, inverseViewMatrix.data(), 4);

    originalCRenderingDeviceSetViewMatrix(This, Edx, viewMatrix);
}

HOOK(void, __fastcall, CFxToneMappingExecute, Sonic::fpCFxToneMappingExecute, Sonic::CFxToneMapping* This)
{
    originalCFxToneMappingExecute(This);

    if (!globalUsePBR)
        return;

    boost::shared_ptr<hh::ygg::CYggSurface> surface;
    luAvgTex->GetSurface(surface, 0, 0);

    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, surface);
    This->m_pScheduler->m_pMisc->m_pDevice->UnsetDepthStencil();
    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(fxCopyColorShader.m_spVertexShader, fxCopyColorShader.m_spPixelShader);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSampler(0, This->m_spLuAvgTex);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);
    This->m_pScheduler->m_pMisc->m_pDevice->RenderQuad(nullptr, 0, 0);
}

HOOK(void, __fastcall, CGameplayFlowStageOnExit, 0xD05360, void* This)
{
    globalUsePBR = false;
    originalCGameplayFlowStageOnExit(This);
}

bool ShaderHandler::enabled = false;

void ShaderHandler::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CFxRenderGameSceneInitialize);
    INSTALL_HOOK(CFxRenderGameSceneExecute);
    INSTALL_HOOK(CRenderingDeviceSetViewMatrix);
    INSTALL_HOOK(CFxToneMappingExecute);
    INSTALL_HOOK(CGameplayFlowStageOnExit);
}