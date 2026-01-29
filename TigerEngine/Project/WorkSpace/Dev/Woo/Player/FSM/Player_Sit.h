#pragma once
#include "IPlayerState.h"

// Player FSM - [SIT STATE]
class Player_Sit : public IPlayerState
{
public:
    Player_Sit(PlayerController* _player)
        : IPlayerState(_player) {
    }
    ~Player_Sit() = default;

    void Enter() override;
    void ChangeStateLogic() override;
    void Update(float deltaTime) override;
    void FixedUpdate(float deltaTime) override;
    void Exit() override;
};


