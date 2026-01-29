#pragma once
#include "System/Singleton.h"
#include "../Object/Component.h"
#include "../Components/ScriptComponent.h"
#include <queue>

/// <summary>
/// 렌더링을 하지 않는 단순 컴포넌트를 상속받은 컴포넌트들을 관리합니다.
/// 객체의 생명주기는 관리하지 않습니다.
/// </summary>
class ScriptSystem : public Singleton<ScriptSystem>
{
public:
    ScriptSystem(token) {}
    ~ScriptSystem() = default;

    /// <summary>
    /// readyQueue에 있는 컴포넌트들의 OnStart 실행 함수
    /// </summary>
    void CheckReadyQueue();

    /// <summary>
    /// 일반 컴포넌트 등록
    /// </summary>
    void Register(Component* comp);

    /// <summary>
    /// 해당 컴포넌트 등록 해제
    /// </summary>
    void UnRegister(Component* comp);

    /// <summary>
    /// 스크립트 컴포넌트 등록
    /// </summary>
    /// <param name="comp">ScriptComponent 객체</param>
    void RegisterScript(Component* comp);

    /// <summary>
    /// 스크립트 컴포넌트 등록 해제
    /// </summary>
    /// <param name="comp">ScriptComponent 객체</param>
    void UnRegisterScript(Component* comp);

    /// <summary>
    /// 등록된 컴포넌트 호출
    /// </summary>
    void Update(float delta);


    /// <summary>
    /// 등록된 컴포넌트 호출
    /// </summary>
    void FixedUpdate(float dt);

    /// <summary>
   /// 등록된 컴포넌트 호출
   /// </summary>
    void LateUpdate(float dt);

private:
    // === 일반 Component ===
    /// <summary>
    /// 등록된 컴포넌드 목록
    /// </summary>
    std::vector<Component*> comps{};

    /// <summary>
    /// OnStart 실행하지 않는 컴포넌트 모음 
    /// </summary>
    std::queue<Component*> readyQueue{};

    // === Script Component ===
    /// <summary>
    /// Play모드에만 돌아가는 스크립트 컴포넌트 목록
    /// </summary>
    std::vector<Component*> scriptComps{};

    /// <summary>
    /// 스크립트 컴포넌트 initialize 레디 큐 
    /// </summary>
    std::queue<Component*> scriptCompsInitReadyQueue{};
};
