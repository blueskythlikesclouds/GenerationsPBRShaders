#pragma once

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

struct SHLightFieldData
{
    Eigen::Matrix4f m_InverseMatrix;
    Eigen::Vector3f m_Position;
    uint32_t m_ProbeCounts[3];
    float m_Distance;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> m_spPicture;
};

struct IBLProbeData
{
    Eigen::Matrix4f m_InverseMatrix;
    Eigen::Vector3f m_Position;
    float m_Bias;
    float m_Radius;
    float m_Distance;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> m_spPicture;
};

struct LightMotionData
{
    std::string m_Name;
    boost::shared_ptr<Hedgehog::Motion::CLightMotionData> m_spData;
    Hedgehog::Motion::CLightSubMotionValueData m_ValueData;
};

struct LocalLightData
{
    Eigen::Vector3f m_Position;
    Eigen::Vector3f m_Color;
    Eigen::Vector4f m_Range;
    float m_Distance;
};

template<typename T>
using RenderDataPtrSet = std::set<const T*, DistanceComparePointer<const T*>, boost::fast_pool_allocator<const T*>>;

template<typename T>
using RenderDataSet = std::set<T, DistanceCompareReference<T>, boost::fast_pool_allocator<T>>;

class RenderDataManager
{
    static bool enabled;

public:
    static boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> ms_spDefaultIBLPicture;
    static boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> ms_spRgbTablePicture;

    static std::vector<std::unique_ptr<SHLightFieldData>> ms_SHLFs;
    static std::vector<std::unique_ptr<IBLProbeData>> ms_IBLProbes;
    static std::vector<std::unique_ptr<LightMotionData>> ms_LightMotions;

    static RenderDataPtrSet<SHLightFieldData> ms_SHLFsInFrustum;
    static RenderDataPtrSet<IBLProbeData> ms_IBLProbesInFrustum;
    static RenderDataSet<LocalLightData> ms_LocalLightsInFrustum;

    static void applyPatches();
};
