#pragma once

class IBLCaptureService
{
    static bool enabled;

public:
    static std::unique_ptr<DirectX::ScratchImage> result;
    static Eigen::Vector3f position;
    static size_t faceIndex;
    static bool includeSky;

    static void capture(const Eigen::Vector3f& position, size_t resolution, bool includeSky);
    static std::unique_ptr<DirectX::ScratchImage> getResultIfReady();

    static void applyPatches();
};
