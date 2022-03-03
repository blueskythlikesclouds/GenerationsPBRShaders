#pragma once

template<typename T, size_t StartSlot, bool PS, bool Dynamic>
class alignas(16) ConstantBuffer : public T
{
    ComPtr<ID3D11Buffer> buffer;

public:
    void upload(void* dxpDevice);
};

struct SceneEffectCB
{
    float reflectanceOverride;
    float roughnessOverride;
    float ambientOcclusionOverride;
    float metalnessOverride;

    float giColorOverride[3];
    float giShadowOverride;

    float sggiParam[2];
    float rcpMiddleGray;
    float esmFactor;

    float shadowMapSize[2];

    BOOL useWhiteAlbedo;
    BOOL useFlatNormal;
    BOOL isEnableLUT;
    BOOL usePBR;
};

struct RenderDataCB
{
    Eigen::Matrix34f shLightFieldMatrices[3];
    Eigen::Vector4f shLightFieldParams[3];

    Eigen::Matrix34f iblProbeMatrices[24];
    Eigen::Vector4f iblProbeParams[24];
    float iblProbeLodParams[24];

    float localLightData[256];

    size_t iblProbeCount;
    float defaultIblLodParam;
    size_t localLightCount;
};

struct GITextureCB
{
    Eigen::Vector4f giAtlasParam;
    Eigen::Vector4f occlusionAtlasParam;
    BOOL isSg;
    BOOL hasOcclusion;
};

struct FilterCB
{
    union
    {
        // SSAO
        struct
        {
            int ssaoSampleCount;
            float ssaoRcpSampleCount;
            float ssaoRadius;
            float ssaoDistanceFade;
            float ssaoStrength;
        };

        // BoxBlur
        struct
        {
            float boxBlurSourceSize[2];
            float boxBlurDepthThreshold;
        };

        // Light
        struct
        {
            BOOL lightEnableSSAO;
        };

        // RLR
        struct
        {
            float rlrFramebufferSize[4];
            int rlrStepCount;
            float rlrMaxRoughness;
            float rlrRayLength;
            float rlrFade;
            float rlrAccuracyThreshold;
            float rlrSaturation;
            float rlrBrightness;
        };

        // ConvolutionFilter
        struct
        {
            float convolutionParam[4];
            float convolutionDirection[2];
        };

        // VolumetricLighting
        struct
        {
            int volumetricLightingSampleCount;
            float volumetricLightingRcpSampleCount;
            float volumetricLightingG;
            float volumetricLightingInScatteringScale;
        };

        // LUT
        struct
        {
            BOOL lutEnable;
        };
    };
};

extern ConstantBuffer<SceneEffectCB, 2, true, false> sceneEffectCB;
extern ConstantBuffer<RenderDataCB, 3, true, false> renderDataCB;
extern ConstantBuffer<GITextureCB, 4, true, true> giTextureCB;
extern ConstantBuffer<FilterCB, 5, true, true> filterCB;

template <typename T, size_t StartSlot, bool PS, bool Dynamic>
void ConstantBuffer<T, StartSlot, PS, Dynamic>::upload(void* dxpDevice)
{
    ID3D11DeviceContext* deviceContext = GenerationsD3D11::GetDeviceContext(dxpDevice);

    if (!buffer)
    {
        ID3D11Device* device = GenerationsD3D11::GetDevice(dxpDevice);

        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = (sizeof(T) + 16) & ~0xF;
        bufferDesc.Usage = Dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = Dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = this;

        device->CreateBuffer(&bufferDesc, &initialData, buffer.GetAddressOf());

        if (PS)
            deviceContext->PSSetConstantBuffers(StartSlot, 1, buffer.GetAddressOf());
        else
            deviceContext->VSSetConstantBuffers(StartSlot, 1, buffer.GetAddressOf());
    }
    else
    {
        const auto lock = GenerationsD3D11::LockGuard(dxpDevice);

        if (Dynamic)
        {
            D3D11_MAPPED_SUBRESOURCE mappedSubResource;
            deviceContext->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource);
            memcpy(mappedSubResource.pData, this, sizeof(T));
            deviceContext->Unmap(buffer.Get(), 0);
        }
        else
        {
            deviceContext->UpdateSubresource(buffer.Get(), 0, nullptr, this, 0, 0);
        }
    }
}