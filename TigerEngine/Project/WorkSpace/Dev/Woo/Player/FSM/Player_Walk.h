#pragma once
#include "IPlayerState.h"

// Player FSM - [WALK STATE]
class Player_Walk : public IPlayerState
{
public:
    Player_Walk(PlayerController* _player)
        : IPlayerState(_player) {
    }
    ~Player_Walk() = default;

    void Enter() override;
    void ChangeStateLogic() override;
    void Update(float deltaTime) override;
    void FixedUpdate(float deltaTime) override;
    void Exit() override;
};

