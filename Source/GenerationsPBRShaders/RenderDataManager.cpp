#include "RenderDataManager.h"
#include "IBLProbe.h"
#include "SHLightField.h"
#include "StageId.h"
#include "NodeBVH.h"

boost::shared_ptr<hh::ygg::CYggPicture> RenderDataManager::defaultIBLPicture;
boost::shared_ptr<hh::ygg::CYggPicture> RenderDataManager::rgbTablePicture;

std::vector<std::unique_ptr<SHLightFieldData>> RenderDataManager::shlfs;
std::vector<std::unique_ptr<IBLProbeData>> RenderDataManager::iblProbes;
std::vector<std::unique_ptr<LightMotionData>> RenderDataManager::lightMotions;
std::vector<std::unique_ptr<LocalLightData>> RenderDataManager::localLights;

std::vector<const SHLightFieldData*> RenderDataManager::shlfsInFrustum;
std::vector<const IBLProbeData*> RenderDataManager::iblProbesInFrustum;
std::vector<const LocalLightData*> RenderDataManager::localLightsInFrustum;

NodeBVH RenderDataManager::nodeBVH;

HOOK(void, __fastcall, CTerrainDirectorInitializeRenderData, 0x719310, void* This)
{
    originalCTerrainDirectorInitializeRenderData(This);

    RenderDataManager::localLightsInFrustum.clear();
    RenderDataManager::shlfsInFrustum.clear();
    RenderDataManager::iblProbesInFrustum.clear();
    RenderDataManager::defaultIBLPicture.reset();
    RenderDataManager::rgbTablePicture.reset();
    RenderDataManager::shlfs.clear();
    RenderDataManager::iblProbes.clear();
    RenderDataManager::lightMotions.clear();
    RenderDataManager::localLights.clear();
    RenderDataManager::nodeBVH.reset();

    globalUsePBR = false;

    Sonic::CFxScheduler* scheduler = ((Sonic::CRenderDirectorFxPipeline*)(*Sonic::CGameDocument::ms_pInstance)->m_pMember->m_spRenderDirector.get())->m_pScheduler;

    scheduler->GetPicture(RenderDataManager::defaultIBLPicture, (StageId::get() + "_defaultibl").c_str());
    scheduler->GetPicture(RenderDataManager::rgbTablePicture, (StageId::get() + "_rgb_table0").c_str());

    hh::db::CDatabase* database = (*Sonic::CGameDocument::ms_pInstance)->m_pMember->m_spDatabase.get();

    boost::shared_ptr<hh::db::CRawData> shlfData;
    database->GetRawData(shlfData, (StageId::get() + ".shlf").c_str(), 0);

    boost::shared_ptr<hh::db::CRawData> probeData;
    database->GetRawData(probeData, (StageId::get() + ".probe").c_str(), 0);

    globalUsePBR = RenderDataManager::defaultIBLPicture != nullptr || RenderDataManager::rgbTablePicture != nullptr ||
        shlfData != nullptr || probeData != nullptr;

    if (shlfData && shlfData->IsMadeAll())
    {
        hlBINAV2Fix(shlfData->m_spData.get(), shlfData->m_DataSize);

        SHLightFieldSet* shlfSet = (SHLightFieldSet*)hlBINAV2GetData(shlfData->m_spData.get());

        for (uint32_t i = 0; i < shlfSet->shlfCount; i++)
        {
            const SHLightField& shlf = shlfSet->shlfs[i];

            Eigen::Affine3f affine =
                Eigen::Translation3f(shlf.position[0], shlf.position[1], shlf.position[2]) *
                Eigen::AngleAxisf(shlf.rotation[0], Eigen::Vector3f::UnitX()) *
                Eigen::AngleAxisf(shlf.rotation[1], Eigen::Vector3f::UnitY()) *
                Eigen::AngleAxisf(shlf.rotation[2], Eigen::Vector3f::UnitZ()) *
                Eigen::Scaling(shlf.scale[0], shlf.scale[1], shlf.scale[2]);

            SHLightFieldData data;

            data.probeCounts[0] = shlf.probeCounts[0];
            data.probeCounts[1] = shlf.probeCounts[1];
            data.probeCounts[2] = shlf.probeCounts[2];

            data.obb = OBB(affine.matrix(), 0.5f);
            data.inverseMatrix = affine.inverse().matrix().transpose();
            data.position = Eigen::AlignedVector3f(shlf.position[0], shlf.position[1], shlf.position[2]) / 10.0f;

            const AABB aabb = getAABBFromOBB(affine.matrix(), 0.5f, 1.0f / 10.0f);
            data.radius = getAABBRadius(aabb);

            scheduler->GetPicture(data.picture, shlf.name);

            RenderDataManager::shlfs.push_back(std::make_unique<SHLightFieldData>(std::move(data)));

            if (RenderDataManager::shlfs.back()->picture)
                RenderDataManager::nodeBVH.add(NodeType::SHLightField, RenderDataManager::shlfs.back().get(), aabb);
        }

        RenderDataManager::shlfsInFrustum.reserve(RenderDataManager::shlfs.size());
    }

    if (probeData && probeData->IsMadeAll())
    {
        hlBINAV2Fix(probeData->m_spData.get(), probeData->m_DataSize);

        IBLProbeSet* iblProbeSet = (IBLProbeSet*)hlBINAV2GetData(probeData->m_spData.get());

        for (uint32_t i = 0; i < iblProbeSet->probeCount; i++)
        {
            const IBLProbe& iblProbe = iblProbeSet->probes[i];

            Eigen::Matrix4f matrix;

            for (uint32_t x = 0; x < 4; x++)
                for (uint32_t y = 0; y < 4; y++)
                    matrix(x, y) = iblProbe.matrix[y][x];

            IBLProbeData data;

            data.name = iblProbe.name;
            data.obb = OBB(matrix, 1);
            data.inverseMatrix = matrix.inverse().transpose();
            data.position = Eigen::AlignedVector3f(iblProbe.position[0], iblProbe.position[1], iblProbe.position[2]) / 10.0f;
            data.bias = iblProbe.bias;

            const AABB aabb = getAABBFromOBB(matrix, 1.0f, 1.0f);
            data.radius = getAABBRadius(aabb);

            scheduler->GetPicture(data.picture, iblProbe.name);

            RenderDataManager::iblProbes.push_back(std::make_unique<IBLProbeData>(std::move(data)));

            if (RenderDataManager::iblProbes.back()->picture)
                RenderDataManager::nodeBVH.add(NodeType::IBLProbe, RenderDataManager::iblProbes.back().get(), aabb);
        }

        RenderDataManager::iblProbesInFrustum.reserve(RenderDataManager::iblProbes.size());
    }

    hh::mot::CMotionDatabaseWrapper motionWrapper(database);

    boost::shared_ptr<hh::db::CRawData> rawData;
    database->GetRawData(rawData, "Autodraw.txt", 0);

    if (rawData && rawData->IsMadeAll())
    {
        char* autodraw = (char*)rawData->m_spData.get();

        for (size_t i = 0; i < rawData->m_DataSize;)
        {
            if (autodraw[i] == '\n' || autodraw[i] == '\r')
            {
                i++;
                continue;
            }

            size_t j;
            for (j = i; j < rawData->m_DataSize; j++)
            {
                if (autodraw[j] == '\n' || autodraw[j] == '\r')
                    break;
            }

            char line[256];
            strncpy(line, autodraw + i, j - i);

            i = j + 1;

            char* extension = strstr(line, ".lit-anim");
            if (extension == nullptr)
                continue;

            *extension = '\0';

            boost::shared_ptr<hh::mot::CLightMotionData> lightMotionData;
            motionWrapper.GetLightMotionData(lightMotionData, line, 0);

            if (!lightMotionData)
                continue;

            LightMotionData data;
            data.data = lightMotionData;
            data.name = line;

            RenderDataManager::lightMotions.push_back(std::make_unique<LightMotionData>(std::move(data)));
        }
    }

    hh::mr::CMirageDatabaseWrapper mirageWrapper(database);

    boost::shared_ptr<hh::mr::CLightListData> lightListData;
    mirageWrapper.GetLightListData(lightListData, "light-list", 0);

    if (lightListData && lightListData->IsMadeAll())
    {
        for (auto it = lightListData->m_Lights.m_pBegin; it != lightListData->m_Lights.m_pEnd; it++)
        {
            if ((*it)->m_Type != hh::mr::eLightType_Point)
                continue;

            LocalLightData data;
            data.lightData = *it;
            data.position = (*it)->m_Position;
            data.color = (*it)->m_Color.head<3>();
            data.range = (*it)->m_Range;
            data.lightMotionData = nullptr;

            for (auto& motionData : RenderDataManager::lightMotions)
            {
                if (strstr((*it)->m_TypeAndName.c_str(), motionData->name.c_str()) == nullptr)
                    continue;

                data.lightMotionData = motionData.get();
                break;
            }

            RenderDataManager::localLights.push_back(std::make_unique<LocalLightData>(std::move(data)));

            const float range = (*it)->m_Range.w();

            AABB aabb;
            aabb.min() = (*it)->m_Position - Eigen::AlignedVector3f(range, range, range);
            aabb.max() = (*it)->m_Position + Eigen::AlignedVector3f(range, range, range);

            RenderDataManager::nodeBVH.add(NodeType::LocalLight, RenderDataManager::localLights.back().get(), aabb);
        }

        RenderDataManager::localLightsInFrustum.reserve(RenderDataManager::localLights.size());
    }

    RenderDataManager::nodeBVH.build();

    // Try env_brdf as a last resort.
    if (!globalUsePBR)
    {
        boost::shared_ptr<hh::mr::CPictureData> envBrdfPicture;
        mirageWrapper.GetPictureData(envBrdfPicture, "env_brdf", 0);
        globalUsePBR |= envBrdfPicture != nullptr;
    }
}

