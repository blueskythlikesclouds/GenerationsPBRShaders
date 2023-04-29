#include "ShaderHandler.h"

#include "ConstantBuffer.h"
#include "HBAOPlusHandler.h"
#include "RenderDataManager.h"
#include "Utilities.h"

//
// TODO: Split literally everything here to multiple files.
//

hh::mr::SShaderPair fxDeferredPassLightShader;
hh::mr::SShaderPair fxRLRShader;
hh::mr::SShaderPair fxDeferredPassIBLShader;
hh::mr::SShaderPair fxConvolutionFilterShader;

hh::mr::SShaderPair fxCopyColorShader;
hh::mr::SShaderPair fxCopyColorDepthShader;

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

boost::shared_ptr<hh::ygg::CYggTexture> ssaoTex;

boost::shared_ptr<hh::ygg::CYggTexture> volumetricLightTex;

boost::shared_ptr<hh::ygg::CYggPicture> envBrdfPicture;
boost::shared_ptr<hh::ygg::CYggPicture> blueNoisePicture;

HOOK(void, __fastcall, CFxRenderGameSceneInitialize, Sonic::fpCFxRenderGameSceneInitialize, Sonic::CFxRenderGameScene* This)
{
    originalCFxRenderGameSceneInitialize(This);

    fxDeferredPassLightShader = This->m_pScheduler->GetShader("FxFilterPT", "FxDeferredPassLight");
    fxRLRShader = This->m_pScheduler->GetShader("FxFilterPT", "FxRLR");

    fxDeferredPassIBLShader = This->m_pScheduler->GetShader("FxFilterPT", "FxDeferredPassIBL");

    fxConvolutionFilterShader = This->m_pScheduler->GetShader("FxFilterT", "FxConvolutionFilter");

    fxCopyColorShader = This->m_pScheduler->GetShader("FxFilterT", "FxCopyColor");
    fxCopyColorDepthShader = This->m_pScheduler->GetShader("FxFilterT", "FxCopyColorDepth");

    fxVolumetricLightingShader = This->m_pScheduler->GetShader("FxFilterPT", "FxVolumetricLighting");
    fxVolumetricLightingIgnoreSkyShader = This->m_pScheduler->GetShader("FxFilterPT", "FxVolumetricLighting_IgnoreSky");

    fxBoxBlurShader = This->m_pScheduler->GetShader("FxFilterT", "FxBoxBlur");

    luAvgTex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(1u, 1u, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R16F, D3DPOOL_DEFAULT, NULL);

    gBuffer0Tex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);
    gBuffer1Tex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A2B10G10R10, D3DPOOL_DEFAULT, NULL);
    gBuffer2Tex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, NULL);
    gBuffer3Tex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, NULL);

    const uint32_t RLR_WIDTH = This->m_spColorTex->m_CreationParams.Width >> Configuration::rlrResolution;
    const uint32_t RLR_HEIGHT = This->m_spColorTex->m_CreationParams.Height >> Configuration::rlrResolution;
    const uint32_t RLR_LEVELS = Utilities::computeMipLevels(RLR_WIDTH, RLR_HEIGHT);

    rlrTex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(RLR_WIDTH, RLR_HEIGHT, RLR_LEVELS, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);
    rlrTempTex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(RLR_WIDTH, RLR_HEIGHT, RLR_LEVELS, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);

    ssaoTex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_L16, D3DPOOL_DEFAULT, NULL);

    volumetricLightTex = This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(0.5f, 0.5f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);

    luAvgTex->m_AutoReset = false;
    envBrdfPicture.reset();
    blueNoisePicture.reset();
}

