﻿#include "ShaderHandler.h"
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
    float m_Distance;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> m_spPicture;
};

Hedgehog::Mirage::SShaderPair s_FxDeferredPassTerrainShader;
Hedgehog::Mirage::SShaderPair s_FxDeferredPassObjectShader;
Hedgehog::Mirage::SShaderPair s_FxRLRShader;
Hedgehog::Mirage::SShaderPair s_FxDeferredPassIBLShader;
Hedgehog::Mirage::SShaderPair s_FxConvolutionFilterShader;

Hedgehog::Mirage::SShaderPair s_FxCopyColorDepthShader;

boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spGBuffer1Tex;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spGBuffer2Tex;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spGBuffer3Tex;

boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spRLRTex;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> s_spRLRTempTex;

std::vector<std::unique_ptr<SHLightFieldCache>> s_SHLFs;
std::vector<std::unique_ptr<IBLProbeCache>> s_IBLProbes;

template<typename T>
struct DistanceCompare
{
    bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs->m_Distance < rhs->m_Distance;
    }
};

std::set<const SHLightFieldCache*, DistanceCompare<const SHLightFieldCache*>> s_SHLFsInFrustum;
std::set<const IBLProbeCache*, DistanceCompare<const IBLProbeCache*>> s_IBLProbesInFrustum;

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

    This->m_pScheduler->GetShader(s_FxDeferredPassTerrainShader, "FxFilterPT", "FxDeferredPassTerrain");
    This->m_pScheduler->GetShader(s_FxDeferredPassObjectShader, "FxFilterPT", "FxDeferredPassObject");
    This->m_pScheduler->GetShader(s_FxRLRShader, "FxFilterPT", "FxRLR");
    This->m_pScheduler->GetShader(s_FxDeferredPassIBLShader, "FxFilterPT", "FxDeferredPassIBL");

    This->m_pScheduler->GetShader(s_FxConvolutionFilterShader, "FxFilterT", "FxConvolutionFilter");

    This->m_pScheduler->GetShader(s_FxCopyColorDepthShader, "FxFilterT", "FxCopyColorDepth");

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spGBuffer1Tex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spGBuffer2Tex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spGBuffer3Tex, 1.0f, 1.0f, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT, NULL);

    const uint32_t RLR_WIDTH = This->m_spColorTex->m_CreationParams.Width >> 1;
    const uint32_t RLR_HEIGHT = This->m_spColorTex->m_CreationParams.Height >> 1;

    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spRLRTex, RLR_WIDTH, RLR_HEIGHT, CalculateMipCount(RLR_WIDTH, RLR_HEIGHT), D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);
    This->m_pScheduler->m_pMisc->m_pDevice->CreateTexture(s_spRLRTempTex, RLR_WIDTH, RLR_HEIGHT, CalculateMipCount(RLR_WIDTH, RLR_HEIGHT), D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, NULL);

    s_spEnvBRDFPicture = nullptr;
    s_spDefaultIBLPicture = nullptr;
}

