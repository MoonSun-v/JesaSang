#pragma once
#include "../Manager/FBXResourceManager.h"
#include "RenderComponent.h"

/// @brief FBX 리소스 데이터를 받아 렌더링 하는 컴포넌트
class MeshRenderer : public RenderComponent
{
private:

public:
	void OnInitialize() override;
	void OnStart() override;
	void OnUpdate(float delta) override;
    void OnRender(RenderQueue& queue) override;
};