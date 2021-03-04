#include "ShaderHandler.h"
#include "StageId.h"
#include "SHLightField.h"
#include "IBLProbe.h"

//
// TODO: Split literally everything here to multiple files.
//

struct SHLightFieldCache
{
    Eigen::Matrix4f m_InverseMatrix;
    Eigen::Vector3f m_Position;
    uint32_t m_ProbeCounts[3];
    float m_Distance;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> m_spPicture;
};

struct IBLProbeCache
{
    Eigen::Matrix4f m_InverseMatrix;
    Eigen::Vector3f m_Position;
    float m_Bias;
    float m_Radius;
    float m_Distance;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> m_spPicture;
};

struct LightCache
{
    boost::shared_ptr<Hedgehog::Mirage::CLightData> m_spLightData;
    float m_Distance;
};

std::array<Hedgehog::Mirage::SShaderPair, 1 + 32> s_FxDeferredPassLightShaders;
Hedgehog::Mirage::SShaderPair s_FxRLRShader;
std::array<Hedgehog::Mirage::SShaderPair, 1 + 8> s_FxDeferredPassIBLShaders;
Hedgehog::Mirage::SShaderPair s_FxConvolutionFilterShader;

Hedgehog::Mirage::SShaderPair s_FxCopyColorDepthShader;

Hedgehog::Mirage::SShaderPair s_FxSSAOShader;

boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spGBuffer1Tex;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spGBuffer2Tex;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spGBuffer3Tex;

boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spRLRTex;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spRLRTempTex;

boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spSSAOTex;

std::vector<std::unique_ptr<SHLightFieldCache>> s_SHLFs;
std::vector<std::unique_ptr<IBLProbeCache>> s_IBLProbes;

template<typename T>
struct DistanceComparePointer
{
    bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs->m_Distance < rhs->m_Distance;
    }
};

template<typename T>
struct DistanceCompareReference
{
    bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs.m_Distance < rhs.m_Distance;
    }
};

std::set<const SHLightFieldCache*, DistanceComparePointer<const SHLightFieldCache*>> s_SHLFsInFrustum;
std::set<const IBLProbeCache*, DistanceComparePointer<const IBLProbeCache*>> s_IBLProbesInFrustum;
std::set<LightCache, DistanceCompareReference<LightCache>> s_LocalLightsInFrustum;

boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> s_spEnvBRDFPicture;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> s_spDefaultIBLPicture;

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

        This->m_pScheduler->GetShader(s_FxDeferredPassLightShaders[i], "FxFilterPT", name);
    }

    This->m_pScheduler->GetShader(s_FxRLRShader, "FxFilterPT", "FxRLR");

    for (size_t i = 0; i < 1 + 8; i++)
    {
        char name[32];
        sprintf(name, "FxDeferredPassIBL_%d", i);

        This->m_pScheduler->GetShader(s_FxDeferredPassIBLShaders[i], "FxFilterPT", name);
    }

    This->m_pScheduler->GetShader(s_FxConvolutionFilterShader, "FxFilterT", "FxConvolutionFilter");

    This->m_pScheduler->GetShader(s_FxCopyColorDepthShader, "FxFilterT", "FxCopyColorDepth");

    This->m_pScheduler->GetShader(s_FxSSAOShader, "FxFilterPT", "FxSSAO");

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spGBuffer1Tex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spGBuffer2Tex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spGBuffer3Tex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT, NULL);

    const uint32_t RLR_WIDTH = This->m_spColorTex->m_CreationParams.Width >> 1;
    const uint32_t RLR_HEIGHT = This->m_spColorTex->m_CreationParams.Height >> 1;

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spRLRTex, RLR_WIDTH, RLR_HEIGHT, CalculateMipCount(RLR_WIDTH, RLR_HEIGHT), D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spRLRTempTex, RLR_WIDTH, RLR_HEIGHT, CalculateMipCount(RLR_WIDTH, RLR_HEIGHT), D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);

    const uint32_t SSAO_WIDTH = This->m_spColorTex->m_CreationParams.Width >> 1;
    const uint32_t SSAO_HEIGHT = This->m_spColorTex->m_CreationParams.Height >> 1;

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spSSAOTex, SSAO_WIDTH, SSAO_HEIGHT, 1, D3DUSAGE_RENDERTARGET, D3DFMT_L16, D3DPOOL_DEFAULT, NULL);

    s_spEnvBRDFPicture.reset();
    s_spDefaultIBLPicture.reset();
}