HOOK(void, __fastcall, CFxRenderGameSceneExecute, Sonic::fpCFxRenderGameSceneExecute, Sonic::CFxRenderGameScene* This)
{
    // If we changed stages, clear the resources.
    if (StageId::hasChanged())
    {
        s_SHLFs.clear();
        s_IBLProbes.clear();
        s_spDefaultIBLPicture = nullptr;
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
            HlBlob hlBlob = { spShlfData->m_spData.get(), spShlfData->m_DataSize };
            hlBINAV2Fix(&hlBlob);
            
            SHLightFieldSet* shlfSet = (SHLightFieldSet*)hlBINAV2GetData(&hlBlob);

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
            HlBlob hlBlob = { spProbeData->m_spData.get(), spProbeData->m_DataSize };
            hlBINAV2Fix(&hlBlob);

            IBLProbeSet* iblProbeSet = (IBLProbeSet*)hlBINAV2GetData(&hlBlob);

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
    pDevice->Clear({ D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0, 1.0f, 0 });

    //************************************************************//
    // Pre-pass: Render opaque/punch-through objects and terrain. //
    //************************************************************//

    // Set g_MiddleGray_Scale_LuminanceLow_LuminanceHigh
    pD3DDevice->SetPixelShaderConstantF(193, (const float*)0x1A572D0, 1);

    // Enable Z buffer.
    pRenderingDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ZENABLE);

    pRenderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    pRenderingDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
    pRenderingDevice->LockRenderState(D3DRS_ZFUNC);

    // Enable stencil buffer.
    pRenderingDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_STENCILENABLE);

    // We're forward-rendering first, so we'd like to replace the contents of the stencil buffer.
    pRenderingDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
    pRenderingDevice->LockRenderState(D3DRS_STENCILPASS);

    // Disable alpha blending since we're rendering opaque/punch-through meshes.
    pRenderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

    // Enable g_IsUseDeferred so shaders output data to GBuffer render targets.
    BOOL isUseDeferred[] = { true };
    pD3DDevice->SetPixelShaderConstantB(6, isUseDeferred, 1);
    pD3DDevice->SetVertexShaderConstantB(6, isUseDeferred, 1);

    // Enable g_IsUseCubicFilter
    const BOOL isUseCubicFilter[] = { true };
    pD3DDevice->SetPixelShaderConstantB(8, isUseCubicFilter, 1);

    // Set Default IBL and Env BRDF
    pDevice->SetSampler(14, s_spDefaultIBLPicture);
    pDevice->SetSamplerFilter(14, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    pDevice->SetSamplerAddressMode(14, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(15, s_spEnvBRDFPicture);
    pDevice->SetSamplerFilter(15, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(15, D3DTADDRESS_CLAMP);

    // We'll render terrain first. Terrain will have a stencil value of 1.
    pRenderingDevice->SetRenderState(D3DRS_STENCILREF, 1);
    pRenderingDevice->LockRenderState(D3DRS_STENCILREF);

    // We're rendering opaque terrain meshes first, so disable alpha testing.
    pRenderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    This->RenderScene(Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_TERRAIN, Hedgehog::Yggdrasill::HH_YGG_RENDER_SLOT_OPAQUE);

    // Done with D3DRS_ALPHATESTENABLE FALSE, unlock it.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Render punch-through terrain meshes this time and enable alpha testing.
    pRenderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    This->RenderScene(Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_TERRAIN, Hedgehog::Yggdrasill::HH_YGG_RENDER_SLOT_PUNCH_THROUGH);

    // Done with D3DRS_ALPHATESTENABLE TRUE, unlock it.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Done with D3DRS_STENCILREF 1, unlock it.
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILREF);

    // We're rendering objects now. Objects will have a stencil value of 2.
    pRenderingDevice->SetRenderState(D3DRS_STENCILREF, 2);
    pRenderingDevice->LockRenderState(D3DRS_STENCILREF);

    // Opaque meshes first, so disable alpha testing.
    pRenderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    This->RenderScene(
        Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_OBJECT | Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_PLAYER,
        Hedgehog::Yggdrasill::HH_YGG_RENDER_SLOT_OPAQUE);

    // Done with D3DRS_ALPHATESTENABLE FALSE, unlock it.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // We can render punch-through meshes now. Enable alpha testing.
    pRenderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    This->RenderScene(
        Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_OBJECT | Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_PLAYER,
        Hedgehog::Yggdrasill::HH_YGG_RENDER_SLOT_PUNCH_THROUGH);

    // Done with D3DRS_ALPHATESTENABLE TRUE, unlock it.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);

    // Done with D3DRS_STENCILREF 2, unlock it.
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILREF);

    // Done with D3DRS_ZENABLE/D3DRS_ZWRITEENABLE TRUE, unlock them.
    pRenderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);

    // Done with D3DRS_STENCILPASS D3DSTENCILOP_REPLACE, unlock it.
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILPASS);

    // We're done rendering opaque/punch-through terrain and objects!
    pDevice->SetRenderTarget(1, nullptr);
    pDevice->SetRenderTarget(2, nullptr);
    pDevice->SetRenderTarget(3, nullptr);

    //*******************************************************//
    // Deferred terrain pass: Add direct lighting to terrain //
    // through deferred rendering.                           //
    //*******************************************************//

    // Since we're going to be rendering to quads, disable Z buffer and alpha test entirely.
    pRenderingDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ZENABLE);

    pRenderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    pRenderingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHATESTENABLE);

    // We don't want any changes to be done to the stencil buffer.
    pRenderingDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    pRenderingDevice->LockRenderState(D3DRS_STENCILPASS);

    // We only want to render when the stencil value of the pixel equals to our reference value.
    pRenderingDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
    pRenderingDevice->LockRenderState(D3DRS_STENCILFUNC);

    // Set reference value to 1 since we want to render terrain.
    pRenderingDevice->SetRenderState(D3DRS_STENCILREF, 1);
    pRenderingDevice->LockRenderState(D3DRS_STENCILREF);

    pDevice->SetShader(s_FxDeferredPassTerrainShader);

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

    // We're rendering terrain, so we want to use the shadowmap
    // that only contains objects.
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> spShadowMapNoTerrain;
    This->GetTexture(spShadowMapNoTerrain, "shadowmap_noterrain");

    pDevice->SetSampler(13, spShadowMapNoTerrain);
    pDevice->SetSamplerFilter(13, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(13, D3DTADDRESS_BORDER);

    float shadowMapNoTerrainSize[] = { spShadowMapNoTerrain->m_CreationParams.Width, 1.0f / spShadowMapNoTerrain->m_CreationParams.Width, 0, 0 };
    pD3DDevice->SetPixelShaderConstantF(150, shadowMapNoTerrainSize, 1);

    pDevice->RenderQuad(nullptr, 0, 0);

    // Done with D3DRS_STENCILREF 1, unlock it.
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILREF);

    //****************************************************//
    // Deferred object pass: Add direct lighting and SHLF //
    // to objects through deferred rendering.             //
    //****************************************************//

    // Set reference value to 2 since we want to render objects.
    pRenderingDevice->SetRenderState(D3DRS_STENCILREF, 2);
    pRenderingDevice->LockRenderState(D3DRS_STENCILREF);

    pDevice->SetShader(s_FxDeferredPassObjectShader);

    // We're rendering objects, so we want to use the shadowmap
    // that contains both objects and terrain.
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggTexture> spShadowMap;
    This->GetTexture(spShadowMapNoTerrain, "shadowmap");

    pDevice->SetSampler(13, spShadowMapNoTerrain);
    pDevice->SetSamplerFilter(13, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(13, D3DTADDRESS_BORDER);

    float shadowMapSize[] = { spShadowMapNoTerrain->m_CreationParams.Width, 1.0f / spShadowMapNoTerrain->m_CreationParams.Width, 0, 0 };
    pD3DDevice->SetPixelShaderConstantF(150, shadowMapSize, 1);

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

        pD3DDevice->SetPixelShaderConstantF(151 + i * 3, cache->m_InverseMatrix.data(), 3);

        float shlfParam[] = { (1.0f / cache->m_ProbeCounts[0]) * 0.5f, (1.0f / cache->m_ProbeCounts[1]) * 0.5f, (1.0f / cache->m_ProbeCounts[2]) * 0.5f, 0 };
        pD3DDevice->SetPixelShaderConstantF(163 + i, shlfParam, 1);
    }

    pDevice->RenderQuad(nullptr, 0, 0);

    // We're done with D3DRS_STENCILFUNC/D3DRS_STENCILREF, unlock them.
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILFUNC);
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILREF);

    //*****************************//
    // Real-time Local Reflections //
    //*****************************//

    // Disable stencil buffer.
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILENABLE);
    pRenderingDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_STENCILENABLE);

    boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spRLRSurface;
    s_spRLRTex->GetSurface(spRLRSurface, 0, 0);

    pDevice->SetShader(s_FxRLRShader);
    pDevice->SetRenderTarget(0, spRLRSurface);
    pDevice->SetDepthStencil(nullptr);
    pDevice->Clear(D3DCLEAR_TARGET, 0, 1.0f, 0);
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
        const float param[] = { 1.0f / desc.Width, 1.0f / desc.Height, exp2f(i), (float)i };
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

    // Enable stencil buffer back.
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILENABLE);
    pRenderingDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_STENCILENABLE);

    // Revert render targets/textures.
    pDevice->SetRenderTarget(0, This->m_spColorSurface);
    pDevice->SetDepthStencil(This->m_spDepthSurface);
    pDevice->SetSampler(0, This->m_spColorTex);
    pDevice->SetSamplerFilter(0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE);

    //****************************************************//
    // Deferred specular pass: Add specular lighting to   //
    // terrain and objects.                               //
    //****************************************************//
    
    // Since we're going to operate on both terrain and objects,
    // set stencil values accordingly.
    pRenderingDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_LESS);
    pRenderingDevice->LockRenderState(D3DRS_STENCILFUNC);

    pRenderingDevice->SetRenderState(D3DRS_STENCILREF, 0);
    pRenderingDevice->LockRenderState(D3DRS_STENCILREF);

    pDevice->SetShader(s_FxDeferredPassIBLShader);

    // Pick probes in the view.
    s_IBLProbesInFrustum.clear();

    for (auto& probe : s_IBLProbes)
    {
        probe->m_Distance = (pSceneRenderer->m_pCamera->m_Position - probe->m_Position).squaredNorm();
        s_IBLProbesInFrustum.insert(probe.get());
    }

    float probeParams[32];
    float probeLodParams[8];

    auto probeIterator = s_IBLProbesInFrustum.begin();

    for (size_t i = 0; i < std::min<size_t>(s_IBLProbesInFrustum.size(), 8); i++)
    {
        const IBLProbeCache* cache = *probeIterator++;

        pD3DDevice->SetPixelShaderConstantF(150 + i * 3, cache->m_InverseMatrix.data(), 3);

        probeParams[i * 4 + 0] = cache->m_Position.x();
        probeParams[i * 4 + 1] = cache->m_Position.y();
        probeParams[i * 4 + 2] = cache->m_Position.z();
        probeParams[i * 4 + 3] = cache->m_Bias;

        if (cache->m_spPicture && cache->m_spPicture->m_spPictureData && cache->m_spPicture->m_spPictureData->m_pD3DTexture)
            probeLodParams[i] = std::min<float>(3, cache->m_spPicture->m_spPictureData->m_pD3DTexture->GetLevelCount());
        else
            probeLodParams[i] = 0.0f;

        pDevice->SetSampler(4 + i, cache->m_spPicture);
        pDevice->SetSamplerFilter(4 + i, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
        pDevice->SetSamplerAddressMode(4 + i, D3DTADDRESS_CLAMP);
    }

    pD3DDevice->SetPixelShaderConstantF(174, probeParams, 1);
    pD3DDevice->SetPixelShaderConstantF(182, probeLodParams, 1);

    // Set RLR, Default IBL and Env BRDF
    pDevice->SetSampler(13, s_spRLRTex);
    pDevice->SetSamplerFilter(13, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    pDevice->SetSamplerAddressMode(13, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(14, s_spDefaultIBLPicture);
    pDevice->SetSamplerFilter(8, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    pDevice->SetSamplerAddressMode(8, D3DTADDRESS_CLAMP);

    pDevice->SetSampler(15, s_spEnvBRDFPicture);
    pDevice->SetSamplerFilter(9, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_NONE);
    pDevice->SetSamplerAddressMode(9, D3DTADDRESS_CLAMP);

    // Set IBL parameter.
    if (s_spDefaultIBLPicture && s_spDefaultIBLPicture->m_spPictureData && s_spDefaultIBLPicture->m_spPictureData->m_pD3DTexture)
    {
        float iblLodParam[] = { std::min<float>(3, s_spDefaultIBLPicture->m_spPictureData->m_pD3DTexture->GetLevelCount()), s_spRLRTex->m_CreationParams.Levels, 0, 0 };
        pD3DDevice->SetPixelShaderConstantF(184, iblLodParam, 1);
    }

    pDevice->RenderQuad(nullptr, 0, 0);

    //***************************************************************//
    // Forward rendering: Render sky, transparent objects and water. //
    //***************************************************************//

    // We're doing forward rendering now. Set the deferred bool to false.
    isUseDeferred[0] = false;
    pD3DDevice->SetPixelShaderConstantB(6, isUseDeferred, 1);
    pD3DDevice->SetVertexShaderConstantB(6, isUseDeferred, 1);

    //*****//
    // Sky //
    //*****//

    // We don't want the sky to intersect with any mesh in the view.
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILFUNC);
    pRenderingDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
    pRenderingDevice->LockRenderState(D3DRS_STENCILFUNC);

    pRenderingDevice->UnlockRenderState(D3DRS_STENCILREF);
    pRenderingDevice->SetRenderState(D3DRS_STENCILREF, 0);
    pRenderingDevice->LockRenderState(D3DRS_STENCILREF);

    // Enable Z buffer back.
    pRenderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    pRenderingDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pRenderingDevice->LockRenderState(D3DRS_ZENABLE);

    // We don't want sky to write to the Z buffer.
    pRenderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);
    pRenderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    // Set the depth stencil back.
    pDevice->SetDepthStencil(This->m_spColorSurface);

    This->RenderScene(Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_SKY, -1);

    //*********//
    // Capture //
    //*********//

    // Disable stencil buffer as we don't need it anymore.
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILENABLE);
    pRenderingDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_STENCILENABLE);

    // Enable Z writing, but always do it.
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

    //*********************//
    // Transparent objects //
    //*********************//

    pDevice->SetRenderTarget(0, This->m_spColorSurface);
    pDevice->SetDepthStencil(This->m_spDepthSurface);

    // We don't want transparent objects and water to write to Z buffer. It should do comparisons, though.
    pRenderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);
    pRenderingDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ZWRITEENABLE);

    pRenderingDevice->UnlockRenderState(D3DRS_ZFUNC);
    pRenderingDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
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
    pD3DDevice->SetPixelShaderConstantF(192, shadowMapSize, 1);

    This->RenderScene(
        Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_OBJECT | Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_PLAYER,
        Hedgehog::Yggdrasill::HH_YGG_RENDER_SLOT_TRANSPARENT);

    // Terrain
    pDevice->SetSampler(13, spShadowMapNoTerrain);
    pD3DDevice->SetPixelShaderConstantF(192, shadowMapNoTerrainSize, 1);

    This->RenderScene(Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_TERRAIN, Hedgehog::Yggdrasill::HH_YGG_RENDER_SLOT_TRANSPARENT);

    // Done with transparency, revert render states.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    pRenderingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    pRenderingDevice->LockRenderState(D3DRS_ALPHABLENDENABLE);

    //*******//
    // Water //
    //*******//

    // Water doesn't need alpha blending so we keep it false.
    This->RenderScene(Hedgehog::Yggdrasill::HH_YGG_RENDER_TYPE_TERRAIN, Hedgehog::Yggdrasill::HH_YGG_RENDER_SLOT_WATER);

    // Unlock render states that we're done with.
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHABLENDENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ALPHATESTENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILFUNC);
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILPASS);
    pRenderingDevice->UnlockRenderState(D3DRS_STENCILREF);
    pRenderingDevice->UnlockRenderState(D3DRS_ZENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ZWRITEENABLE);
    pRenderingDevice->UnlockRenderState(D3DRS_ZFUNC);

    This->SetDefaultTexture(This->m_spColorTex);
    This->SetBuffer("colortex", This->m_spColorTex);
    This->SetBuffer("depthtex", This->m_spDepthTex);
    This->SetBuffer("captured_colortex", This->m_spCapturedColorTex);
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

    // RAWZ -> INTZ (INTZ supports stencil)
    constexpr uint32_t INTZ = MAKEFOURCC('I', 'N', 'T', 'Z');
    WRITE_MEMORY(0x10C8A8B, uint32_t, INTZ);
    WRITE_MEMORY(0x10C8C23, uint32_t, INTZ);

    INSTALL_HOOK(CFxRenderGameSceneInitialize);
    INSTALL_HOOK(CFxRenderGameSceneExecute);

    INSTALL_HOOK(CRenderingDeviceSetViewMatrix);
}
