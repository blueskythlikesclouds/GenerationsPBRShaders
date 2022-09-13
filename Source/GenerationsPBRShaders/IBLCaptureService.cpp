#ifdef ENABLE_IBL_CAPTURE_SERVICE

#include "IBLCaptureService.h"
#include "DllMods/Source/GenerationsFreeCamera/Camera.h"

bool* const g_ExecuteFrustumCulling = (bool*)0x1A42FFE;

bool IBLCaptureService::enabled;

std::unique_ptr<DirectX::ScratchImage> IBLCaptureService::result;
Eigen::AlignedVector3f IBLCaptureService::position;
size_t IBLCaptureService::faceIndex;
IBLCaptureMode IBLCaptureService::mode;

void IBLCaptureService::capture(const Eigen::AlignedVector3f& position, const size_t resolution, IBLCaptureMode mode)
{
    if (result != nullptr)
        return;

    result = std::make_unique<DirectX::ScratchImage>();
    result->InitializeCube(DXGI_FORMAT_R16G16B16A16_FLOAT, resolution, resolution, 1, 1);
    IBLCaptureService::position = position;
    IBLCaptureService::mode = mode;
    faceIndex = 0;

    // Disable frustum culling since Generations doesn't update the view frustum fast enough.
    *g_ExecuteFrustumCulling = false;
}

std::unique_ptr<DirectX::ScratchImage> IBLCaptureService::getResultIfReady()
{
    return faceIndex >= 6 ? std::move(result) : nullptr;
}

namespace IBLCapture
{
    const Eigen::AlignedVector3f TARGETS[] =
    {
        Eigen::AlignedVector3f(1.0f,  0.0f,  0.0f),
        Eigen::AlignedVector3f(-1.0f, 0.0f,  0.0f),
        Eigen::AlignedVector3f(0.0f,  1.0f,  0.0f),
        Eigen::AlignedVector3f(0.0f, -1.0f,  0.0f),
        Eigen::AlignedVector3f(0.0f,  0.0f, -1.0f),
        Eigen::AlignedVector3f(0.0f,  0.0f,  1.0f)
    };

    const Eigen::AlignedVector3f UP_DIRECTIONS[] =
    {
        Eigen::AlignedVector3f(0.0f,  1.0f,  0.0f),
        Eigen::AlignedVector3f(0.0f,  1.0f,  0.0f),
        Eigen::AlignedVector3f(0.0f,  0.0f,  1.0f),
        Eigen::AlignedVector3f(0.0f,  0.0f, -1.0f),
        Eigen::AlignedVector3f(0.0f,  1.0f,  0.0f),
        Eigen::AlignedVector3f(0.0f,  1.0f,  0.0f)
    };

    HOOK(void, __fastcall, CCameraUpdate, 0x10FB770, Camera* This, void* Edx, void* pUpdateInfo)
    {
        if (IBLCaptureService::faceIndex >= 6 || IBLCaptureService::result == nullptr)
        {
            originalCCameraUpdate(This, Edx, pUpdateInfo);
            return;
        }

        This->viewMatrix = Eigen::CreateLookAtMatrix<Eigen::AlignedVector3f>(IBLCaptureService::position, 
            IBLCaptureService::position + TARGETS[IBLCaptureService::faceIndex], UP_DIRECTIONS[IBLCaptureService::faceIndex]);

        This->fieldOfView = DEGREES_TO_RADIANS(90.0f);

        This->projectionMatrix = Eigen::CreatePerspectiveMatrix(This->fieldOfView, 1.0f, 0.001f, 10000.0f);

        This->position = IBLCaptureService::position;

        This->direction = TARGETS[IBLCaptureService::faceIndex];

        This->aspectRatio = 1.0f;
    }

    ComPtr<ID3D11Texture2D> stagingTex;

    HOOK(void, __fastcall, CFxRenderGameSceneExecute, Sonic::fpCFxRenderGameSceneExecute, Sonic::CFxRenderGameScene* This)
    {
        const bool capturing = IBLCaptureService::faceIndex < 6 && IBLCaptureService::result != nullptr;
        const hh::ygg::ERenderCategory tmpRenderCategories = This->m_RenderCategories;

        if (capturing)
            This->m_RenderCategories = IBLCaptureService::mode == IBLCaptureMode::DefaultIBL ? hh::ygg::eRenderCategory_Sky : hh::ygg::eRenderCategory_Terrain;
        
        originalCFxRenderGameSceneExecute(This);

        if (!capturing)
        {
            *g_ExecuteFrustumCulling = true;
            return;
        }

        const auto dxpDevice = This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice;
        const auto colorTex = GenerationsD3D11::GetResource(This->m_spColorTex->m_pD3DTexture);

        if (!stagingTex)
        {
            D3D11_TEXTURE2D_DESC desc;
            reinterpret_cast<ID3D11Texture2D*>(colorTex)->GetDesc(&desc);

            desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

            GenerationsD3D11::GetDevice(dxpDevice)->CreateTexture2D(&desc, nullptr, stagingTex.ReleaseAndGetAddressOf());
        }

        const GenerationsD3D11::LockGuard lock(dxpDevice);
        const auto deviceContext = GenerationsD3D11::GetDeviceContext(dxpDevice);

        D3D11_MAPPED_SUBRESOURCE mappedSubresource;

        deviceContext->CopyResource(stagingTex.Get(), colorTex);
        deviceContext->Map(stagingTex.Get(), 0, D3D11_MAP_READ, 0, &mappedSubresource);

        DirectX::Image image{};
        image.width = This->m_spColorTex->m_CreationParams.Width;
        image.height = This->m_spColorTex->m_CreationParams.Height;
        image.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        image.pixels = (uint8_t*)mappedSubresource.pData;
        DirectX::ComputePitch(image.format, image.width, image.height, image.rowPitch, image.slicePitch);

        const DirectX::TexMetadata& metadata = IBLCaptureService::result->GetMetadata();

        DirectX::ScratchImage tmpScratchImage;
        DirectX::Resize(image, metadata.width, metadata.height, DirectX::TEX_FILTER_BOX, tmpScratchImage);

        uint8_t* srcPixels = tmpScratchImage.GetPixels();
        uint8_t* dstPixels = IBLCaptureService::result->GetImage(0, IBLCaptureService::faceIndex, 0)->pixels;

        memcpy(dstPixels, srcPixels, tmpScratchImage.GetPixelsSize());
        deviceContext->Unmap(stagingTex.Get(), 0);

        IBLCaptureService::faceIndex++;

        This->m_RenderCategories = tmpRenderCategories;

        This->m_pScheduler->m_pMisc->m_spSceneRenderer->m_pCamera->m_AspectRatio = 16.0f / 9.0f;
    }

    void applyPatches()
    {
        INSTALL_HOOK(CCameraUpdate);
        INSTALL_HOOK(CFxRenderGameSceneExecute);
    }
}

void IBLCaptureService::applyPatches()
{
    if (enabled)
        return;

    enabled = true;

    IBLCapture::applyPatches();
}

#endif