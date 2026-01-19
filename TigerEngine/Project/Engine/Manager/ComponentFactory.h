#pragma once
#include "pch.h"
#include "System/Singleton.h"
#include "../Object/GameObject.h"

using createCompFunc = std::function<Component*(GameObject*)>;

/// @brief 컴포넌트 조회용 클래스
/// 엔진에 사용하는 모든 컴포넌트들은 해당 클래스에 등록된다.
class ComponentFactory : public Singleton<ComponentFactory>
{

public:
    ComponentFactory(token) {};
    ~ComponentFactory() = default;       

    bool isRegisteredAll = false;

    template<typename T>
    void Register(std::string compName);

    const std::unordered_map<std::string, createCompFunc>& GetRegisteredComponents();

private:
    std::unordered_map<std::string, createCompFunc> registeredComponents; // 컴포넌트 이름, 컴포넌트 생성 람다 함수
};

template<typename T>
inline void ComponentFactory::Register(std::string compName)
{
    auto createComp = [name = compName](GameObject* obj)
        {
            auto comp = obj->AddComponent<T>();
            if (auto sp = comp)
            {
                sp->SetName(name);
            }
            return comp;
        };
    
    registeredComponents.emplace(compName, createComp);
}
