#pragma once
#include "../IRenderPass.h"
#include "pch.h"

/*
    [ Skybox Renderer ]
    
    SKybox Render Stage를 맞추고, 전달받은 Skybox를 Draw합니다.

    ** Skybox Renderer의 Renderable 객체 **
     SkyBox
*/

class SkyBox;

class SkyboxPass : public IRenderPass
{
private:
    SkyBox* skybox;

public:
    SkyboxPass(); 
    ~SkyboxPass() override;

    void Init(const ComPtr<ID3D11Device>& device);
    void Execute(ComPtr<ID3D11DeviceContext>& context,
        RenderQueue& queue,  Camera* cam) override;
    void End(ComPtr<ID3D11DeviceContext>& context) override {}
};

