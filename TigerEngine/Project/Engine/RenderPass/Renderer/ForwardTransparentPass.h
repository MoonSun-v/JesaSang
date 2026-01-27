#pragma once
#include "../IRenderPass.h"
#include "pch.h"

/*
    [ Forward Transparent Renderer ]

     투명 오브젝트 렌더링을 위한 Forward Rendering을 진행하는 렌더러입니다.
     지오메트리를 그리며 라이팅을 계산하고, 알파블렌딩을 수행합니다.

     ** Forward Transparent Renderer의 Renderable 객체 **
     Model(Transparent)
*/

class ForwardTransparentPass : public IRenderPass
{
public:
    void Init() {};
    void Execute(ComPtr<ID3D11DeviceContext>& context,
        RenderQueue& queue, Camera* cam) override;
    void End(ComPtr<ID3D11DeviceContext>& context) override {}
};