HOOK(void, __fastcall, CFxRenderGameSceneExecute, Sonic::fpCFxRenderGameSceneExecute, Sonic::CFxRenderGameScene* This)
{
    // Get shadowmaps.
    const auto shadowMapNoTerrain = This->GetTexture(SceneEffect::esm.renderTerrain ? "shadowmap" : "shadowmap_noterrain");
    const auto shadowMap = This->GetTexture("shadowmap");

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
    sceneEffectCB.defaultIBLIntensity = SceneEffect::ibl.defaultIBLIntensity;
    sceneEffectCB.upload(d3dDevice);

    if (!globalUsePBR)
        return originalCFxRenderGameSceneExecute(This);

    if (!blueNoisePicture)
        blueNoisePicture = This->m_pScheduler->GetPicture("blue_noise");

    if (!envBrdfPicture)
        envBrdfPicture = This->m_pScheduler->GetPicture("env_brdf");

    d3dDevice->SetTexture(16, blueNoisePicture ? blueNoisePicture->m_spPictureData->m_pD3DTexture : nullptr); // g_BlueNoiseTexture
    d3dDevice->SetTexture(17, luAvgTex ? luAvgTex->m_pD3DTexture : nullptr); // g_LuminanceTexture
    d3dDevice->SetTexture(20, envBrdfPicture && !giOnly ? envBrdfPicture->m_spPictureData->m_pD3DTexture : nullptr); // g_EnvBRDFTexture
    d3dDevice->SetTexture(26, RenderDataManager::defaultIBLPicture && !SceneEffect::debug.disableDefaultIBL
                                  ? RenderDataManager::defaultIBLPicture->m_spPictureData->m_pD3DTexture
                                  : nullptr); // g_DefaultIBLTexture

    device->SetSamplerFilter(11, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    device->SetSamplerAddressMode(11, D3DTADDRESS_CLAMP); // g_LinearClampSampler

    device->SetSamplerFilter(12, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_POINT);
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
        std::min<size_t>(Configuration::maxProbeCount, SceneEffect::ibl.maxIBLProbeCount));

    if (SceneEffect::debug.disableIBLProbe)
        renderDataCB.iblProbeCount = 0;

    for (size_t i = 0; i < renderDataCB.iblProbeCount; i++)
    {
        const IBLProbeData* cache = RenderDataManager::iblProbesInFrustum[i];

        memcpy(&renderDataCB.iblProbeMatrices[i], cache->inverseMatrix.data(), sizeof(Eigen::Matrix34f));
        memcpy(&renderDataCB.iblProbeParams[i], cache->position.data(), sizeof(Eigen::AlignedVector3f));
        renderDataCB.iblProbeParams[i].w() = (float)cache->pictureIndex;
    }

    renderDataCB.iblProbeLodParam = (float)(RenderDataManager::iblProbesMipLevels - 1);
    renderDataCB.defaultIblLodParam = (float)(RenderDataManager::defaultIblMipLevels - 1);
    renderDataCB.defaultIblExposurePacked = RenderDataManager::defaultIblExposurePacked;

    GenerationsD3D11::LockDevice(d3dDevice);
    GenerationsD3D11::GetDeviceContext(d3dDevice)->PSSetShaderResources(27, 1, RenderDataManager::iblProbesSRV.GetAddressOf());
    GenerationsD3D11::UnlockDevice(d3dDevice);

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

    // Prepare heightmap.
    if (RenderDataManager::heightMapPicture &&
        RenderDataManager::heightMapPicture->m_spPictureData &&
        RenderDataManager::heightMapPicture->m_spPictureData->IsMadeAll())
    {
        static ComPtr<ID3D11SamplerState> samplerState;
        if (samplerState == nullptr)
        {
            CD3D11_SAMPLER_DESC samplerDesc;
            GenerationsD3D11::GetDevice(d3dDevice)->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());
        }

        if (!RenderDataManager::heightMapSRV)
        {
            GenerationsD3D11::GetDevice(d3dDevice)->CreateShaderResourceView(
                GenerationsD3D11::GetResource(RenderDataManager::heightMapPicture->m_spPictureData->m_pD3DTexture),
                nullptr,
                RenderDataManager::heightMapSRV.ReleaseAndGetAddressOf());
        }

        GenerationsD3D11::LockDevice(d3dDevice);
        GenerationsD3D11::GetDeviceContext(d3dDevice)->VSSetShaderResources(0, 1, RenderDataManager::heightMapSRV.GetAddressOf());
        GenerationsD3D11::GetDeviceContext(d3dDevice)->VSSetSamplers(0, 1, samplerState.GetAddressOf());
        GenerationsD3D11::UnlockDevice(d3dDevice);
    }

    // Prepare and set render targets. Clear their contents.
    const auto gBuffer0Surface = gBuffer0Tex->GetSurface();
    const auto gBuffer1Surface = gBuffer1Tex->GetSurface();
    const auto gBuffer2Surface = gBuffer2Tex->GetSurface();
    const auto gBuffer3Surface = gBuffer3Tex->GetSurface();

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

    // Make the game use "deferred" shader permutations.
    const hh::base::CStringSymbol deferredSymbol = "deferred";
    device->m_pRenderingInfrastructure->m_RenderingDevice.m_pPixelShaderPermutation = &deferredSymbol;

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
    device->SetTexture(13, shadowMap);

    This->RenderScene(
        hh::ygg::eRenderCategory_Object | 
        hh::ygg::eRenderCategory_ObjectOverlay | hh::ygg::eRenderCategory_Player,
        hh::ygg::eRenderLevel_Opaque);

    device->SetTexture(13, shadowMapNoTerrain);

    This->RenderScene(hh::ygg::eRenderCategory_Terrain,
        hh::ygg::eRenderLevel_Opaque);

    // Done with D3DRS_ALPHATESTENABLE FALSE, unlock it.
    renderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Render punch-through meshes this time and enable alpha testing.
    renderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    renderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    device->SetTexture(13, shadowMap);

    This->RenderScene(
        hh::ygg::eRenderCategory_Object | 
        hh::ygg::eRenderCategory_ObjectOverlay | hh::ygg::eRenderCategory_Player,
        hh::ygg::eRenderLevel_PunchThrough);

    device->SetTexture(13, shadowMapNoTerrain);

    This->RenderScene(hh::ygg::eRenderCategory_Terrain,
        hh::ygg::eRenderLevel_PunchThrough);

    // Done with D3DRS_ALPHATESTENABLE TRUE, unlock it.
    renderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Done with D3DRS_ZENABLE/D3DRS_ZWRITEENABLE TRUE, unlock them.
    renderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    renderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);

    // Revert back to normal permutations.
    device->m_pRenderingInfrastructure->m_RenderingDevice.m_pPixelShaderPermutation = nullptr;

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
        // Force SSAO to allocate
        if (!ssaoTex->m_pD3DTexture)
        {
            device->SetRenderTarget(0, ssaoTex->GetSurface());
            device->Flush();
        }

        // ???
        if (!ssaoTex->m_pD3DTexture)
            __debugbreak();

        HBAOPlusHandler::initialize(GenerationsD3D11::GetDevice(d3dDevice));

        const auto lock = GenerationsD3D11::LockGuard(d3dDevice);
        
        HBAOPlusHandler::execute(
            GenerationsD3D11::GetDeviceContext(d3dDevice),
            GenerationsD3D11::GetShaderResourceView(gBuffer3Tex->m_pD3DTexture),
            GenerationsD3D11::GetShaderResourceView(This->m_spDepthTex->m_pD3DTexture),
            GenerationsD3D11::GetRenderTargetView(ssaoTex->GetSurface()->m_pD3DSurface));
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
        d3dDevice->SetTexture(19, ssaoTex->m_pD3DTexture);

    filterCB.light.enableSSAO = SceneEffect::ssao.enable;
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
        device->SetShader(fxRLRShader);
        device->SetRenderTarget(0, rlrTex->GetSurface());
        device->SetTexture(0, gBuffer0Tex);

        // Set parameters
        filterCB.rlr.framebufferSize[0] = (float)This->m_spColorTex->m_CreationParams.Width;
        filterCB.rlr.framebufferSize[1] = (float)This->m_spColorTex->m_CreationParams.Height;
        filterCB.rlr.framebufferSize[2] = 1.0f / (float)This->m_spColorTex->m_CreationParams.Width;
        filterCB.rlr.framebufferSize[3] = 1.0f / (float)This->m_spColorTex->m_CreationParams.Height;
        filterCB.rlr.stepCount = SceneEffect::rlr.stepCount;
        filterCB.rlr.maxRoughness = SceneEffect::debug.viewMode == DEBUG_VIEW_MODE_IBL_ONLY ? -1.0f : SceneEffect::rlr.maxRoughness;
        filterCB.rlr.rayLength = SceneEffect::rlr.rayLength;
        filterCB.rlr.fade = 1.0f / SceneEffect::rlr.fade;
        filterCB.upload(d3dDevice);

        device->DrawQuad2D(nullptr, 0, 0);

        // Apply gaussian blur
        device->SetShader(fxConvolutionFilterShader);

        for (size_t i = 1; i < rlrTex->m_CreationParams.Levels; i++)
        {
            const float width = 1.0f / (float) std::max(1u, rlrTex->m_CreationParams.Width >> i);
            const float height = 1.0f / (float) std::max(1u, rlrTex->m_CreationParams.Height >> i);

            device->SetRenderTarget(0, rlrTempTex->GetSurface(i));
            device->SetTexture(4, rlrTex);

            filterCB.gaussianBlur.offset[0] = 1.3333333333333333f * width;
            filterCB.gaussianBlur.offset[1] = 0.0f;
            filterCB.gaussianBlur.scale = exp2f((float)i);
            filterCB.gaussianBlur.level = (float)(i - 1);
            filterCB.upload(d3dDevice);

            device->DrawQuad2D(nullptr, 0, 0);

            device->SetRenderTarget(0, rlrTex->GetSurface(i));
            device->SetTexture(4, rlrTempTex);

            filterCB.gaussianBlur.offset[0] = 0.0f;
            filterCB.gaussianBlur.offset[1] = 1.3333333333333333f * height;
            filterCB.gaussianBlur.level = (float)i;
            filterCB.upload(d3dDevice);

            device->DrawQuad2D(nullptr, 0, 0);
        }
    }

    //****************************************************//
    // Deferred specular pass: Add specular lighting to   //
    // terrain and objects.                               //
    //****************************************************//

    device->SetShader(fxDeferredPassIBLShader);
    device->SetRenderTarget(0, This->m_spColorSurface);

    device->SetTexture(0, gBuffer0Tex);

    if (filterCB.ibl.enableRLR)
        d3dDevice->SetTexture(21, rlrTex->m_pD3DTexture);

    filterCB.ibl.enableSSAO = SceneEffect::ssao.enable;
    filterCB.ibl.enableRLR = SceneEffect::rlr.enable && Configuration::rlrEnable;
    filterCB.ibl.rlrLodParam = (float)(rlrTex->m_CreationParams.Levels - 1);

    if (SceneEffect::rlr.maxLod >= 0 && filterCB.ibl.rlrLodParam > SceneEffect::rlr.maxLod)
        filterCB.ibl.rlrLodParam = (float) SceneEffect::rlr.maxLod;

    filterCB.upload(d3dDevice);

    device->DrawQuad2D(nullptr, 0, 0);

    // Unset SSAO/RLR/ShadowMap.
    device->UnsetSampler(7);
    d3dDevice->SetTexture(19, nullptr);
    d3dDevice->SetTexture(21, nullptr);

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
    device->SetShader(fxCopyColorDepthShader);
    device->SetRenderTarget(0, This->m_spCapturedColorTex->GetSurface());
    device->SetDepthStencil(This->m_spCapturedDepthTex->GetSurface());

    device->SetTexture(0, This->m_spColorTex);
    device->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    device->SetTexture(1, This->m_spDepthTex);
    device->SetSamplerFilter(1, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    device->SetSamplerAddressMode(1, D3DTADDRESS_CLAMP);

    device->DrawQuad2D(nullptr, 0, 0);

    device->UnsetSampler(0);
    device->UnsetSampler(1);

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

        device->SetShader(SceneEffect::volumetricLighting.ignoreSky ? fxVolumetricLightingIgnoreSkyShader : fxVolumetricLightingShader);
        device->UnsetDepthStencil();
        device->SetRenderTarget(0, volumetricLightTex->GetSurface());

        filterCB.volumetricLighting.sampleCount = SceneEffect::volumetricLighting.sampleCount;
        filterCB.volumetricLighting.rcpSampleCount = 1.0f / (float)SceneEffect::volumetricLighting.sampleCount;
        filterCB.volumetricLighting.g = SceneEffect::volumetricLighting.g;
        filterCB.volumetricLighting.inScatteringScale = SceneEffect::volumetricLighting.inScatteringScale;
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

        filterCB.boxBlur.sourceSize[0] = 1.0f / (float)volumetricLightTex->m_CreationParams.Width;
        filterCB.boxBlur.sourceSize[1] = 1.0f / (float)volumetricLightTex->m_CreationParams.Height;
        filterCB.boxBlur.depthThreshold = SceneEffect::volumetricLighting.depthThreshold;
        filterCB.upload(d3dDevice);

        device->DrawQuad2D(nullptr, 0, 0);

        device->UnsetSampler(4);
        device->UnsetSampler(12);

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
        colorTex = ssaoTex;
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

    This->m_pScheduler->m_pMisc->m_pDevice->SetRenderTarget(0, luAvgTex->GetSurface());
    This->m_pScheduler->m_pMisc->m_pDevice->UnsetDepthStencil();
    This->m_pScheduler->m_pMisc->m_pDevice->SetShader(fxCopyColorShader.m_spVertexShader, fxCopyColorShader.m_spPixelShader);
    This->m_pScheduler->m_pMisc->m_pDevice->SetTexture(0, This->m_spLuAvgTex);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    This->m_pScheduler->m_pMisc->m_pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);
    This->m_pScheduler->m_pMisc->m_pDevice->DrawQuad2D(nullptr, 0, 0);
    This->m_pScheduler->m_pMisc->m_pDevice->UnsetSampler(0);
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