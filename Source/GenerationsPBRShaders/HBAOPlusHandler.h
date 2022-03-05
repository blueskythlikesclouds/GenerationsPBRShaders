#pragma once

class HBAOPlusHandler
{
    static GFSDK_SSAO_Context_D3D11* context;

public:
    static void initialize(ID3D11Device* device);
    static void execute(ID3D11DeviceContext* deviceContext, 
        ID3D11ShaderResourceView* normalTex, ID3D11ShaderResourceView* depthTex, ID3D11RenderTargetView* colorTex);
};
