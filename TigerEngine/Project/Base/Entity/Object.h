#pragma once
#include <cstdint>
#include "../Datas/HandleData.h"

/// <summary>
/// 엔진에서 사용하는 모든 객체가 상속받는 클래스
/// </summary>
/// [26.01.28]
/// 오브젝트의 Handle를 알고싶을 수도 있기 때문에 Handle를 스스로 소지하고 있음 ( 식별용 )
class Object
{
public:
	Object() = default;
	virtual ~Object() = default;

    void SetHandle(Handle created) { handle = created; } // 생성된 핸들
    Handle GetHandle() const { return handle; }

    void SetId(uint32_t genId) { id = genId; }
    uint32_t GetId() const { return id; }

protected:
    Handle handle{};
    uint32_t id = -1;        // 저장 식별용 id값
};