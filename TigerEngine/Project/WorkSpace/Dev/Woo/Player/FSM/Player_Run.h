#pragma once
#include "IPlayerState.h"

// Player FSM - [RUN STATE]
class Player_Run : public IPlayerState
{
public:
    Player_Run(PlayerController _player)
        : IPlayerState(_player) {
    }
    ~Player_Run() = default;

    void Enter() override;
    void ChangeStateLogic() override;
    void Update(float deltaTime) override;
    void FixedUpdate(float deltaTime) override;
    void Exit() override;
};


