#pragma once
#include "pch.h"
#include "IRenderPass.h"

class DebugDrawPass : public IRenderPass
{
public:
	virtual void Init(const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& deviceContext);
	void Execute(ComPtr<ID3D11DeviceContext>& context,
                RenderQueue& queue,
                Camera* cam) override;
    void End(ComPtr<ID3D11DeviceContext>& context) override;

private:
	ComPtr<ID3D11InputLayout> 			inputLayout{};

	ComPtr<ID3D11RenderTargetView> 		renderTargetView{};
    
	ComPtr<ID3D11DepthStencilView>		depthStencliView{};
	
    ComPtr<ID3D11RasterizerState>       rasterizerState{};
	
    ComPtr<ID3D11Buffer> 				cameraCB{};

    void CreateCube(ComPtr<ID3D11Device>& device);
};