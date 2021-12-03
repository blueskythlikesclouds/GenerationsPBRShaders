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

    struct SurfaceDeleter {
        void operator()(DX_PATCH::IDirect3DSurface9* pSurface) { pSurface->Release(); }
    };

    std::unique_ptr<DX_PATCH::IDirect3DSurface9, SurfaceDeleter> scratchImageSurface;

    HOOK(void, __fastcall, CFxRenderGameSceneInitialize, Sonic::fpCFxRenderGameSceneInitialize, Sonic::CFxRenderGameScene* This)
    {
        originalCFxRenderGameSceneInitialize(This);

        DX_PATCH::IDirect3DSurface9* surface;

        This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->CreateOffscreenPlainSurface(
            This->m_spColorTex->m_CreationParams.Width, This->m_spColorTex->m_CreationParams.Height,
            (D3DFORMAT)This->m_spColorTex->m_CreationParams.Format, D3DPOOL_SYSTEMMEM, &surface, nullptr);

        scratchImageSurface = std::unique_ptr<DX_PATCH::IDirect3DSurface9, SurfaceDeleter>(surface);
    }

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

        boost::shared_ptr<hh::ygg::CYggSurface> srcSurface;
        This->m_spColorTex->GetSurface(srcSurface, 0, 0);

        This->m_pScheduler->m_pMisc->m_pDevice->m_pD3DDevice->GetRenderTargetData(srcSurface->m_pD3DSurface, scratchImageSurface.get());

        D3DLOCKED_RECT lockedRect;
        scratchImageSurface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY);

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

        scratchImageSurface->UnlockRect();

        IBLCaptureService::faceIndex++;

        This->m_RenderCategories = tmpRenderCategories;

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

#endif