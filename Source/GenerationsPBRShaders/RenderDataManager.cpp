#include "RenderDataManager.h"
#include "IBLProbe.h"
#include "SHLightField.h"
#include "StageId.h"

boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> RenderDataManager::ms_spDefaultIBLPicture;
boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> RenderDataManager::ms_spRgbTablePicture;

std::vector<std::unique_ptr<SHLightFieldData>> RenderDataManager::ms_SHLFs;
std::vector<std::unique_ptr<IBLProbeData>> RenderDataManager::ms_IBLProbes;
std::vector<std::unique_ptr<LightMotionData>> RenderDataManager::ms_LightMotions;

RenderDataPtrSet<SHLightFieldData> RenderDataManager::ms_SHLFsInFrustum;
RenderDataPtrSet<IBLProbeData> RenderDataManager::ms_IBLProbesInFrustum;
RenderDataSet<LocalLightData> RenderDataManager::ms_LocalLightsInFrustum;

float GetRadius(const Eigen::Matrix4f& matrix, const float relativeBounds)
{
    const Eigen::Vector4f min = { -relativeBounds, -relativeBounds, -relativeBounds, 1.0f };
    const Eigen::Vector4f max = { +relativeBounds, +relativeBounds, +relativeBounds, 1.0f };

    Box aabb;
    aabb.extend((matrix * min).head<3>());
    aabb.extend((matrix * max).head<3>());

    float radius = 0.0f;
    for (size_t i = 0; i < 8; i++)
        radius = std::max<float>(radius, (aabb.center() - aabb.corner((Box::CornerType)i)).norm());

    return radius;
}

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

            data.m_Radius = GetRadius(affine.matrix(), 0.5f) / 10.0f;

            data.m_InverseMatrix = affine.inverse().matrix();
            data.m_Position = Eigen::Vector3f(shlf.Position[0], shlf.Position[1], shlf.Position[2]) / 10.0f;

            pScheduler->GetPicture(data.m_spPicture, shlf.Name);

            if (data.m_spPicture != nullptr && data.m_spPicture->m_spPictureData)
                data.m_spPicture->m_spPictureData->Validate();

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
            data.m_Radius = GetRadius(matrix, 1);

            pScheduler->GetPicture(data.m_spPicture, iblProbe.Name);

            if (data.m_spPicture != nullptr && data.m_spPicture->m_spPictureData)
                data.m_spPicture->m_spPictureData->Validate();

            RenderDataManager::ms_IBLProbes.push_back(std::make_unique<IBLProbeData>(std::move(data)));
        }
    }

    Hedgehog::Motion::CMotionDatabaseWrapper wrapper(pDatabase);

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
        wrapper.GetLightMotionData(spLightMotionData, line, 0);

        if (!spLightMotionData)
            continue;

        spLightMotionData->Validate();

        LightMotionData data;
        data.m_spData = spLightMotionData;
        data.m_Name = line;

        RenderDataManager::ms_LightMotions.push_back(std::make_unique<LightMotionData>(std::move(data)));
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
        const Frustum frustum(pSceneRenderer->m_pCamera->m_Projection * pSceneRenderer->m_pCamera->m_View);

        if (pSceneRenderer->m_pLightManager->m_pStaticLightContext != nullptr)
        {
            RenderDataManager::ms_LocalLightsInFrustum.clear();

            Hedgehog::Mirage::CLightListData* pLightListData =
                pSceneRenderer->m_pLightManager->m_pStaticLightContext->m_spLightListData.get();

            for (auto it = pLightListData->m_Lights.m_pBegin; it != pLightListData->m_Lights.m_pEnd; it++)
            {
                if ((*it)->m_Type != Hedgehog::Mirage::eLightType_Omni)
                    continue;

                const float distance = ((*it)->m_Position - pSceneRenderer->m_pCamera->m_Position).squaredNorm();

                if (distance < 100000 && frustum.intersects((*it)->m_Position, (*it)->m_Range.w()))
                {
                    LightMotionData* pMotionData = nullptr;

                    for (auto& upMotionData : RenderDataManager::ms_LightMotions)
                    {
                        if (strstr((*it)->m_TypeAndName.m_pStr, upMotionData->m_Name.c_str()) == nullptr)
                            continue;

                        pMotionData = upMotionData.get();
                        break;
                    }

                    Eigen::Vector3f position = (*it)->m_Position;
                    Eigen::Vector3f color = (*it)->m_Color.head<3>();
                    Eigen::Vector4f range = (*it)->m_Range;

                    if (pMotionData != nullptr)
                    {
                        // I have no idea if this is correct at all.
                        // Simply passing the data as color does not work right.
                        const float colorScale = pMotionData->m_ValueData.m_Data[0x10] / color.dot(Eigen::Vector3f(0.2126f, 0.7152f, 0.0722f));

                        color.x() *= colorScale;
                        color.y() *= colorScale;
                        color.z() *= colorScale;

                        // If the animation made the color almost black, we have no reason to process this local light in the shader.
                        if (color.maxCoeff() < 0.001f)
                            continue;

                        range.x() = pMotionData->m_ValueData.m_Data[0x14];
                        range.y() = pMotionData->m_ValueData.m_Data[0x15];
                        range.z() = pMotionData->m_ValueData.m_Data[0x16];
                        range.w() = pMotionData->m_ValueData.m_Data[0x17];
                    }

                    RenderDataManager::ms_LocalLightsInFrustum.insert({ position, color, range, distance });
                }
            }
        }

        RenderDataManager::ms_SHLFsInFrustum.clear();

        for (auto& shlf : RenderDataManager::ms_SHLFs)
        {
            shlf->m_Distance = (pSceneRenderer->m_pCamera->m_Position - shlf->m_Position).squaredNorm() / (shlf->m_Radius * shlf->m_Radius);
            RenderDataManager::ms_SHLFsInFrustum.insert(shlf.get());
        }

        RenderDataManager::ms_IBLProbesInFrustum.clear();

        for (auto& probe : RenderDataManager::ms_IBLProbes)
        {
            if (!frustum.intersects(probe->m_Position, probe->m_Radius))
                continue;

            probe->m_Distance = (pSceneRenderer->m_pCamera->m_Position - probe->m_Position).squaredNorm() / (probe->m_Radius * probe->m_Radius);
            RenderDataManager::ms_IBLProbesInFrustum.insert(probe.get());
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
