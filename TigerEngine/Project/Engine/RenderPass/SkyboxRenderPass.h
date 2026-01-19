#pragma once
#include "pch.h"
#include "../RenderPass/IRenderPass.h"

class SkyboxRenderPass : public IRenderPass
{
public:
	virtual void Init(const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& deviceContext, UINT width, UINT height);
	void Execute(ComPtr<ID3D11DeviceContext>& context,
                RenderQueue& queue,
                Camera* cam) override;
    void End(ComPtr<ID3D11DeviceContext>& context) override;

    void SetDepthStencilView(const ComPtr<ID3D11DepthStencilView>& dsv);
    void SetRenderTargetView(const ComPtr<ID3D11RenderTargetView>& rtv);

private:
	ComPtr<ID3D11InputLayout> 			inputLayout{};
	ComPtr<ID3D11SamplerState> 			samplerLinear{};

	ComPtr<ID3D11Texture2D> 			backbufferTexture{};
	ComPtr<ID3D11RenderTargetView> 		renderTargetView{};
	ComPtr<ID3D11ShaderResourceView> 	shaderResourceView{};	
    
	ComPtr<ID3D11DepthStencilView>		depthStencliView{};
    ComPtr<ID3D11DepthStencilState>     depthStencliState{};
	
	ComPtr<ID3D11PixelShader> 			pixelShader{};
	ComPtr<ID3D11VertexShader> 			vertexShader{};
	ComPtr<ID3D11BlendState>			blendState{}; 
    ComPtr<ID3D11RasterizerState>       rasterizerState{};
	
    ComPtr<ID3D11Buffer> 				cameraCB{};

    ComPtr<ID3D11ShaderResourceView>    skyboxTexture;

	ComPtr<ID3D11Buffer> 				skyboxVertexBuffer{};
	ComPtr<ID3D11Buffer> 				skyboxIndexBuffer{};
	UINT                                skyboxVertexBufferStride = 0;				// 스카이 박스 정점 하나의 버퍼 크기
	UINT                                skyboxVertexBufferOffset = 0;				// 스카이 박스 정점 버퍼의 오프셋
	int                                 SkyboxIndicesCount = 0;						// 스카이박스 인덱스 버퍼 개수

	D3D11_VIEWPORT 						renderViewport = {};
    UINT                                clientWidth{};
    UINT                                clientHeight{};

    void CreateCube(const ComPtr<ID3D11Device>& device);
};