void renderDataManagerNodeBVHTraverseCallback(void* userData, const Node& node)
{
    Sonic::CCamera* pCamera = (Sonic::CCamera*)userData;

    switch (node.type)
    {
    case NodeType::SHLightField:
    {
        SHLightFieldData* shlf = (SHLightFieldData*)node.data;

        if (!shlf->picture->m_spPictureData->IsMadeAll())
            break;

        shlf->distance = (pCamera->m_MyCamera.m_Position - shlf->position).squaredNorm();

        if (shlf->distance > SceneEffect::culling.shLightFieldCullingRange * SceneEffect::culling.shLightFieldCullingRange)
            break;

        shlf->distance /= shlf->radius * shlf->radius;

        RenderDataManager::shlfsInFrustum.push_back(shlf);
        break;
    }

    case NodeType::IBLProbe:
    {
        IBLProbeData* probe = (IBLProbeData*)node.data;

        if (!probe->picture->m_spPictureData->IsMadeAll())
            break;

        probe->distance = (pCamera->m_MyCamera.m_Position - probe->position).squaredNorm();

        if (probe->distance > SceneEffect::culling.iblProbeCullingRange * SceneEffect::culling.iblProbeCullingRange)
            break;

        probe->distance /= probe->radius * probe->radius;

        RenderDataManager::iblProbesInFrustum.push_back(probe);
        break;
    }

    case NodeType::LocalLight:
    {
        LocalLightData* localLight = (LocalLightData*)node.data;

        localLight->distance = (pCamera->m_MyCamera.m_Position - localLight->position).squaredNorm();

        if (localLight->distance > SceneEffect::culling.localLightCullingRange * SceneEffect::culling.localLightCullingRange)
            break;

        if (localLight->lightMotionData && localLight->lightMotionData->data && localLight->lightMotionData->data->IsMadeAll())
        {
            // I have no idea if this is correct at all.
            // Simply passing the data as color does not work right.
            const float colorScale = localLight->lightMotionData->valueData.m_Data[0x10] / 
                localLight->lightData->m_Color.head<3>().maxCoeff();

            localLight->color = localLight->lightData->m_Color.head<3>() * colorScale;

            // If the animation made the color almost black, we have no reason to process this local light in the shader.
            if (localLight->color.maxCoeff() < 0.001f)
                break;
        }

        RenderDataManager::localLightsInFrustum.push_back(localLight);
        break;
    }

    default:
        break;
    }
}

