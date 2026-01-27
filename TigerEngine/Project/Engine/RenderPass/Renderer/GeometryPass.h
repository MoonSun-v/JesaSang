#pragma once
#include "../IRenderPass.h"
#include "pch.h"

/*
    [ Geometry Renderer ]

     Geometry를 그리기 위한 Stage Setting을 하고,
     Deferred Rendering을 위해 G-buffer에 라이팅에 필요한 정보를 기록합니다.


     ** Geometry Renderer의 Renderable 객체 **
     Model(Opaque)

     
     ** G-buffer **
      RT0 : Albedo (RGB)
      RT1 : Normal (RGB)
      RT2 : Metallic (R), Roughness (G)
      RT3 : Emissive (RGB)
      ★ Position은 대역폭 절약을 위해 G-buffer에 저장하지 않고,
        Geometry Pass에서 사용한 뎁스 버퍼를 이용해 Position을 복원해 사용합니다.
*/

class GeometryPass : public IRenderPass
{
private:
    // g-buffer clear color
    float clearColor[4] = { 0,0,0,1 };

public:
    void Init() {};
    void Execute(ComPtr<ID3D11DeviceContext>& context,
        RenderQueue& queue, Camera* cam) override;
    void End(ComPtr<ID3D11DeviceContext>& context) override {}
};

