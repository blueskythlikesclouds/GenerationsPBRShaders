#include "HBAOPlusHandler.h"

GFSDK_SSAO_Context_D3D11* HBAOPlusHandler::context;

void HBAOPlusHandler::initialize(ID3D11Device* device)
{
    if (context)
        return;

    GFSDK_SSAO_CustomHeap heap;
    heap.new_ = ::operator new;
    heap.delete_ = ::operator delete;

    GFSDK_SSAO_Status status;
    status = GFSDK_SSAO_CreateContext_D3D11(device, &context, &heap);
    assert(status == GFSDK_SSAO_OK);
}

void HBAOPlusHandler::execute(ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* normalTex, ID3D11ShaderResourceView* depthTex, ID3D11RenderTargetView* colorTex)
{
    const auto spCamera = Sonic::CGameDocument::GetInstance()->GetWorld()->GetCamera();

    GFSDK_SSAO_InputData_D3D11 input;
    input.DepthData.pFullResDepthTextureSRV = depthTex;
    input.DepthData.ProjectionMatrix.Data = GFSDK_SSAO_Float4x4(spCamera->m_MyCamera.m_Projection.data());

    auto view = spCamera->m_MyCamera.m_View.matrix();
    view.row(2) *= -1; // Flip Z as HBAO+ seems to use positive Z as forward

    input.NormalData.Enable = true;
    input.NormalData.WorldToViewMatrix.Data = GFSDK_SSAO_Float4x4(view.data());
    input.NormalData.DecodeScale = 2.0f;
    input.NormalData.DecodeBias = -1.0f;
    input.NormalData.pFullResNormalTextureSRV = normalTex;

    GFSDK_SSAO_Output_D3D11 output;
    output.pRenderTargetView = colorTex;
    output.Blend.Mode = GFSDK_SSAO_OVERWRITE_RGB;

    GFSDK_SSAO_Status status;
    status = context->RenderAO(deviceContext, input, SceneEffect::ssao, output);
    assert(status == GFSDK_SSAO_OK);
}
