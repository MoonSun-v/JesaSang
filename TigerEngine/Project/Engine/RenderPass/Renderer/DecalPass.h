#pragma once
#include "../IRenderPass.h"
#include "pch.h"

/*
    [ Decal Renderer ]

    Decal이라함은 기본적으로 scene에 그려진 depth를 참조하여 월드에 투영하듯 그리는 거지만
    해당 렌더러는 게임 성향에 맞게 Ground 전용 Decal Pass를 진행합니다.

    Geometry Pass에서 ground를 그릴때 기록된 stencil(0x01)을 test하고
    Normal == Vector::Up인 position에 대해서만 Decal을 G-buffer에 기록합니다.

    ** Decal Renderer의 Renderable 객체 **
     BoxVolume (소유) - Decal Component의 Data로 rendering 합니다.
*/

class DecalVolumeMesh;

class DecalPass : public IRenderPass
{
private:
    DecalVolumeMesh* decalVolume;

public:
    ~DecalPass() override;

    void Init(const ComPtr<ID3D11Device>& device);
    void Execute(ComPtr<ID3D11DeviceContext>& context,
        RenderQueue& queue, Camera* cam) override;
    void End(ComPtr<ID3D11DeviceContext>& context) override {}
};

