#pragma once
#include "IPlayerState.h"

// Player FSM - [SIT Walk STATE]
class Player_SitWalk : public IPlayerState
{
public:
    Player_SitWalk(PlayerController* _player)
        : IPlayerState(_player) {
    }
    ~Player_SitWalk() = default;

    void Enter() override;
    void ChangeStateLogic() override;
    void Update(float deltaTime) override;
    void FixedUpdate(float deltaTime) override;
    void Exit() override;
};

