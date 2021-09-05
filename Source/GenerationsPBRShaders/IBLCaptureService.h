#pragma once

namespace DirectX
{
    class ScratchImage;
}

enum class IBLCaptureMode
{
    DefaultIBL,
    IBLProbe
};

class IBLCaptureService
{
    static bool enabled;

public:
    static std::unique_ptr<DirectX::ScratchImage> result;
    static Eigen::Vector3f position;
    static size_t faceIndex;
    static IBLCaptureMode mode;

    static void capture(const Eigen::Vector3f& position, size_t resolution, IBLCaptureMode mode);
    static std::unique_ptr<DirectX::ScratchImage> getResultIfReady();

    static void applyPatches();
};
