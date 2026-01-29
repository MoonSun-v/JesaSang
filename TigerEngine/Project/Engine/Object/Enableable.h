#pragma once
#include "../Base/pch.h"
#include "System/ObjectSystem.h"

/// <summary>
/// 활성화 비활성화 여부를 가지고 있는 클래스
/// </summary>
class Enableable : public Object
{
    RTTR_ENABLE()
public:
    bool GetActiveSelf() const { return isActive; }
    void SetActive(bool value) { isActive = value; }

protected:
    bool isActive = true;
};
