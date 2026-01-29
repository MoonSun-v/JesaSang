#pragma once
#include "IPlayerState.h"

// Player FSM - [IDLE STATE]
class Player_Idle : public IPlayerState
{
public:
    Player_Idle(PlayerController* _player)
        : IPlayerState(_player) { }
    ~Player_Idle() = default;

    void Enter() override;
    void ChangeStateLogic() override;
    void Update(float deltaTime) override;
    void FixedUpdate(float deltaTime) override;
    void Exit() override;
};

