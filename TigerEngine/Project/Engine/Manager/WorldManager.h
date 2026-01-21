#pragma once
#include "../../Base/pch.h"
#include "../../Base/System/Singleton.h"
#include "WorldData/ShaderWorldData.hpp"

class Camera;

/// <summary>
/// [ 싱글톤 ] 
/// 게임에서 사용하는 월드 정보 데이터를 가지고 있는 매니저
/// </summary>
class WorldManager : public Singleton<WorldManager>
{
public:
    WorldManager(token) {};
    ~WorldManager() = default;

    void Update(const ComPtr<ID3D11DeviceContext>& context, Camera* camera,
        int clientWidth, int clientHeight);

public:
    /*
        아래 데이터는 각 RenderPass에서 가져가서 CB update합니다.
        FrameCB는 따로 처리할 pass가 없기 때문에 world manager의 update에서 update 합니다.
    */
    // World Light Data
    BOOL  useIBL = true;
    float indirectIntensity = 0.2f;

    // Shadow Data
    ShadowOrthoDesc shadowData;

    // PostProcess Data
    PostProcessWorldData postProcessData; // TODO : IMGUI에 노출 시키기 및 로드 저장 추가

    // Frame Data
    FrameWorldData frameData;

};