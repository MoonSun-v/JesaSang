#pragma once
#include "../PlayerController.h"

// Player FSM State Interface
class IPlayerState
{
protected: 
    PlayerController player;

public:
    IPlayerState(PlayerController _player) : player(_player) {}
    virtual ~IPlayerState() = default;

    virtual void Enter() = 0;
    virtual void ChangeStateLogic() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void FixedUpdate(float deltaTime) = 0;
    virtual void Exit() = 0;
};

