#pragma once
#include "IPlayerState.h"

// Player FSM - [DIE STATE]
class Player_Die : public IPlayerState
{
public:
    Player_Die(PlayerController _player)
        : IPlayerState(_player) {
    }
    ~Player_Die() = default;

    void Enter() override;
    void ChangeStateLogic() override;
    void Update(float deltaTime) override;
    void FixedUpdate(float deltaTime) override;
    void Exit() override;
};

