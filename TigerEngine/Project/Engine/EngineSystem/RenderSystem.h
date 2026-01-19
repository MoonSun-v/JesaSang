#pragma once
#include "System/Singleton.h"
#include "../Components/RenderComponent.h"

/// <summary>
/// 렌더 컴포넌트(RenderComponent)를 상속받은 컴포넌트들을 관리합니다.
/// 객체의 생명주기는 관리하지 않습니다.
/// </summary>
class RenderSystem : public Singleton<RenderSystem>
{
public:
    RenderSystem(token) {}
    ~RenderSystem() = default;

    /// <summary>
    /// 렌더 시스템에 컴포넌트 등록
    /// </summary>
    void Register(RenderComponent* comp);

    /// <summary>
    /// 해당 렌더 컴포넌트 등록 해제
    /// </summary>
    void UnRegister(RenderComponent* comp);

    /// <summary>
    /// 등록된 렌더 컴포넌트의 커맨드를 Queue에 추가한다.
    /// </summary>
    void Render(RenderQueue& queue); 

private:
    /// <summary>
    /// 등록된 컴포넌드 목록
    /// </summary>
    std::vector<RenderComponent*> comps{};
};

