#include "RenderDataManager.h"
#include "IBLProbe.h"
#include "SHLightField.h"
#include "StageId.h"
#include "NodeBVH.h"

boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> RenderDataManager::ms_spDefaultIBLPicture;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> RenderDataManager::ms_spRgbTablePicture;

std::vector<std::unique_ptr<SHLightFieldData>> RenderDataManager::ms_SHLFs;
std::vector<std::unique_ptr<IBLProbeData>> RenderDataManager::ms_IBLProbes;
std::vector<std::unique_ptr<LightMotionData>> RenderDataManager::ms_LightMotions;
std::vector<std::unique_ptr<LocalLightData>> RenderDataManager::ms_LocalLights;

std::vector<const SHLightFieldData*> RenderDataManager::ms_SHLFsInFrustum;
std::vector<const IBLProbeData*> RenderDataManager::ms_IBLProbesInFrustum;
std::vector<const LocalLightData*> RenderDataManager::ms_LocalLightsInFrustum;

NodeBVH RenderDataManager::ms_NodeBVH;

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
    RenderDataManager::ms_LightMotions.clear();
    RenderDataManager::ms_LocalLights.clear();
    RenderDataManager::ms_NodeBVH.reset();

    Sonic::CFxScheduler* pScheduler = ((Sonic::CRenderDirectorFxPipeline*)(*Sonic::CGameDocument::ms_pInstance)->m_pMember->m_spRenderDirector.get())->m_pScheduler;

    pScheduler->GetPicture(RenderDataManager::ms_spDefaultIBLPicture, (StageId::get() + "_defaultibl").c_str());
    pScheduler->GetPicture(RenderDataManager::ms_spRgbTablePicture, (StageId::get() + "_rgb_table0").c_str());

    Hedgehog::Database::CDatabase* pDatabase = (*Sonic::CGameDocument::ms_pInstance)->m_pMember->m_spDatabase.get();

    boost::shared_ptr<Hedgehog::Database::CRawData> spShlfData;
    pDatabase->GetRawData(spShlfData, (StageId::get() + ".shlf").c_str(), 0);

    boost::shared_ptr<Hedgehog::Database::CRawData> spProbeData;
    pDatabase->GetRawData(spProbeData, (StageId::get() + ".probe").c_str(), 0);

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

            data.m_OBB = OBB(affine.matrix(), 0.5f);
            data.m_InverseMatrix = affine.inverse().matrix();
            data.m_Position = Eigen::Vector3f(shlf.Position[0], shlf.Position[1], shlf.Position[2]) / 10.0f;

            pScheduler->GetPicture(data.m_spPicture, shlf.Name);

            if (data.m_spPicture != nullptr && data.m_spPicture->m_spPictureData != nullptr)
                data.m_spPicture->m_spPictureData->Validate();

            RenderDataManager::ms_SHLFs.push_back(std::make_unique<SHLightFieldData>(std::move(data)));
            RenderDataManager::ms_NodeBVH.add(NodeType::SHLightField, RenderDataManager::ms_SHLFs.back().get(), getAABBFromOBB(affine.matrix(), 0.5f, 1.0f / 10.0f));
        }

        RenderDataManager::ms_SHLFsInFrustum.reserve(RenderDataManager::ms_SHLFs.size());
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
                    matrix(x, y) = iblProbe.Matrix[y][x];

            IBLProbeData data;

            data.m_OBB = OBB(matrix, 1);
            data.m_InverseMatrix = matrix.inverse().transpose();
            data.m_Position = Eigen::Vector3f(iblProbe.Position[0], iblProbe.Position[1], iblProbe.Position[2]) / 10.0f;
            data.m_Bias = iblProbe.Bias;

            pScheduler->GetPicture(data.m_spPicture, iblProbe.Name);

            if (data.m_spPicture != nullptr && data.m_spPicture->m_spPictureData != nullptr)
                data.m_spPicture->m_spPictureData->Validate();

            RenderDataManager::ms_IBLProbes.push_back(std::make_unique<IBLProbeData>(std::move(data)));
            RenderDataManager::ms_NodeBVH.add(NodeType::IBLProbe, RenderDataManager::ms_IBLProbes.back().get(), getAABBFromOBB(matrix, 1.0f, 1.0f));
        }

        RenderDataManager::ms_IBLProbesInFrustum.reserve(RenderDataManager::ms_IBLProbes.size());
    }

    Hedgehog::Motion::CMotionDatabaseWrapper motionWrapper(pDatabase);

    boost::shared_ptr<Hedgehog::Database::CRawData> spRawData;
    pDatabase->GetRawData(spRawData, "Autodraw.txt", 0);

    if (spRawData == nullptr || spRawData->m_spData == nullptr)
        return;

    char* pAutodraw = (char*)spRawData->m_spData.get();

    for (size_t i = 0; i < spRawData->m_DataSize;)
    {
        if (pAutodraw[i] == '\n' || pAutodraw[i] == '\r')
        {
            i++;
            continue;
        }

        size_t j;
        for (j = i; j < spRawData->m_DataSize; j++)
        {
            if (pAutodraw[j] == '\n' || pAutodraw[j] == '\r')
                break;
        }

        char line[256];
        strncpy(line, pAutodraw + i, j - i);

        i = j + 1;

        char* pExtension = strstr(line, ".lit-anim");
        if (pExtension == nullptr)
            continue;

        *pExtension = '\0';

        boost::shared_ptr<Hedgehog::Motion::CLightMotionData> spLightMotionData;
        motionWrapper.GetLightMotionData(spLightMotionData, line, 0);

        if (!spLightMotionData)
            continue;

        spLightMotionData->Validate();

        LightMotionData data;
        data.m_spData = spLightMotionData;
        data.m_Name = line;

        RenderDataManager::ms_LightMotions.push_back(std::make_unique<LightMotionData>(std::move(data)));
    }

    Hedgehog::Mirage::CMirageDatabaseWrapper mirageWrapper(pDatabase);

    boost::shared_ptr<Hedgehog::Mirage::CLightListData> spLightListData;
    mirageWrapper.GetLightListData(spLightListData, "light-list", 0);

    if (spLightListData != nullptr)
    {
        for (auto it = spLightListData->m_Lights.m_pBegin; it != spLightListData->m_Lights.m_pEnd; it++)
        {
            if ((*it)->m_Type != Hedgehog::Mirage::eLightType_Omni)
                continue;

            LocalLightData data;
            data.m_spLightData = *it;
            data.m_Position = (*it)->m_Position;
            data.m_Color = (*it)->m_Color.head<3>();
            data.m_Range = (*it)->m_Range;
            data.m_pLightMotionData = nullptr;

            for (auto& upMotionData : RenderDataManager::ms_LightMotions)
            {
                if (strstr((*it)->m_TypeAndName.m_pStr, upMotionData->m_Name.c_str()) == nullptr)
                    continue;

                data.m_pLightMotionData = upMotionData.get();
                break;
            }

            RenderDataManager::ms_LocalLights.push_back(std::make_unique<LocalLightData>(std::move(data)));

            // TODO: Compute this properly
            const float maxRange = (*it)->m_Range.maxCoeff();

            AABB aabb;
            aabb.min() = (*it)->m_Position - Eigen::Vector3f(maxRange, maxRange, maxRange);
            aabb.max() = (*it)->m_Position + Eigen::Vector3f(maxRange, maxRange, maxRange);

            RenderDataManager::ms_NodeBVH.add(NodeType::LocalLight, RenderDataManager::ms_LocalLights.back().get(), aabb);
        }

        RenderDataManager::ms_LocalLightsInFrustum.reserve(RenderDataManager::ms_LocalLights.size());
    }

    RenderDataManager::ms_NodeBVH.build();
}

