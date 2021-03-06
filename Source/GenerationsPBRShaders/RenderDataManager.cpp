#include "RenderDataManager.h"
#include "IBLProbe.h"
#include "SHLightField.h"
#include "StageId.h"

boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> RenderDataManager::ms_spDefaultIBLPicture;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> RenderDataManager::ms_spRgbTablePicture;

std::vector<std::unique_ptr<SHLightFieldData>> RenderDataManager::ms_SHLFs;
std::vector<std::unique_ptr<IBLProbeData>> RenderDataManager::ms_IBLProbes;

std::set<const SHLightFieldData*, DistanceComparePointer<const SHLightFieldData*>> RenderDataManager::ms_SHLFsInFrustum;
std::set<const IBLProbeData*, DistanceComparePointer<const IBLProbeData*>> RenderDataManager::ms_IBLProbesInFrustum;
std::set<LocalLightData, DistanceCompareReference<LocalLightData>> RenderDataManager::ms_LocalLightsInFrustum;

HOOK(void, __fastcall, CTerrainDirectorInitializeRenderData, 0x719310, void* This)
{
    originalCTerrainDirectorInitializeRenderData(This);

    RenderDataManager::ms_LocalLightsInFrustum.clear();
    RenderDataManager::ms_SHLFsInFrustum.clear();
    RenderDataManager::ms_IBLProbesInFrustum.clear();
    RenderDataManager::ms_spDefaultIBLPicture.reset();
    RenderDataManager::ms_spRgbTablePicture.reset();
    RenderDataManager::ms_SHLFs.clear();
    RenderDataManager::ms_IBLProbes.clear();

    Sonic::CFxScheduler* pScheduler = ((Sonic::CRenderDirectorFxPipeline*)(*Sonic::CGameDocument::ms_pInstance)->m_pMember->m_spRenderDirector.get())->m_pScheduler;

    pScheduler->GetPicture(RenderDataManager::ms_spDefaultIBLPicture, (StageId::get() + "_defaultibl").c_str());
    pScheduler->GetPicture(RenderDataManager::ms_spRgbTablePicture, (StageId::get() + "_rgb_table0").c_str());

    boost::shared_ptr<Hedgehog::Database::CRawData> spShlfData;
    (*Sonic::CGameDocument::ms_pInstance)->m_pMember->m_spDatabase->GetRawData(spShlfData, (StageId::get() + ".shlf").c_str(), 0);

    boost::shared_ptr<Hedgehog::Database::CRawData> spProbeData;
    (*Sonic::CGameDocument::ms_pInstance)->m_pMember->m_spDatabase->GetRawData(spProbeData, (StageId::get() + ".probe").c_str(), 0);

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

            SHLightFieldData data;

            data.m_ProbeCounts[0] = shlf.ProbeCounts[0];
            data.m_ProbeCounts[1] = shlf.ProbeCounts[1];
            data.m_ProbeCounts[2] = shlf.ProbeCounts[2];

            data.m_InverseMatrix = affine.inverse().matrix();
            data.m_Position = Eigen::Vector3f(shlf.Position[0], shlf.Position[1], shlf.Position[2]) / 10.0f;

            pScheduler->GetPicture(data.m_spPicture, shlf.Name);

            RenderDataManager::ms_SHLFs.push_back(std::make_unique<SHLightFieldData>(std::move(data)));
        }
    }

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

            IBLProbeData data;

            data.m_InverseMatrix = matrix.inverse();
            data.m_Position = Eigen::Vector3f(iblProbe.Position[0], iblProbe.Position[1], iblProbe.Position[2]) / 10.0f;
            data.m_Bias = iblProbe.Bias;

            // help how do I cull OBBs 
            const float scaleX = matrix.col(0).head<3>().norm();
            const float scaleY = matrix.col(1).head<3>().norm();
            const float scaleZ = matrix.col(2).head<3>().norm();

            data.m_Radius = std::max<float>(std::max<float>(scaleX, scaleY), scaleZ) / 2.0f * sqrtf(2.0f);

            pScheduler->GetPicture(data.m_spPicture, iblProbe.Name);

            RenderDataManager::ms_IBLProbes.push_back(std::make_unique<IBLProbeData>(std::move(data)));
        }
    }
}

HOOK(void, __fastcall, CRenderDirectorFxPipelineUpdateForRender, 0x1105F40, Sonic::CRenderDirectorFxPipeline* This)
{
    originalCRenderDirectorFxPipelineUpdateForRender(This);

    Sonic::CFxSceneRenderer* pSceneRenderer = (Sonic::CFxSceneRenderer*)This->m_pScheduler->m_pMisc->m_spSceneRenderer.get();

    RenderDataManager::ms_LocalLightsInFrustum.clear();
    RenderDataManager::ms_SHLFsInFrustum.clear();
    RenderDataManager::ms_IBLProbesInFrustum.clear();

    if (!pSceneRenderer->m_pCamera)
        return;

    const Frustum frustum(pSceneRenderer->m_pCamera->m_Projection * pSceneRenderer->m_pCamera->m_View);

    if (pSceneRenderer->m_pLightManager->m_pStaticLightContext != nullptr)
    {
        Hedgehog::Mirage::CLightListData* pLightListData =
            pSceneRenderer->m_pLightManager->m_pStaticLightContext->m_spLightListData.get();

        for (auto it = pLightListData->m_Lights.m_pBegin; it != pLightListData->m_Lights.m_pEnd; it++)
        {
            if ((*it)->m_Type != Hedgehog::Mirage::eLightType_Omni)
                continue;

            const float distance = ((*it)->m_Position - pSceneRenderer->m_pCamera->m_Position).squaredNorm();

            if (distance < 100000 && frustum.intersects((*it)->m_Position, (*it)->m_Range.w()))
                RenderDataManager::ms_LocalLightsInFrustum.insert({ *it, distance });
        }
    }

    for (auto& shlf : RenderDataManager::ms_SHLFs)
    {
        shlf->m_Distance = (pSceneRenderer->m_pCamera->m_Position - shlf->m_Position).squaredNorm();
        RenderDataManager::ms_SHLFsInFrustum.insert(shlf.get());
    }

    for (auto& probe : RenderDataManager::ms_IBLProbes)
    {
        if (!frustum.intersects(probe->m_Position, probe->m_Radius))
            continue;

        probe->m_Distance = (pSceneRenderer->m_pCamera->m_Position - probe->m_Position).squaredNorm();
        RenderDataManager::ms_IBLProbesInFrustum.insert(probe.get());
    }
}

bool RenderDataManager::enabled = false;

void RenderDataManager::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CTerrainDirectorInitializeRenderData);
    INSTALL_HOOK(CRenderDirectorFxPipelineUpdateForRender);
}
