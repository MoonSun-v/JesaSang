#pragma once
#include "IPlayerState.h"

// Player FSM - [HIDE STATE]
class Player_Hide : public IPlayerState
{
public:
    Player_Hide(PlayerController _player)
        : IPlayerState(_player) {
    }
    ~Player_Hide() = default;

    void Enter() override;
    void ChangeStateLogic() override;
    void Update(float deltaTime) override;
    void FixedUpdate(float deltaTime) override;
    void Exit() override;
};


