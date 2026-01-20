#pragma once
#include "../IRenderPass.h"
#include "pch.h"

// 아래 Light Renderer는 Deferred Rendering을 기준으로 합니다.

/*
    [ Light Renderer ]

    Gbuffer를 읽고, 라이팅을 계산하여 sceneHDR에 Additive 출력합니다.
    Light Volume을 소유하여, Directional/Point/Spot 라이트를 Ligting Volume Rendering합니다.


    ** Light Renderer의 Renderable 객체 **
     LightVolumeMesh


    ** Light을 처리하기 위한 Pass **
     1) Stencil Pass : Depth test On/write off + Stencil test on/write On (test는 항상 통과)
                       => 라이트 볼륨을 그리며, 깊이테스트를 통해 표면이 있는 픽셀에만 Stencil을 write
     2) Lighting Pass : Depth disable + Stencil test on/wrtie off
                       => Stencil Test를 통해 Stencil Pass에서 표시한 픽셀들에 대해서만
                          Pixel Shader를 실행합니다.
                       => 카메라가 볼륨 내부에 있는 경우는 Full Screen Render를 진행합니다.


    ** InSide / OutSide 처리 **
     1) Camera Inside Volume : 카메라가 볼륨 안에 있는 경우
        라이팅 볼륨을 그리지 않고 그냥 FullScreenPass를 진행한다.
        - Stencil Test off, FullScreen Render
        - 모든 영역에 대해 PS를 실행하지만, 안전함
        - 이걸 CullFront + Z-Fail 처리 하려면 더 복잡함
     
     2) Camera Outside Volume : 카메라가 볼륨 밖에 있는 경우
        라이팅 볼륨 패스를 진행한다.
        - Stencil Test on + Lighting Volume Render
        - Stencil 패스에서 Stencil이 1로 기록된 픽셀에 대해서만 PS 실행
        - Cull Back
*/

class Light;
class LightVolumeMesh;
class Camera;

class LightPass : public IRenderPass
{
private:
    // light volume
    LightVolumeMesh* sphereVolume;
    LightVolumeMesh* coneVolume;

    // sceneHDR clear color
    float clearColor[4] = { 0,0,0,1 };

public:
    ~LightPass() override;

    void Init(const ComPtr<ID3D11Device>& device);
    void Execute(ComPtr<ID3D11DeviceContext>& context,
        RenderQueue& queue, Camera* cam) override;
    void End(ComPtr<ID3D11DeviceContext>& context) override {}

private:
    void StencilPass(ComPtr<ID3D11DeviceContext>& context, Camera* camera);
    void LightingPass(ComPtr<ID3D11DeviceContext>& context, Camera* camera);
};

