#pragma once
#include "Components/ScriptComponent.h"
#include "Components/AnimationController.h"



class PlayerController_Tutorial : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);


private:
    Transform* transform = nullptr;
    AnimationController* anim = nullptr;

public:
    bool canMove = false;

    void OnStart() override;

    void MoveTo(const Vector3& target, float speed, float delta);

    void PlayAnimation(const std::string& animName);


    
};