HOOK(void, __fastcall, CFxRenderGameSceneExecute, Sonic::fpCFxRenderGameSceneExecute, Sonic::CFxRenderGameScene* This)
{
    // If we changed stages, clear the resources.
    if (StageId::hasChanged())
    {
        s_SHLFs.clear();
        s_IBLProbes.clear();
        s_spDefaultIBLPicture.reset();
    }

    if (!s_spEnvBRDFPicture)
        This->m_pScheduler->GetPicture(s_spEnvBRDFPicture, "env_brdf");

    if (!s_spDefaultIBLPicture)
        This->m_pScheduler->GetPicture(s_spDefaultIBLPicture, (StageId::get() + "_defaultibl").c_str());

    // If we haven't read the SHLF yet, read it.
    if (s_SHLFs.empty() && !StageId::isEmpty() && (*Sonic::CGameDocument::ms_pInstance) != nullptr)
    {
        boost::shared_ptr<Hedgehog::Database::CRawData> spShlfData;
        (*Sonic::CGameDocument::ms_pInstance)->m_pMember->m_spDatabase->GetRawData(spShlfData, (StageId::get() + ".shlf").c_str(), 0);

        if (spShlfData && spShlfData->m_spData)
        {
            hlBINAV2Fix(spShlfData->m_spData.get(), spShlfData->m_DataSize);

            SHLightFieldSet* shlfSet = (SHLightFieldSet*)hlBINAV2GetData(spShlfData->m_spData.get());

            for (uint32_t i = 0; i < shlfSet->SHLFCount; i++)
            {
                const SHLightField& shlf = shlfSet->SHLFs[i];

                Eigen::Affine3f affine =
                    Eigen::Translation3f(shlf.Position[0], shlf.Position[1], shlf.Position[2]) *
                    Eigen::AngleAxisf(shlf.Rotation[0], Eigen::Vector3f::UnitX()) *
                    Eigen::AngleAxisf(shlf.Rotation[1], Eigen::Vector3f::UnitY()) *
                    Eigen::AngleAxisf(shlf.Rotation[2], Eigen::Vector3f::UnitZ()) *
                    Eigen::Scaling(shlf.Scale[0], shlf.Scale[1], shlf.Scale[2]);

                SHLightFieldCache cache;

                cache.m_ProbeCounts[0] = shlf.ProbeCounts[0];
                cache.m_ProbeCounts[1] = shlf.ProbeCounts[1];
                cache.m_ProbeCounts[2] = shlf.ProbeCounts[2];

                cache.m_InverseMatrix = affine.inverse().matrix();
                cache.m_Position = Eigen::Vector3f(shlf.Position[0], shlf.Position[1], shlf.Position[2]) / 10.0f;

                This->m_pScheduler->GetPicture(cache.m_spPicture, shlf.Name);

                s_SHLFs.push_back(std::make_unique<SHLightFieldCache>(std::move(cache)));
            }
        }
    }

    // If we haven't read the probes yet, read them.
    if (s_IBLProbes.empty() && !StageId::isEmpty() && (*Sonic::CGameDocument::ms_pInstance) != nullptr)
    {
        boost::shared_ptr<Hedgehog::Database::CRawData> spProbeData;
        (*Sonic::CGameDocument::ms_pInstance)->m_pMember->m_spDatabase->GetRawData(spProbeData, (StageId::get() + ".probe").c_str(), 0);

        if (spProbeData && spProbeData->m_spData)
        {
            hlBINAV2Fix(spProbeData->m_spData.get(), spProbeData->m_DataSize);

            IBLProbeSet* iblProbeSet = (IBLProbeSet*)hlBINAV2GetData(spProbeData->m_spData.get());

            for (uint32_t i = 0; i < iblProbeSet->ProbeCount; i++)
            {
                const IBLProbe& iblProbe = iblProbeSet->Probes[i];

                Eigen::Matrix4f matrix;

                for (uint32_t x = 0; x < 4; x++)
                    for (uint32_t y = 0; y < 4; y++)
                        matrix(x, y) = iblProbe.Matrix[x][y];

                IBLProbeCache cache;

                cache.m_InverseMatrix = matrix.inverse();
                cache.m_Position = Eigen::Vector3f(iblProbe.Position[0], iblProbe.Position[1], iblProbe.Position[2]) / 10.0f;
                cache.m_Bias = iblProbe.Bias;

                // help how do I cull OBBs 
                const float scaleX = matrix.col(0).head<3>().norm();
                const float scaleY = matrix.col(1).head<3>().norm();
                const float scaleZ = matrix.col(2).head<3>().norm();

                cache.m_Radius = std::max<float>(std::max<float>(scaleX, scaleY), scaleZ) / 2.0f * sqrtf(2.0f);

                This->m_pScheduler->GetPicture(cache.m_spPicture, iblProbe.Name);

                s_IBLProbes.push_back(std::make_unique<IBLProbeCache>(std::move(cache)));
            }
        }
    }

    // Cache variables because it gets verbose if I don't.
    DX_PATCH::IDirect3DDevice9* pD3DDevice = This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice;
    Hedgehog::Mirage::CRenderingDevice* pRenderingDevice = &This->m_pScheduler->m_pMisc->m_pRenderingInfrastructure->m_RenderingDevice;
    Hedgehog::Yggdrasill::CYggDevice* pDevice = This->m_pScheduler->m_pMisc->m_pDevice;
    Sonic::CFxSceneRenderer* pSceneRenderer = (Sonic::CFxSceneRenderer*)This->m_pScheduler->m_pMisc->m_spSceneRenderer.get();

    const Frustum frustum(pSceneRenderer->m_pCamera->m_Projection * pSceneRenderer->m_pCamera->m_View);

    // Prepare and set render targets. Clear their contents.
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spGBuffer1Surface;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spGBuffer2Surface;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spGBuffer3Surface;

    s_spGBuffer1Tex->GetSurface(spGBuffer1Surface, 0, 0);
    s_spGBuffer2Tex->GetSurface(spGBuffer2Surface, 0, 0);
    s_spGBuffer3Tex->GetSurface(spGBuffer3Surface, 0, 0);

    pDevice->SetRenderTarget(0, This->m_spColorSurface);
    pDevice->SetRenderTarget(1, spGBuffer1Surface);
    pDevice->SetRenderTarget(2, spGBuffer2Surface);
    pDevice->SetRenderTarget(3, spGBuffer3Surface);
    pDevice->SetDepthStencil(This->m_spDepthSurface);
    pDevice->Clear({ D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0 });

    //***********************//
    // Pre-pass: Render sky. //
    //***********************//

    // Disable Z buffer.
    pRenderingDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ZENABLE);

    // Disable alpha testing.
    pRenderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    // Enable alpha blending.
    pRenderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

    This->RenderScene(Hedgehog::Yggdrasill::eRenderType_Sky, -1);

    // Unlock render states we're done with.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ZENABLE);

    //************************************************************//
    // Pre-pass: Render opaque/punch-through objects and terrain. //
    //************************************************************//

    // Set g_HDRParam_SGGIParam
    float sggiScale = 1.0f / std::max<float>(0.001f, SceneEffect::SGGI.StartSmoothness - SceneEffect::SGGI.EndSmoothness);
    float hdrParam_sggiParam[] = { SceneEffect::HDR.Luminance, 1.0f, -(1.0f - SceneEffect::SGGI.StartSmoothness) * sggiScale, sggiScale };
    pD3DDevice->SetPixelShaderConstantF(106, hdrParam_sggiParam, 1);

    // Set g_DebugParam
    float debugParam[] =
    {
        SceneEffect::Debug.UseWhiteAlbedo || SceneEffect::Debug.ViewMode == DEBUG_VIEW_MODE_GI_ONLY ? 1.0f : -1.0f,
        SceneEffect::Debug.UseFlatNormal ? 1.0f : -1.0f,
        std::min<float>(1.0f, SceneEffect::Debug.ReflectanceOverride),
        std::min<float>(1.0f, SceneEffect::Debug.RoughnessOverride),
        std::min<float>(1.0f, SceneEffect::Debug.MetalnessOverride),
        std::min<float>(1.0f, SceneEffect::Debug.GIShadowMapOverride),
        0,
        0
    };

    pD3DDevice->SetPixelShaderConstantF(222, debugParam, 2);

    // Enable mrgIsUseDeferred so shaders output data to GBuffer render targets.
    BOOL isUseDeferred[] = { true };
    pD3DDevice->SetPixelShaderConstantB(8, isUseDeferred, 1);
    pD3DDevice->SetVertexShaderConstantB(8, isUseDeferred, 1);

    // Setup GI & occlusion flags.
    pDevice->SetSamplerFilter(9, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(9, D3DTADDRESS_CLAMP);
    pDevice->SetSamplerFilter(10, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(10, D3DTADDRESS_CLAMP);

    // Enable Z buffer.
    pRenderingDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ZENABLE);

    pRenderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    pRenderingDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    pRenderingDevice->LockRenderState(D3DRS_ZFUNC);

    // Disable alpha blending since we're rendering opaque/punch-through meshes.
    pRenderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

    // Set Default IBL and Env BRDF
    pDevice->SetSampler(14, s_spDefaultIBLPicture);
    pDevice->SetSamplerFilter(14, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    pDevice->SetSamplerAddressMode(14, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(15, s_spEnvBRDFPicture);
    pDevice->SetSamplerFilter(15, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(15, D3DTADDRESS_CLAMP);

    // We're rendering opaque meshes first, so disable alpha testing.
    pRenderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    // Render objects & player separately from terrain so it gets culled better.
    This->RenderScene(
        Hedgehog::Yggdrasill::eRenderType_Object | Hedgehog::Yggdrasill::eRenderType_Player,
        Hedgehog::Yggdrasill::eRenderSlot_Opaque);

    This->RenderScene(Hedgehog::Yggdrasill::eRenderType_Terrain,
        Hedgehog::Yggdrasill::eRenderSlot_Opaque);

    // Done with D3DRS_ALPHATESTENABLE FALSE, unlock it.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Render punch-through meshes this time and enable alpha testing.
    pRenderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    This->RenderScene(
        Hedgehog::Yggdrasill::eRenderType_Object | Hedgehog::Yggdrasill::eRenderType_Player,
        Hedgehog::Yggdrasill::eRenderSlot_PunchThrough);

    This->RenderScene(Hedgehog::Yggdrasill::eRenderType_Terrain,
        Hedgehog::Yggdrasill::eRenderSlot_PunchThrough);

    // Done with D3DRS_ALPHATESTENABLE TRUE, unlock it.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Done with D3DRS_ZENABLE/D3DRS_ZWRITEENABLE TRUE, unlock them.
    pRenderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);

    // We're done rendering opaque/punch-through terrain and objects!
    pDevice->UnsetRenderTarget(1);
    pDevice->UnsetRenderTarget(2);
    pDevice->UnsetRenderTarget(3);

    //***************//
    // Deferred pass //
    //***************//

    // Since we're going to be rendering to quads, disable Z buffer and alpha test entirely.
    pRenderingDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ZENABLE);

    pRenderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    pRenderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    pDevice->SetSampler(0, This->m_spColorTex);
    pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(1, s_spGBuffer1Tex);
    pDevice->SetSamplerFilter(1, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(1, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(2, s_spGBuffer2Tex);
    pDevice->SetSamplerFilter(2, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(2, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(3, s_spGBuffer3Tex);
    pDevice->SetSamplerFilter(3, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(3, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(11, This->m_spColorTex);
    pDevice->SetSamplerFilter(11, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(11, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(12, This->m_spDepthTex);
    pDevice->SetSamplerFilter(12, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(12, D3DTADDRESS_CLAMP);

    if (SceneEffect::SSAO.Enable)
    {
        boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spSSAOSurface;
        s_spSSAOTex->GetSurface(spSSAOSurface, 0, 0);

        float sampleCount_invSampleCount_radius_distanceFade[] =
        {
            (float)SceneEffect::SSAO.SampleCount,
            1.0f / (float)SceneEffect::SSAO.SampleCount,
            SceneEffect::SSAO.Radius,
            SceneEffect::SSAO.DistanceFade
        };

        float strength[] =
        {
            SceneEffect::SSAO.Strength,
            0,
            0,
            0
        };

        pD3DDevice->SetPixelShaderConstantF(150, sampleCount_invSampleCount_radius_distanceFade, 1);
        pD3DDevice->SetPixelShaderConstantF(151, strength, 1);

        pDevice->SetShader(s_FxSSAOShader);

        pDevice->SetRenderTarget(0, spSSAOSurface);
        pDevice->RenderQuad(nullptr, 0, 0);
    }

    // Pass 32 omni lights from the view to shaders.
    size_t localLightCount = 0;

    if (pSceneRenderer->m_pLightManager && pSceneRenderer->m_pLightManager->m_pStaticLightContext &&
        pSceneRenderer->m_pLightManager->m_pStaticLightContext->m_spLightListData)
    {
        Hedgehog::Mirage::CLightListData* pLightListData =
            pSceneRenderer->m_pLightManager->m_pStaticLightContext->m_spLightListData.get();

        s_LocalLightsInFrustum.clear();

        for (auto it = pLightListData->m_Lights.m_pBegin; it != pLightListData->m_Lights.m_pEnd; it++)
        {
            if ((*it)->m_Type != Hedgehog::Mirage::eLightType_Omni)
                continue;

            const float distance = ((*it)->m_Position - pSceneRenderer->m_pCamera->m_Position).squaredNorm();

            if (distance < 100000 && frustum.intersects((*it)->m_Position, (*it)->m_Range.w()))
                s_LocalLightsInFrustum.insert({ *it, distance });
        }

        float localLightData[10 * 32];

        auto lightIterator = s_LocalLightsInFrustum.begin();

        localLightCount = std::min<size_t>(32, s_LocalLightsInFrustum.size());

        for (size_t i = 0; i < localLightCount; i++)
        {
            Hedgehog::Mirage::CLightData* pLightData = (*lightIterator++).m_spLightData.get();

            localLightData[i * 10 + 0] = pLightData->m_Position.x();
            localLightData[i * 10 + 1] = pLightData->m_Position.y();
            localLightData[i * 10 + 2] = pLightData->m_Position.z();
            localLightData[i * 10 + 3] = pLightData->m_Color.x();
            localLightData[i * 10 + 4] = pLightData->m_Color.y();
            localLightData[i * 10 + 5] = pLightData->m_Color.z();
            localLightData[i * 10 + 6] = pLightData->m_Range.x();
            localLightData[i * 10 + 7] = pLightData->m_Range.y();
            localLightData[i * 10 + 8] = pLightData->m_Range.z();
            localLightData[i * 10 + 9] = pLightData->m_Range.w();
        }

        if (localLightCount > 0)
            pD3DDevice->SetPixelShaderConstantF(107, (const float*)localLightData, std::min<int>(80, (localLightCount * 10 + 5) / 4));
    }

    if (SceneEffect::Debug.DisableOmniLight || SceneEffect::Debug.ViewMode == DEBUG_VIEW_MODE_GI_ONLY)
        localLightCount = 0;

    //***************************************************//
    // Deferred light pass: Add direct lighting and SHLF //
    // to meshes through deferred rendering.             //
    //***************************************************//

    // Set the corresponding shader.
    pDevice->SetShader(s_FxDeferredPassLightShaders[localLightCount]);

    // Set shadowmaps.
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> spShadowMapNoTerrain;
    This->GetTexture(spShadowMapNoTerrain, "shadowmap_noterrain");

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> spShadowMap;
    This->GetTexture(spShadowMap, "shadowmap");

    pDevice->SetSampler(9, spShadowMapNoTerrain);
    pDevice->SetSamplerFilter(9, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(9, D3DTADDRESS_BORDER);

    pDevice->SetSampler(13, spShadowMap);
    pDevice->SetSamplerFilter(13, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(13, D3DTADDRESS_BORDER);

    float shadowMapParams[] = {
        (float)spShadowMap->m_CreationParams.Width, 1.0f / (float)spShadowMap->m_CreationParams.Width, SceneEffect::ESM.Factor, 0 };

    pD3DDevice->SetPixelShaderConstantF(65, shadowMapParams, 1);

    // Pick SHLFs in the view.
    s_SHLFsInFrustum.clear();

    for (auto& shlf : s_SHLFs)
    {
        shlf->m_Distance = (pSceneRenderer->m_pCamera->m_Position - shlf->m_Position).squaredNorm();
        s_SHLFsInFrustum.insert(shlf.get());
    }

    auto shlfIterator = s_SHLFsInFrustum.begin();

    for (size_t i = 0; i < std::min<size_t>(s_SHLFsInFrustum.size(), 4); i++)
    {
        const SHLightFieldCache* cache = *shlfIterator++;

        pDevice->SetSampler(4 + i, cache->m_spPicture);
        pDevice->SetSamplerFilter(4 + i, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
        pDevice->SetSamplerAddressMode(4 + i, D3DTADDRESS_CLAMP);

        pD3DDevice->SetPixelShaderConstantF(187 + i * 3, cache->m_InverseMatrix.data(), 3);

        float shlfParam[] = { (1.0f / cache->m_ProbeCounts[0]) * 0.5f, (1.0f / cache->m_ProbeCounts[1]) * 0.5f, (1.0f / cache->m_ProbeCounts[2]) * 0.5f, 0 };
        pD3DDevice->SetPixelShaderConstantF(199 + i, shlfParam, 1);

        if (SceneEffect::Debug.DisableSHLightField)
            pDevice->UnsetSampler(4 + i);
    }

    // Set SSAO.
    pD3DDevice->SetPixelShaderConstantB(8, (const BOOL*)&SceneEffect::SSAO.Enable, 1);

    if (SceneEffect::SSAO.Enable)
    {
        float ssaoSize[] =
        {
            (float)s_spSSAOTex->m_CreationParams.Width,
            (float)s_spSSAOTex->m_CreationParams.Height,

            1.0f / (float)s_spSSAOTex->m_CreationParams.Width,
            1.0f / (float)s_spSSAOTex->m_CreationParams.Height,
        };

        pD3DDevice->SetPixelShaderConstantF(203, ssaoSize, 1);

        pDevice->SetSampler(10, s_spSSAOTex);
        pDevice->SetSamplerFilter(10, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
        pDevice->SetSamplerAddressMode(10, D3DTADDRESS_CLAMP);

        pDevice->SetRenderTarget(1, spGBuffer2Surface); // To update AO in the GBuffer
    }

    if (SceneEffect::Debug.DisableDirectLight || SceneEffect::Debug.ViewMode == DEBUG_VIEW_MODE_GI_ONLY)
    {
        float globalLightColor[] = { 0, 0, 0, 0 };
        pD3DDevice->SetPixelShaderConstantF(36, globalLightColor, 1);
        pD3DDevice->SetPixelShaderConstantF(37, globalLightColor, 1);
    }

    pDevice->SetRenderTarget(0, This->m_spColorSurface);
    pDevice->RenderQuad(nullptr, 0, 0);

    if (SceneEffect::SSAO.Enable)
        pDevice->UnsetRenderTarget(1);

    //*****************************//
    // Real-time Local Reflections //
    //*****************************//

    if (SceneEffect::RLR.Enable)
    {
        boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spRLRSurface;
        s_spRLRTex->GetSurface(spRLRSurface, 0, 0);

        pDevice->SetShader(s_FxRLRShader);
        pDevice->SetRenderTarget(0, spRLRSurface);
        pDevice->UnsetDepthStencil();
        pDevice->Clear(D3DCLEAR_TARGET, 0, 1.0f, 0);

        // Set parameters
        float framebufferSize[] = {
            (float)This->m_spColorTex->m_CreationParams.Width,
            (float)This->m_spColorTex->m_CreationParams.Height,
            1.0f / (float)This->m_spColorTex->m_CreationParams.Width,
            1.0f / (float)This->m_spColorTex->m_CreationParams.Height,
        };

        float stepCount_maxRoughness_rayLength_fade[] = {
            (float)SceneEffect::RLR.StepCount,
            SceneEffect::RLR.MaxRoughness,
            SceneEffect::RLR.RayLength,
            1.0f / SceneEffect::RLR.Fade };

        float accuracyThreshold_saturation_brightness[] = {
            SceneEffect::RLR.AccuracyThreshold,
            std::min<float>(1.0f, std::max<float>(0.0f, SceneEffect::RLR.Saturation)),
            SceneEffect::RLR.Brightness,
            0 };

        pD3DDevice->SetPixelShaderConstantF(150, framebufferSize, 1);
        pD3DDevice->SetPixelShaderConstantF(151, stepCount_maxRoughness_rayLength_fade, 1);
        pD3DDevice->SetPixelShaderConstantF(152, accuracyThreshold_saturation_brightness, 1);

        pDevice->RenderQuad(nullptr, 0, 0);

        // Apply gaussian blur to it because yes.
        pDevice->SetShader(s_FxConvolutionFilterShader);

        pDevice->SetSamplerFilter(0, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_POINT);
        pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

        for (uint32_t i = 1; i < s_spRLRTex->m_CreationParams.Levels; i++)
        {
            boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spSrcSurface, spDstSurface, spDstSurfaceTemp;

            s_spRLRTex->GetSurface(spSrcSurface, i - 1, 0);
            s_spRLRTex->GetSurface(spDstSurface, i, 0);
            s_spRLRTempTex->GetSurface(spDstSurfaceTemp, i, 0);

            D3DSURFACE_DESC desc;
            spDstSurface->m_pD3DSurface->GetDesc(&desc);

            // Downscale first.
            pD3DDevice->StretchRect(spSrcSurface->m_pD3DSurface, nullptr, spDstSurface->m_pD3DSurface, nullptr, D3DTEXF_LINEAR);

            // Now apply gaussian filter to it.
            const float param[] = { 1.0f / desc.Width, 1.0f / desc.Height, exp2f((float)i), (float)i };
            pD3DDevice->SetPixelShaderConstantF(150, param, 1);

            // Horizontal
            pDevice->SetRenderTarget(0, spDstSurfaceTemp);

            float direction[] = { -1, 0, 0, 0 };
            pD3DDevice->SetPixelShaderConstantF(151, direction, 1);

            pDevice->SetSampler(0, s_spRLRTex);

            pDevice->RenderQuad(nullptr, 0, 0);

            // Vertical
            pDevice->SetRenderTarget(0, spDstSurface);

            direction[0] = 0;
            direction[1] = -1;
            pD3DDevice->SetPixelShaderConstantF(151, direction, 1);

            pDevice->SetSampler(0, s_spRLRTempTex);

            pDevice->RenderQuad(nullptr, 0, 0);
        }

        // Revert render targets/textures.
        pDevice->SetRenderTarget(0, This->m_spColorSurface);
        pDevice->SetDepthStencil(This->m_spDepthSurface);
        pDevice->SetSampler(0, This->m_spColorTex);
        pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    }

    //****************************************************//
    // Deferred specular pass: Add specular lighting to   //
    // terrain and objects.                               //
    //****************************************************//

    // Pick probes in the view.
    s_IBLProbesInFrustum.clear();

    for (auto& probe : s_IBLProbes)
    {
        if (!frustum.intersects(probe->m_Position, probe->m_Radius))
            continue;

        probe->m_Distance = (pSceneRenderer->m_pCamera->m_Position - probe->m_Position).squaredNorm();
        s_IBLProbesInFrustum.insert(probe.get());
    }

    float probeParams[32];
    float probeLodParams[8];

    auto probeIterator = s_IBLProbesInFrustum.begin();

    size_t iblCount = std::min<size_t>(s_IBLProbesInFrustum.size(), 8);

    if (SceneEffect::Debug.DisableIBLProbe)
        iblCount = 0;

    // Set the corresponding shader.
    pDevice->SetShader(s_FxDeferredPassIBLShaders[iblCount]);

    for (size_t i = 0; i < iblCount; i++)
    {
        const IBLProbeCache* cache = *probeIterator++;

        pD3DDevice->SetPixelShaderConstantF(107 + i * 3, cache->m_InverseMatrix.data(), 3);

        probeParams[i * 4 + 0] = cache->m_Position.x();
        probeParams[i * 4 + 1] = cache->m_Position.y();
        probeParams[i * 4 + 2] = cache->m_Position.z();
        probeParams[i * 4 + 3] = cache->m_Bias;

        if (cache->m_spPicture && cache->m_spPicture->m_spPictureData && cache->m_spPicture->m_spPictureData->m_pD3DTexture)
            probeLodParams[i] = std::min<float>(3.0f, (float)cache->m_spPicture->m_spPictureData->m_pD3DTexture->GetLevelCount());
        else
            probeLodParams[i] = 0.0f;

        pDevice->SetSampler(4 + i, cache->m_spPicture);
        pDevice->SetSamplerFilter(4 + i, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
        pDevice->SetSamplerAddressMode(4 + i, D3DTADDRESS_CLAMP);
    }

    if (iblCount > 0)
    {
        pD3DDevice->SetPixelShaderConstantF(131, probeParams, iblCount);
        pD3DDevice->SetPixelShaderConstantF(139, probeLodParams, 2);
    }

    // Set RLR, Default IBL and Env BRDF
    if (SceneEffect::RLR.Enable)
    {
        pDevice->SetSampler(13, s_spRLRTex);
        pDevice->SetSamplerFilter(13, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
        pDevice->SetSamplerAddressMode(13, D3DTADDRESS_CLAMP);
    }
    else
    {
        pDevice->UnsetSampler(13);
    }

    pDevice->SetSampler(14, s_spDefaultIBLPicture);
    pDevice->SetSamplerFilter(14, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    pDevice->SetSamplerAddressMode(14, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(15, s_spEnvBRDFPicture);
    pDevice->SetSamplerFilter(15, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(15, D3DTADDRESS_CLAMP);

    // Set IBL parameter.
    if (s_spDefaultIBLPicture && s_spDefaultIBLPicture->m_spPictureData && s_spDefaultIBLPicture->m_spPictureData->m_pD3DTexture)
    {
        float iblLodParam[] =
        {
            std::min<float>(3, (float)s_spDefaultIBLPicture->m_spPictureData->m_pD3DTexture->GetLevelCount()),
            (float)(SceneEffect::RLR.MaxLod >= 0 ? std::min<int32_t>(SceneEffect::RLR.MaxLod, s_spRLRTex->m_CreationParams.Levels) : s_spRLRTex->m_CreationParams.Levels),
            0,
            0
        };

        pD3DDevice->SetPixelShaderConstantF(141, iblLodParam, 1);
    }

    if (SceneEffect::Debug.DisableDefaultIBL)
        pDevice->UnsetSampler(14);

    if (SceneEffect::Debug.ViewMode == DEBUG_VIEW_MODE_GI_ONLY)
        pDevice->UnsetSampler(15);

    // Set g_IsEnableRLR
    // Looks like setting sampler to null returns 0, 0, 0, 1 instead of 0, 0, 0, 0
    pD3DDevice->SetPixelShaderConstantB(8, (const BOOL*)&SceneEffect::RLR.Enable, 1);

    pDevice->RenderQuad(nullptr, 0, 0);

    //*********//
    // Capture //
    //*********//

    // Enable Z writing, but always do it.
    pRenderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    pRenderingDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ZENABLE);

    pRenderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);
    pRenderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    pRenderingDevice->UnlockRenderState(D3DRS_ZFUNC);
    pRenderingDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    pRenderingDevice->LockRenderState(D3DRS_ZFUNC);

    // Capture the current view to use in transparent objects and water
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spCapturedColorSurface;
    This->m_spCapturedColorTex->GetSurface(spCapturedColorSurface, 0, 0);

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spCapturedDepthSurface;
    This->m_spCapturedDepthTex->GetSurface(spCapturedDepthSurface, 0, 0);

    pDevice->SetRenderTarget(0, spCapturedColorSurface);
    pDevice->SetDepthStencil(spCapturedDepthSurface);

    pDevice->SetShader(s_FxCopyColorDepthShader);

    pDevice->SetSampler(0, This->m_spColorTex);
    pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(0, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(1, This->m_spDepthTex);
    pDevice->SetSamplerFilter(1, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(1, D3DTADDRESS_CLAMP);

    pDevice->RenderQuad(nullptr, 0, 0);

    //***************************************************************//
    // Forward rendering: Render transparent objects and water. //
    //***************************************************************//

    // We're doing forward rendering now. Set the deferred bool to false.
    isUseDeferred[0] = false;
    pD3DDevice->SetPixelShaderConstantB(8, isUseDeferred, 1);
    pD3DDevice->SetVertexShaderConstantB(8, isUseDeferred, 1);

    // Setup GI & occlusion flags.
    pDevice->SetSamplerFilter(9, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(9, D3DTADDRESS_CLAMP);
    pDevice->SetSamplerFilter(10, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(10, D3DTADDRESS_CLAMP);

    pDevice->SetRenderTarget(0, This->m_spColorSurface);
    pDevice->SetDepthStencil(This->m_spDepthSurface);

    // We don't want transparent objects and water to write to Z buffer. It should do comparisons, though.
    pRenderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);
    pRenderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    pRenderingDevice->UnlockRenderState(D3DRS_ZFUNC);
    pRenderingDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    pRenderingDevice->LockRenderState(D3DRS_ZFUNC);

    // Set the framebuffer and depth samplers.
    pDevice->SetSampler(11, This->m_spCapturedColorTex);
    pDevice->SetSamplerFilter(11, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(11, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(12, This->m_spCapturedDepthTex);
    pDevice->SetSamplerFilter(12, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(12, D3DTADDRESS_CLAMP);

    // Transparent objects should do alpha blending.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    pRenderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

    // Objects
    pDevice->SetSampler(13, spShadowMap);
    pDevice->SetSamplerFilter(13, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(13, D3DTADDRESS_BORDER);
    pD3DDevice->SetPixelShaderConstantF(65, shadowMapParams, 1);

    This->RenderScene(
        Hedgehog::Yggdrasill::eRenderType_Object | Hedgehog::Yggdrasill::eRenderType_Player,
        Hedgehog::Yggdrasill::eRenderSlot_Transparent);

    // Terrain
    pDevice->SetSampler(13, spShadowMapNoTerrain);
    pD3DDevice->SetPixelShaderConstantF(65, shadowMapParams, 1);

    This->RenderScene(Hedgehog::Yggdrasill::eRenderType_Terrain, Hedgehog::Yggdrasill::eRenderSlot_Transparent);

    // Done with transparency, revert render states.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    pRenderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

    //*******//
    // Water //
    //*******//

    // Water doesn't need alpha blending so we keep it false.
    This->RenderScene(Hedgehog::Yggdrasill::eRenderType_Terrain, Hedgehog::Yggdrasill::eRenderSlot_Water);

    // Unlock render states that we're done with.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ZFUNC);

    //***********//
    // Particles //
    //***********//
    pRenderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pRenderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    pRenderingDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pRenderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    // Set HDR param to scale particles by luminance.
    hdrParam_sggiParam[1] = hdrParam_sggiParam[0];
    pD3DDevice->SetPixelShaderConstantF(106, hdrParam_sggiParam, 1);
    
    This->RenderScene(Hedgehog::Yggdrasill::eRenderType_Effect | Hedgehog::Yggdrasill::eRenderType_SparkleObject | Hedgehog::Yggdrasill::eRenderType_SparkleFramebuffer,
        Hedgehog::Yggdrasill::eRenderSlot_Opaque | Hedgehog::Yggdrasill::eRenderSlot_PunchThrough | Hedgehog::Yggdrasill::eRenderSlot_Transparent);

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> colorTex;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> capturedColorTex;

    switch (SceneEffect::Debug.ViewMode)
    {
    case DEBUG_VIEW_MODE_GBUFFER1:
        colorTex = s_spGBuffer1Tex;
        break;

    case DEBUG_VIEW_MODE_GBUFFER2:
        colorTex = s_spGBuffer2Tex;
        break;

    case DEBUG_VIEW_MODE_GBUFFER3:
        colorTex = s_spGBuffer3Tex;
        break;

    case DEBUG_VIEW_MODE_RLR:
        colorTex = s_spRLRTex;
        break;

    case DEBUG_VIEW_MODE_SSAO:
        colorTex = s_spSSAOTex;
        break;

    case DEBUG_VIEW_MODE_SHADOW_MAP:
        colorTex = spShadowMap;
        break;

    case DEBUG_VIEW_MODE_SHADOW_MAP_NO_TERRAIN:
        colorTex = spShadowMapNoTerrain;
        break;

    case DEBUG_VIEW_MODE_NONE:
    case DEBUG_VIEW_MODE_GI_ONLY:
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

HOOK(void, __fastcall, CRenderingDeviceSetViewMatrix, Hedgehog::Mirage::fpCRenderingDeviceSetViewMatrix,
    Hedgehog::Mirage::CRenderingDevice* This, void* Edx, const Eigen::Matrix4f& viewMatrix)
{
    const Eigen::Matrix4f inverseViewMatrix = viewMatrix.inverse();
    This->m_pD3DDevice->SetPixelShaderConstantF(94, inverseViewMatrix.data(), 4);

    originalCRenderingDeviceSetViewMatrix(This, Edx, viewMatrix);
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

    // Don't render anything in CFxRenderParticle
    WRITE_MEMORY(0x10C8273, uint8_t, 0x83, 0xC4, 0x08, 0x90, 0x90);
}