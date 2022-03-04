#include "ShaderHandler.h"

#include "ConstantBuffer.h"
#include "RenderDataManager.h"

//
// TODO: Split literally everything here to multiple files.
//

hh::mr::SShaderPair fxDeferredPassLightShader;
hh::mr::SShaderPair fxRLRShader;
hh::mr::SShaderPair fxDeferredPassIBLShader;
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

    This->m_pScheduler->GetShader(fxDeferredPassLightShader, "FxFilterPT", "FxDeferredPassLight");
    This->m_pScheduler->GetShader(fxRLRShader, "FxFilterPT", "FxRLR");

    This->m_pScheduler->GetShader(fxDeferredPassIBLShader, "FxFilterPT", "FxDeferredPassIBL");

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

    // Set parameters
    const bool overrideGIColor = SceneEffect::debug.giColorOverride.minCoeff() >= 0.0f;
    const bool giOnly = SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_GI_ONLY;
    const bool iblOnly = SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_IBL_ONLY;

    sceneEffectCB.reflectanceOverride = iblOnly ? 1.0f : std::min<float>(1.0f, SceneEffect::debug.reflectanceOverride),
    sceneEffectCB.roughnessOverride = iblOnly ? 0.0f : SceneEffect::debug.smoothnessOverride >= 0.0f ? std::max<float>(0.01f, 1 - SceneEffect::debug.smoothnessOverride) : -1.0f;
    sceneEffectCB.ambientOcclusionOverride = giOnly || iblOnly ? 1.0f : std::min<float>(1.0f, SceneEffect::debug.ambientOcclusionOverride);
    sceneEffectCB.metalnessOverride = giOnly || iblOnly ? 0.0f : std::min<float>(1.0f, SceneEffect::debug.metalnessOverride);
    sceneEffectCB.giColorOverride[0] = overrideGIColor ? SceneEffect::debug.giColorOverride.x() : -1.0f;
    sceneEffectCB.giColorOverride[1] = overrideGIColor ? SceneEffect::debug.giColorOverride.y() : -1.0f,
    sceneEffectCB.giColorOverride[2] = overrideGIColor ? SceneEffect::debug.giColorOverride.z() : -1.0f;
    sceneEffectCB.giShadowOverride = std::min<float>(1.0f, SceneEffect::debug.giShadowMapOverride);
    sceneEffectCB.rcpMiddleGray = 1.0f / (*(float*)0x1A572D0 + 0.001f);
    sceneEffectCB.sggiParam[1] = 1.0f / std::max<float>(0.001f, SceneEffect::sggi.startSmoothness - SceneEffect::sggi.endSmoothness);
    sceneEffectCB.sggiParam[0] = -(1.0f - SceneEffect::sggi.startSmoothness) * sceneEffectCB.sggiParam[1];
    sceneEffectCB.esmFactor = SceneEffect::esm.factor;
    sceneEffectCB.shadowMapSize[0] = (float)shadowMap->m_CreationParams.Width;
    sceneEffectCB.shadowMapSize[1] = 1.0f / (float)shadowMap->m_CreationParams.Width;
    sceneEffectCB.useWhiteAlbedo = SceneEffect::debug.useWhiteAlbedo || giOnly;
    sceneEffectCB.useFlatNormal = iblOnly || SceneEffect::debug.useFlatNormal;
    sceneEffectCB.usePBR = globalUsePBR;
    sceneEffectCB.upload(d3dDevice);

    if (!globalUsePBR)
        return originalCFxRenderGameSceneExecute(This);

    if (!blueNoisePicture)
        This->m_pScheduler->GetPicture(blueNoisePicture, "blue_noise");

    if (!envBrdfPicture)
        This->m_pScheduler->GetPicture(envBrdfPicture, "env_brdf");

    d3dDevice->SetTexture(16, blueNoisePicture ? blueNoisePicture->m_spPictureData->m_pD3DTexture : nullptr); // g_BlueNoiseTexture
    d3dDevice->SetTexture(17, luAvgTex ? luAvgTex->m_pD3DTexture : nullptr); // g_LuminanceTexture
    d3dDevice->SetTexture(20, envBrdfPicture && !giOnly ? envBrdfPicture->m_spPictureData->m_pD3DTexture : nullptr); // g_EnvBRDFTexture
    d3dDevice->SetTexture(26, RenderDataManager::defaultIBLPicture && !SceneEffect::debug.disableDefaultIBL
                                  ? RenderDataManager::defaultIBLPicture->m_spPictureData->m_pD3DTexture
                                  : nullptr); // g_DefaultIBLTexture

    device->SetSamplerFilter(11, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    device->SetSamplerAddressMode(11, D3DTADDRESS_CLAMP); // g_LinearClampSampler

    device->SetSamplerFilter(12, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(12, D3DTADDRESS_CLAMP); // g_PointClampSampler

    device->SetSamplerFilter(13, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(13, D3DTADDRESS_BORDER); // g_PointBorderSampler

    device->SetSamplerFilter(15, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(15, D3DTADDRESS_WRAP); // g_PointRepeatSampler

    // Set SHLFs in the frustum.
    for (size_t i = 0; i < 3; i++)
    {
        const SHLightFieldData* cache = !RenderDataManager::shlfsInFrustum.empty() ?
            RenderDataManager::shlfsInFrustum[std::min(i, RenderDataManager::shlfsInFrustum.size() - 1)] : nullptr;

        if (cache == nullptr || SceneEffect::debug.disableSHLightField)
            continue;

        d3dDevice->SetTexture(23 + i, cache->picture->m_spPictureData->m_pD3DTexture);

        memcpy(&renderDataCB.shLightFieldMatrices[i], cache->inverseMatrix.data(), sizeof(Eigen::Matrix34f));
        renderDataCB.shLightFieldParams[i].x() = (1.0f / cache->probeCounts[0]) * 0.5f;
        renderDataCB.shLightFieldParams[i].y() = (1.0f / cache->probeCounts[1]) * 0.5f;
        renderDataCB.shLightFieldParams[i].z() = (1.0f / cache->probeCounts[2]) * 0.5f;
    }

    // Set IBLs in the frustum.
    renderDataCB.iblProbeCount = std::min<size_t>(RenderDataManager::iblProbesInFrustum.size(),
        std::min<size_t>(Configuration::maxProbeCount, SceneEffect::debug.maxProbeCount));

    if (SceneEffect::debug.disableIBLProbe)
        renderDataCB.iblProbeCount = 0;

    for (size_t i = 0; i < renderDataCB.iblProbeCount; i++)
    {
        const IBLProbeData* cache = RenderDataManager::iblProbesInFrustum[i];

        memcpy(&renderDataCB.iblProbeMatrices[i], cache->inverseMatrix.data(), sizeof(Eigen::Matrix34f));
        memcpy(&renderDataCB.iblProbeParams[i], cache->position.data(), sizeof(Eigen::AlignedVector3f));
        renderDataCB.iblProbeParams[i].w() = (float)cache->pictureIndex;
        renderDataCB.iblProbeLodParams[i] = 3.0f;
    }

    renderDataCB.defaultIblLodParam = 3.0f;

    // Set local lights in the frustum.
    renderDataCB.localLightCount = std::min<size_t>(32, RenderDataManager::localLightsInFrustum.size());

    if (SceneEffect::debug.disableLocalLight || SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_GI_ONLY)
        renderDataCB.localLightCount = 0;

    for (size_t i = 0; i < renderDataCB.localLightCount; i++)
    {
        const LocalLightData* data = RenderDataManager::localLightsInFrustum[i];

        const float rangeSqr = data->range.w() * data->range.w();

        renderDataCB.localLightData[i * 8 + 0] = data->position.x();
        renderDataCB.localLightData[i * 8 + 1] = data->position.y();
        renderDataCB.localLightData[i * 8 + 2] = data->position.z();
        renderDataCB.localLightData[i * 8 + 3] = rangeSqr;
        renderDataCB.localLightData[i * 8 + 4] = data->color.x() / (float)(4 * M_PI);
        renderDataCB.localLightData[i * 8 + 5] = data->color.y() / (float)(4 * M_PI);
        renderDataCB.localLightData[i * 8 + 6] = data->color.z() / (float)(4 * M_PI);
        renderDataCB.localLightData[i * 8 + 7] = 1.0f / rangeSqr;
    }

    renderDataCB.upload(d3dDevice);

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

        This->RenderScene(hh::ygg::eRenderCategory_Sky, -1);
    }

    // Unlock render states we're done with.
    renderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ZENABLE);

    //************************************************************//
    // Pre-pass: Render opaque/punch-through objects and terrain. //
    //************************************************************//

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

    // We're rendering opaque meshes first, so disable alpha testing.
    renderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    renderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    // Render objects & player separately from terrain so it gets culled better.
    This->RenderScene(
        hh::ygg::eRenderCategory_Object | 
        hh::ygg::eRenderCategory_ObjectOverlay | hh::ygg::eRenderCategory_Player,
        hh::ygg::eRenderLevel_Opaque);

    This->RenderScene(hh::ygg::eRenderCategory_Terrain,
        hh::ygg::eRenderLevel_Opaque);

    // Done with D3DRS_ALPHATESTENABLE FALSE, unlock it.
    renderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Render punch-through meshes this time and enable alpha testing.
    renderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    renderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    This->RenderScene(
        hh::ygg::eRenderCategory_Object | 
        hh::ygg::eRenderCategory_ObjectOverlay | hh::ygg::eRenderCategory_Player,
        hh::ygg::eRenderLevel_PunchThrough);

    This->RenderScene(hh::ygg::eRenderCategory_Terrain,
        hh::ygg::eRenderLevel_PunchThrough);

    // Done with D3DRS_ALPHATESTENABLE TRUE, unlock it.
    renderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Done with D3DRS_ZENABLE/D3DRS_ZWRITEENABLE TRUE, unlock them.
    renderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);

    // We're done rendering opaque/punch-through terrain and objects!
    device->UnsetRenderTarget(0);
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

    device->SetTexture(0, This->m_spColorTex);
    device->SetTexture(1, gBuffer1Tex);
    device->SetTexture(2, gBuffer2Tex);
    device->SetTexture(3, gBuffer3Tex);
    device->SetTexture(11, This->m_spColorTex);
    device->SetTexture(12, This->m_spDepthTex);

    if (SceneEffect::ssao.enable)
    {
        boost::shared_ptr<hh::ygg::CYggSurface> ssaoSurface;
        ssaoTex->GetSurface(ssaoSurface, 0, 0);

        device->SetShader(fxSSAOShader);
        device->SetRenderTarget(0, ssaoSurface);

        filterCB.ssaoSampleCount = SceneEffect::ssao.sampleCount;
        filterCB.ssaoRcpSampleCount = 1.0f / (float)SceneEffect::ssao.sampleCount;
        filterCB.ssaoRadius = SceneEffect::ssao.radius;
        filterCB.ssaoDistanceFade = SceneEffect::ssao.distanceFade;
        filterCB.ssaoStrength = SceneEffect::ssao.strength;
        filterCB.upload(d3dDevice);

        device->DrawQuad2D(nullptr, 0, 0);

        // Apply blur
        boost::shared_ptr<hh::ygg::CYggSurface> ssaoBlurredSurface;
        ssaoBlurredTex->GetSurface(ssaoBlurredSurface, 0, 0);

        device->SetShader(fxBoxBlurShader);
        device->SetRenderTarget(0, ssaoBlurredSurface);
        d3dDevice->SetTexture(19, nullptr);

        filterCB.boxBlurSourceSize[0] = 1.0f / (float)ssaoTex->m_CreationParams.Width;
        filterCB.boxBlurSourceSize[1] = 1.0f / (float)ssaoTex->m_CreationParams.Height;
        filterCB.boxBlurDepthThreshold = SceneEffect::ssao.depthThreshold;
        filterCB.upload(d3dDevice);

        device->SetTexture(4, ssaoTex);
        device->DrawQuad2D(nullptr, 0, 0);
    }

    //***************************************************//
    // Deferred light pass: Add direct lighting and SHLF //
    // to meshes through deferred rendering.             //
    //***************************************************//

    // Set the shader.
    device->SetShader(fxDeferredPassLightShader);
    device->SetRenderTarget(0, gBuffer0Surface);

    device->SetTexture(7, shadowMapNoTerrain);
    device->SetTexture(13, shadowMap);

    if (SceneEffect::ssao.enable)
        d3dDevice->SetTexture(19, ssaoBlurredTex->m_pD3DTexture);

    filterCB.lightEnableSSAO = SceneEffect::ssao.enable;
    filterCB.upload(d3dDevice);

    if (SceneEffect::debug.disableDirectLight || SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_GI_ONLY)
    {
        float globalLightColor[] = { 0, 0, 0, 0 };
        d3dDevice->SetPixelShaderConstantF(36, globalLightColor, 1);
        d3dDevice->SetPixelShaderConstantF(37, globalLightColor, 1);
    }

    device->DrawQuad2D(nullptr, 0, 0);

    //*****************************//
    // Real-time Local Reflections //
    //*****************************//

    if (SceneEffect::rlr.enable && Configuration::rlrEnable)
    {
        boost::shared_ptr<hh::ygg::CYggSurface> rlrSurface;
        rlrTex->GetSurface(rlrSurface, 0, 0);

        device->SetShader(fxRLRShader);
        device->SetRenderTarget(0, rlrSurface);
        device->SetTexture(0, gBuffer0Tex);
        d3dDevice->SetTexture(21, nullptr);

        // Set parameters
        filterCB.rlrFramebufferSize[0] = (float)This->m_spColorTex->m_CreationParams.Width;
        filterCB.rlrFramebufferSize[1] = (float)This->m_spColorTex->m_CreationParams.Height;
        filterCB.rlrFramebufferSize[2] = 1.0f / (float)This->m_spColorTex->m_CreationParams.Width;
        filterCB.rlrFramebufferSize[3] = 1.0f / (float)This->m_spColorTex->m_CreationParams.Height;
        filterCB.rlrStepCount = SceneEffect::rlr.stepCount;
        filterCB.rlrMaxRoughness = SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_IBL_ONLY ? -1.0f : SceneEffect::rlr.maxRoughness;
        filterCB.rlrRayLength = SceneEffect::rlr.rayLength;
        filterCB.rlrFade = 1.0f / SceneEffect::rlr.fade;
        filterCB.rlrAccuracyThreshold = SceneEffect::rlr.accuracyThreshold;
        filterCB.rlrSaturation = std::min<float>(1.0f, std::max<float>(0.0f, SceneEffect::rlr.saturation));
        filterCB.rlrBrightness = SceneEffect::rlr.brightness;
        filterCB.upload(d3dDevice);

        device->DrawQuad2D(nullptr, 0, 0);
    }

    //****************************************************//
    // Deferred specular pass: Add specular lighting to   //
    // terrain and objects.                               //
    //****************************************************//

    device->SetShader(fxDeferredPassIBLShader);
    device->SetRenderTarget(0, This->m_spColorSurface);

    device->SetTexture(0, gBuffer0Tex);

    if (filterCB.iblEnableRLR)
        d3dDevice->SetTexture(21, rlrTex->m_pD3DTexture);

    filterCB.iblEnableSSAO = SceneEffect::ssao.enable;
    filterCB.iblEnableRLR = SceneEffect::rlr.enable && Configuration::rlrEnable;
    filterCB.upload(d3dDevice);

    device->DrawQuad2D(nullptr, 0, 0);

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

    device->SetShader(fxCopyColorDepthShader);
    device->SetRenderTarget(0, capturedColorSurface);
    device->SetDepthStencil(capturedDepthSurface);

    device->SetTexture(0, This->m_spColorTex);
    device->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    device->SetTexture(1, This->m_spDepthTex);
    device->SetSamplerFilter(1, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(1, D3DTADDRESS_CLAMP);

    device->DrawQuad2D(nullptr, 0, 0);

    //***************************************************************//
    // Forward rendering: Render transparent objects and water. //
    //***************************************************************//

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
    device->SetTexture(11, This->m_spCapturedColorTex);
    device->SetTexture(12, This->m_spCapturedDepthTex);

    // Transparent objects should do alpha blending.
    renderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    renderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    renderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

    // Terrain
    device->SetTexture(13, shadowMapNoTerrain);

    This->RenderScene(hh::ygg::eRenderCategory_Terrain, 
        hh::ygg::eRenderLevel_Transparent);

    // Objects
    device->SetTexture(13, shadowMap);
    device->SetSamplerFilter(13, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(13, D3DTADDRESS_BORDER);

    This->RenderScene(
        hh::ygg::eRenderCategory_Object | 
        hh::ygg::eRenderCategory_ObjectOverlay | hh::ygg::eRenderCategory_Player,
        hh::ygg::eRenderLevel_Transparent);

    // XLU Object
    renderingDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_SRCALPHA);
    renderingDevice->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);

    renderingDevice->LockRenderState(D3DRS_SRCBLENDALPHA);
    renderingDevice->LockRenderState(D3DRS_DESTBLENDALPHA);

    This->RenderScene(hh::ygg::eRenderCategory_ObjectXlu,
        hh::ygg::eRenderLevel_Opaque | hh::ygg::eRenderLevel_PunchThrough | hh::ygg::eRenderLevel_Transparent);

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
    This->RenderScene(hh::ygg::eRenderCategory_Terrain, hh::ygg::eRenderLevel_Water);

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

        This->RenderScene(hh::ygg::eRenderCategory_Effect | hh::ygg::eRenderCategory_SparkleObject | hh::ygg::eRenderCategory_SparkleFramebuffer,
            hh::ygg::eRenderLevel_Opaque | hh::ygg::eRenderLevel_PunchThrough | hh::ygg::eRenderLevel_Transparent);
    }

    //*********************//
    // Volumetric Lighting //
    //*********************//
    if (SceneEffect::volumetricLighting.enable || SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_VOLUMETRIC_LIGHTING)
    {
        renderingDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        renderingDevice->LockRenderState(D3DRS_ZENABLE);

        renderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        renderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

        renderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        renderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

        boost::shared_ptr<hh::ygg::CYggSurface> volumetricLightSurface;
        volumetricLightTex->GetSurface(volumetricLightSurface, 0, 0);

        device->SetShader(SceneEffect::volumetricLighting.ignoreSky ? fxVolumetricLightingIgnoreSkyShader : fxVolumetricLightingShader);
        device->UnsetDepthStencil();
        device->SetRenderTarget(0, volumetricLightSurface);

        filterCB.volumetricLightingSampleCount = SceneEffect::volumetricLighting.sampleCount;
        filterCB.volumetricLightingRcpSampleCount = 1.0f / (float)SceneEffect::volumetricLighting.sampleCount;
        filterCB.volumetricLightingG = SceneEffect::volumetricLighting.g;
        filterCB.volumetricLightingInScatteringScale = SceneEffect::volumetricLighting.inScatteringScale;
        filterCB.upload(d3dDevice);

        device->DrawQuad2D(nullptr, 0, 0);

        // Apply blur
        renderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
        renderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        renderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

        renderingDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
        renderingDevice->LockRenderState(D3DRS_SRCBLEND);

        renderingDevice->SetRenderState(D3DRS_DESTBLEND, SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_VOLUMETRIC_LIGHTING ? D3DBLEND_ZERO : D3DBLEND_ONE);
        renderingDevice->LockRenderState(D3DRS_DESTBLEND);

        device->SetRenderTarget(0, This->m_spColorSurface);
        device->SetShader(fxBoxBlurShader);

        device->SetTexture(4, volumetricLightTex);
        device->SetSamplerFilter(4, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
        device->SetSamplerAddressMode(4, D3DTADDRESS_CLAMP);

        device->SetTexture(12, This->m_spDepthTex);
        device->SetSamplerFilter(12, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
        device->SetSamplerAddressMode(12, D3DTADDRESS_CLAMP);

        filterCB.boxBlurSourceSize[0] = 1.0f / (float)volumetricLightTex->m_CreationParams.Width;
        filterCB.boxBlurSourceSize[1] = 1.0f / (float)volumetricLightTex->m_CreationParams.Height;
        filterCB.boxBlurDepthThreshold = SceneEffect::volumetricLighting.depthThreshold;
        filterCB.upload(d3dDevice);

        device->DrawQuad2D(nullptr, 0, 0);

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
    This->m_pScheduler->m_pMisc->m_pDevice->SetTexture(0, This->m_spLuAvgTex);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);
    This->m_pScheduler->m_pMisc->m_pDevice->DrawQuad2D(nullptr, 0, 0);
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