HOOK(bool, __fastcall, CRenderDirectorFxPipelineUpdate, 0x1105F20, Sonic::CRenderDirectorFxPipeline* This, void* Edx, const hh::fnd::SUpdateInfo& updateInfo)
{
    if (!globalUsePBR)
        return originalCRenderDirectorFxPipelineUpdate(This, Edx, updateInfo);

    if (updateInfo.Category == "0")
    {            
        static double lightMotionTime = 0.0f;

        for (auto& motionData : RenderDataManager::lightMotions)
        {
            if (!motionData->data || !motionData->data->IsMadeAll())
                continue;

            const hh::mot::CLightSubMotionData& subMotionData = motionData->data->m_SubMotions[0];

            const double subMotionFrameCount = subMotionData.m_EndFrame - subMotionData.m_StartFrame;
            const double currentFrame = fmod(lightMotionTime * subMotionData.m_FrameRate, subMotionFrameCount);

            motionData->data->Step(0, (float)currentFrame, motionData->valueData);
        }

        lightMotionTime += updateInfo.DeltaTime;
    }

    else if (updateInfo.Category == "b")
    {
        const boost::shared_ptr<Sonic::CCamera> spCamera = Sonic::CGameDocument::GetInstance()->GetWorld()->GetCamera();
        const Frustum frustum(spCamera->m_MyCamera.m_Projection * spCamera->m_MyCamera.m_View.matrix());

        const SHLightFieldData* frontShlf =
            !RenderDataManager::shlfsInFrustum.empty() ? RenderDataManager::shlfsInFrustum.front() : nullptr;

        RenderDataManager::shlfsInFrustum.clear();
        RenderDataManager::iblProbesInFrustum.clear();
        RenderDataManager::localLightsInFrustum.clear();
        RenderDataManager::nodeBVH.traverse(frustum, spCamera.get(), renderDataManagerNodeBVHTraverseCallback);

        std::stable_sort(RenderDataManager::shlfsInFrustum.begin(), RenderDataManager::shlfsInFrustum.end(),
            [](const auto& lhs, const auto& rhs) { return lhs->distance < rhs->distance;  });

        std::stable_sort(RenderDataManager::iblProbesInFrustum.begin(), RenderDataManager::iblProbesInFrustum.end(),
            [](const auto& lhs, const auto& rhs) { return lhs->distance < rhs->distance;  });

        std::stable_sort(RenderDataManager::localLightsInFrustum.begin(), RenderDataManager::localLightsInFrustum.end(),
            [](const auto& lhs, const auto& rhs) { return lhs->distance < rhs->distance;  });

        if (RenderDataManager::shlfsInFrustum.empty() && frontShlf != nullptr)
            RenderDataManager::shlfsInFrustum.push_back(frontShlf);

        else if (RenderDataManager::shlfsInFrustum.empty() && RenderDataManager::shlfs.size() == 1)
            RenderDataManager::shlfsInFrustum.push_back(RenderDataManager::shlfs[0].get());
    }

    return originalCRenderDirectorFxPipelineUpdate(This, Edx, updateInfo);
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
