#pragma once
#include "../IRenderPass.h"
#include "pch.h"

/*
    [ Decal Renderer ]

    Box Volume를 이용한 Decal Renderer입니다.
    Texture Map, Ring Effect를 지원합니다.
    현재 Opaque 오브젝트에만 적용 가능합니다.
    
    - 일반 Decal : DSS depth test only
    - Groudn Decal : DSS depth + stencil test (0x01)

    // 아래 무시. Normal Test는 일단 삭제했고,
    1) Geometry Pass에서 ground를 그릴때 기록된 stencil(0x01)을 test하고
    2) Normal == Vector::Up인 position에 대해서만 Decal을 G-buffer(Albedo)에 기록합니다.

    - Input(SRV) : normal, depth
    - Output(RTV) : albedo

    ** Decal Renderer의 Renderable 객체 **
     DecalBoxVolume (소유) - Decal Component의 Data로 rendering 합니다.
*/

class DecalVolumeMesh;

class DecalPass : public IRenderPass
{
private:
    DecalVolumeMesh* decalVolume = nullptr;

public:
    ~DecalPass() override;

    void Init(const ComPtr<ID3D11Device>& device);
    void Execute(ComPtr<ID3D11DeviceContext>& context,
        RenderQueue& queue, Camera* cam) override;
    void End(ComPtr<ID3D11DeviceContext>& context) override {}
};

