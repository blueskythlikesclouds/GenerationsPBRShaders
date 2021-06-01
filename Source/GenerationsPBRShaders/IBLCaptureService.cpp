﻿#include "IBLCaptureService.h"
#include "DllMods/Source/GenerationsFreeCamera/Camera.h"

bool* const g_ExecuteFrustumCulling = (bool*)0x1A42FFE;

bool IBLCaptureService::enabled;

std::unique_ptr<DirectX::ScratchImage> IBLCaptureService::result;
Eigen::Vector3f IBLCaptureService::position;
size_t IBLCaptureService::faceIndex;
IBLCaptureMode IBLCaptureService::mode;

void IBLCaptureService::capture(const Eigen::Vector3f& position, const size_t resolution, IBLCaptureMode mode)
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
    const Eigen::Vector3f TARGETS[] =
    {
        Eigen::Vector3f(1.0f,  0.0f,  0.0f),
        Eigen::Vector3f(-1.0f, 0.0f,  0.0f),
        Eigen::Vector3f(0.0f,  1.0f,  0.0f),
        Eigen::Vector3f(0.0f, -1.0f,  0.0f),
        Eigen::Vector3f(0.0f,  0.0f, -1.0f),
        Eigen::Vector3f(0.0f,  0.0f,  1.0f)
    };

    const Eigen::Vector3f UP_DIRECTIONS[] =
    {
        Eigen::Vector3f(0.0f,  1.0f,  0.0f),
        Eigen::Vector3f(0.0f,  1.0f,  0.0f),
        Eigen::Vector3f(0.0f,  0.0f,  1.0f),
        Eigen::Vector3f(0.0f,  0.0f, -1.0f),
        Eigen::Vector3f(0.0f,  1.0f,  0.0f),
        Eigen::Vector3f(0.0f,  1.0f,  0.0f)
    };

    HOOK(void, __fastcall, CCameraUpdate, 0x10FB770, Camera* This, void* Edx, void* pUpdateInfo)
    {
        if (IBLCaptureService::faceIndex >= 6 || IBLCaptureService::result == nullptr)
        {
            originalCCameraUpdate(This, Edx, pUpdateInfo);
            return;
        }

        This->viewMatrix = Eigen::CreateLookAtMatrix<Eigen::Vector3f>(IBLCaptureService::position, 
            IBLCaptureService::position + TARGETS[IBLCaptureService::faceIndex], UP_DIRECTIONS[IBLCaptureService::faceIndex]);

        This->fieldOfView = DEGREES_TO_RADIANS(90.0f);

        This->projectionMatrix = Eigen::CreatePerspectiveMatrix(This->fieldOfView, 1.0f, 0.001f, 10000.0f);

        This->position = IBLCaptureService::position;

        This->direction = TARGETS[IBLCaptureService::faceIndex];

        This->aspectRatio = 1.0f;
    }

    struct SurfaceDeleter {
        void operator()(DX_PATCH::IDirect3DSurface9* pSurface) { pSurface->Release(); }
    };

    std::unique_ptr<DX_PATCH::IDirect3DSurface9, SurfaceDeleter> s_spScratchImageSurface;

    HOOK(void, __fastcall, CFxRenderGameSceneInitialize, Sonic::fpCFxRenderGameSceneInitialize, Sonic::CFxRenderGameScene* This)
    {
        originalCFxRenderGameSceneInitialize(This);

        DX_PATCH::IDirect3DSurface9* pSurface;

        This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->CreateOffscreenPlainSurface(
            This->m_spColorTex->m_CreationParams.Width, This->m_spColorTex->m_CreationParams.Height,
            (D3DFORMAT)This->m_spColorTex->m_CreationParams.Format, D3DPOOL_SYSTEMMEM, &pSurface, nullptr);

        s_spScratchImageSurface = std::unique_ptr<DX_PATCH::IDirect3DSurface9, SurfaceDeleter>(pSurface);
    }

    HOOK(void, __fastcall, CFxRenderGameSceneExecute, Sonic::fpCFxRenderGameSceneExecute, Sonic::CFxRenderGameScene* This)
    {
        const bool capturing = !IBLCaptureService::faceIndex < 6 && IBLCaptureService::result != nullptr;
        const Hedgehog::Yggdrasill::ERenderType tmpRenderTypes = This->m_RenderTypes;

        if (capturing)
            This->m_RenderTypes = IBLCaptureService::mode == IBLCaptureMode::DefaultIBL ? Hedgehog::Yggdrasill::eRenderType_Sky : Hedgehog::Yggdrasill::eRenderType_Terrain;
        
        originalCFxRenderGameSceneExecute(This);

        if (!capturing)
        {
            *g_ExecuteFrustumCulling = true;
            return;
        }

        boost::shared_ptr<Hedgehog::Yggdrasill::CYggSurface> spSrcSurface;
        This->m_spColorTex->GetSurface(spSrcSurface, 0, 0);

        This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->GetRenderTargetData(spSrcSurface->m_pD3DSurface, s_spScratchImageSurface.get());

        D3DLOCKED_RECT lockedRect;
        s_spScratchImageSurface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY);

        DirectX::Image image{};
        image.width = This->m_spColorTex->m_CreationParams.Width;
        image.height = This->m_spColorTex->m_CreationParams.Height;
        image.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        image.pixels = (uint8_t*)lockedRect.pBits;
        DirectX::ComputePitch(image.format, image.width, image.height, image.rowPitch, image.slicePitch);

        const DirectX::TexMetadata& metadata = IBLCaptureService::result->GetMetadata();

        DirectX::ScratchImage tmpScratchImage;
        DirectX::Resize(image, metadata.width, metadata.height, DirectX::TEX_FILTER_BOX, tmpScratchImage);

        uint8_t* srcPixels = tmpScratchImage.GetPixels();
        uint8_t* dstPixels = IBLCaptureService::result->GetImage(0, IBLCaptureService::faceIndex, 0)->pixels;

        memcpy(dstPixels, srcPixels, tmpScratchImage.GetPixelsSize());

        s_spScratchImageSurface->UnlockRect();

        IBLCaptureService::faceIndex++;

        This->m_RenderTypes = tmpRenderTypes;

        This->m_pScheduler->m_pMisc->m_spSceneRenderer->m_pCamera->m_AspectRatio = 16.0f / 9.0f;
    }

    void applyPatches()
    {
        INSTALL_HOOK(CCameraUpdate);
        INSTALL_HOOK(CFxRenderGameSceneInitialize);
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