void RenderDataManagerNodeBVHTraverseCallback(void* userData, const Node& node)
{
    Sonic::CFxSceneRenderer* pSceneRenderer = (Sonic::CFxSceneRenderer*)userData;

    switch (node.type)
    {
    case NodeType::SHLightField:
    {
        SHLightFieldData* shlf = (SHLightFieldData*)node.data;

        shlf->m_Distance = shlf->m_OBB.closestPointDistanceSquared(pSceneRenderer->m_pCamera->m_Position * 10.0f) / 100.0f;

        if (shlf->m_Distance > SceneEffect::Culling.SHLightFieldCullingRange * SceneEffect::Culling.SHLightFieldCullingRange)
            break;

        RenderDataManager::ms_SHLFsInFrustum.push_back(shlf);
        break;
    }

    case NodeType::IBLProbe:
    {
        IBLProbeData* probe = (IBLProbeData*)node.data;

        probe->m_Distance = probe->m_OBB.closestPointDistanceSquared(pSceneRenderer->m_pCamera->m_Position);
        if (probe->m_Distance > SceneEffect::Culling.IBLProbeCullingRange * SceneEffect::Culling.IBLProbeCullingRange)
            break;

        RenderDataManager::ms_IBLProbesInFrustum.push_back(probe);
        break;
    }

    case NodeType::LocalLight:
    {
        LocalLightData* localLight = (LocalLightData*)node.data;

        localLight->m_Distance = (pSceneRenderer->m_pCamera->m_Position - localLight->m_Position).squaredNorm();

        if (localLight->m_Distance > SceneEffect::Culling.LocalLightCullingRange * SceneEffect::Culling.LocalLightCullingRange)
            break;

        if (localLight->m_pLightMotionData != nullptr)
        {
            // I have no idea if this is correct at all.
            // Simply passing the data as color does not work right.
            const float colorScale = localLight->m_pLightMotionData->m_ValueData.m_Data[0x10] / 
                localLight->m_spLightData->m_Color.head<3>().dot(Eigen::Vector3f(0.2126f, 0.7152f, 0.0722f));

            localLight->m_Color = localLight->m_spLightData->m_Color.head<3>() * colorScale;

            // If the animation made the color almost black, we have no reason to process this local light in the shader.
            if (localLight->m_Color.maxCoeff() < 0.001f)
                break;

            localLight->m_Range.x() = localLight->m_pLightMotionData->m_ValueData.m_Data[0x14];
            localLight->m_Range.y() = localLight->m_pLightMotionData->m_ValueData.m_Data[0x15];
            localLight->m_Range.z() = localLight->m_pLightMotionData->m_ValueData.m_Data[0x16];
            localLight->m_Range.w() = localLight->m_pLightMotionData->m_ValueData.m_Data[0x17];
        }

        RenderDataManager::ms_LocalLightsInFrustum.push_back(localLight);
        break;
    }

    default:
        break;
    }
}

