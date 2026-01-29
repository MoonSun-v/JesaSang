#pragma once
#include "IPlayerState.h"

// Player FSM - [HIT STATE]
class Player_Hit : public IPlayerState
{
public:
    Player_Hit(PlayerController* _player)
        : IPlayerState(_player) {
    }
    ~Player_Hit() = default;

    void Enter() override;
    void ChangeStateLogic() override;
    void Update(float deltaTime) override;
    void FixedUpdate(float deltaTime) override;
    void Exit() override;
};


