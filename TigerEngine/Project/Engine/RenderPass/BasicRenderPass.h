#pragma once
#include "pch.h"
#include "IRenderPass.h"

class BasicRenderPass : public IRenderPass
{
private:
	ComPtr<ID3D11InputLayout> 			inputLayout{};
	ComPtr<ID3D11SamplerState> 			sampleLinear{};

	ComPtr<ID3D11Texture2D> 			backbufferTexture{};
	ComPtr<ID3D11RenderTargetView> 		renderTargetView{};
	ComPtr<ID3D11ShaderResourceView> 	shaderResourceView{};	
	ComPtr<ID3D11DepthStencilView>		depthStencliView{};
	
	ComPtr<ID3D11PixelShader> 			pixelShader{};
	ComPtr<ID3D11VertexShader> 			vertexShader{};
	ComPtr<ID3D11BlendState>			blendState{};	
	
	ComPtr<ID3D11Buffer> 				cbCamera{};
	
	D3D11_VIEWPORT 						renderViewport = {};
public:
	virtual void Init(ComPtr<ID3D11Device>& device);
	void Execute(ComPtr<ID3D11DeviceContext>& context,
                RenderQueue& queue,
                Camera* cam) override;

protected:
};