HOOK(bool, __fastcall, CRenderDirectorFxPipelineUpdate, 0x1105F20, Sonic::CRenderDirectorFxPipeline* This, void* Edx, uint8_t* A2)
{
    char* pUpdateCommand = *(char**)(A2 + 8);

    Sonic::CRenderDirectorFxPipeline* pRenderDirector = (Sonic::CRenderDirectorFxPipeline*)((uint32_t)This - 4); // shifted ptr
    Sonic::CFxSceneRenderer* pSceneRenderer = (Sonic::CFxSceneRenderer*)pRenderDirector->m_pScheduler->m_pMisc->m_spSceneRenderer.get();

    if (!pSceneRenderer->m_pCamera)
        return originalCRenderDirectorFxPipelineUpdate(This, Edx, A2);

    if (strcmp(pUpdateCommand, "0") == 0)
    {            
        static double s_LightMotionTime = 0.0f;

        for (auto& upMotionData : RenderDataManager::ms_LightMotions)
        {
            const Hedgehog::Motion::CLightSubMotionData& subMotionData = upMotionData->m_spData->m_SubMotions[0];

            const double subMotionFrameCount = subMotionData.m_EndFrame - subMotionData.m_StartFrame;
            const double currentFrame = fmod(s_LightMotionTime * subMotionData.m_FrameRate, subMotionFrameCount);

            upMotionData->m_spData->Step(0, (float)currentFrame, upMotionData->m_ValueData);
        }

        s_LightMotionTime += *(float*)A2;
    }

    else if (strcmp(pUpdateCommand, "b") == 0)
    {
        static float s_ProbeUpdateTime = 0.0f;

        if (s_ProbeUpdateTime > 0.035f)
        {
            const Frustum frustum(pSceneRenderer->m_pCamera->m_Projection * pSceneRenderer->m_pCamera->m_View);

            const SHLightFieldData* pFrontSHLF =
                !RenderDataManager::ms_SHLFsInFrustum.empty() ? RenderDataManager::ms_SHLFsInFrustum.front() : nullptr;

            RenderDataManager::ms_SHLFsInFrustum.clear();
            RenderDataManager::ms_IBLProbesInFrustum.clear();
            RenderDataManager::ms_LocalLightsInFrustum.clear();
            RenderDataManager::ms_NodeBVH.traverse(frustum, pSceneRenderer, RenderDataManagerNodeBVHTraverseCallback);

            std::stable_sort(RenderDataManager::ms_SHLFsInFrustum.begin(), RenderDataManager::ms_SHLFsInFrustum.end(), 
                [](const auto& lhs, const auto& rhs) { return lhs->m_Distance < rhs->m_Distance;  });

            std::stable_sort(RenderDataManager::ms_IBLProbesInFrustum.begin(), RenderDataManager::ms_IBLProbesInFrustum.end(),
                [](const auto& lhs, const auto& rhs) { return lhs->m_Distance < rhs->m_Distance;  });

            std::stable_sort(RenderDataManager::ms_LocalLightsInFrustum.begin(), RenderDataManager::ms_LocalLightsInFrustum.end(),
                [](const auto& lhs, const auto& rhs) { return lhs->m_Distance < rhs->m_Distance;  });

            if (RenderDataManager::ms_SHLFsInFrustum.empty() && pFrontSHLF != nullptr)
                RenderDataManager::ms_SHLFsInFrustum.push_back(pFrontSHLF);

            s_ProbeUpdateTime = 0.0f;
        }

        else 
        {
            s_ProbeUpdateTime += *(float*)A2;
        }
    }

    return originalCRenderDirectorFxPipelineUpdate(This, Edx, A2);
}

bool RenderDataManager::enabled = false;

void RenderDataManager::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    INSTALL_HOOK(CTerrainDirectorInitializeRenderData);
    INSTALL_HOOK(CRenderDirectorFxPipelineUpdate);
}
