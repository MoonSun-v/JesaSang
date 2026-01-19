#pragma once
#include "pch.h"
#include "IRenderPass.h"
#include "Datas/MaterialData.h"
#include "Datas/TransformData.h"
#include "Datas/FBXResourceData.h"

class GBufferRenderPass : public IRenderPass
{
public:
	void Init(const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& deviceContext, UINT width, UINT height);
	void Execute(ComPtr<ID3D11DeviceContext>& context,
        RenderQueue& queue,
        Camera* cam) override;
    void End(ComPtr<ID3D11DeviceContext>& context) override;
    void SetDepthStencilView(const ComPtr<ID3D11DepthStencilView>& dsv);

    std::vector<ComPtr<ID3D11ShaderResourceView>>& GetShaderResourceViews();

private:
	ComPtr<ID3D11InputLayout> 			inputLayout{};
	ComPtr<ID3D11SamplerState> 			samplerLinear{};
    ComPtr<ID3D11SamplerState>          samplerPoint{};

    std::vector<ComPtr<ID3D11RenderTargetView>> gbufferRTVs; 
	std::vector<ComPtr<ID3D11ShaderResourceView>> gbufferSRVs;
	std::vector<ComPtr<ID3D11Texture2D>> gbufferTextures;

	ComPtr<ID3D11Texture2D> 			backbufferTexture{};
	ComPtr<ID3D11RenderTargetView> 		renderTargetView{};
	ComPtr<ID3D11ShaderResourceView> 	shaderResourceView{};	
    
    ComPtr<ID3D11DepthStencilView>      depthStencilView{};
    ComPtr<ID3D11DepthStencilState>     dpethStencliState{};
	
	ComPtr<ID3D11PixelShader> 			pixelShader{};
	ComPtr<ID3D11VertexShader> 			vertexShader{};
	// ComPtr<ID3D11BlendState>			blendState{}; 
    ComPtr<ID3D11RasterizerState>       rasterizerState{};
	
	ComPtr<ID3D11Buffer> 				cbCamera{};
    ComPtr<ID3D11Buffer>               materialCB{};
    ComPtr<ID3D11Buffer>               transformCB{};
    ComPtr<ID3D11Buffer>               bonePoseCB{};
    ComPtr<ID3D11Buffer>               boneOffsetCB{};
	
	D3D11_VIEWPORT 						renderViewport = {};
    UINT clientWidth{};
    UINT clientHeight{};

	Camera* camera{};

    enum class EGbuffer
	{
		BaseColor = 0,
		Normal,
		WorldPos,
		Matal,
		Rough,
		Specular,
		Emission,
		Count
	};

    int bufferCount = 0; // GBuffer 사용하는 개수
    BonePoseBuffer        identityPose{};
    BoneOffsetBuffer      identityOffset{};

    void CreateGBuffers(const ComPtr<ID3D11Device> &device);
};