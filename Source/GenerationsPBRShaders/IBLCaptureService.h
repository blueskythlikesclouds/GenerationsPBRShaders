#pragma once

class IBLCaptureService
{
    static bool enabled;

public:
    static std::unique_ptr<DirectX::ScratchImage> result;
    static Eigen::Vector3f position;
    static Eigen::Matrix4f matrix;
    static size_t index;

    static void capture(const Eigen::Vector3f& position, const Eigen::Matrix4f& matrix, size_t resolution);
    static std::unique_ptr<DirectX::ScratchImage> getResultIfReady();

    static void applyPatches();
};
