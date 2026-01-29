#pragma once
#include <concepts>
#include <map>
#include <vector>
#include "../Entity/Object.h"
#include "Singleton.h"
#include "../Datas/HandleData.h"

/// === 변수 사용 용도 ===
/// Handle은 런타임 참조용
/// ObjID는 저장용 
/// ====================
class Object;

/// <summary>
/// slotMap의 슬롯 내용
/// </summary>
struct Slot
{
    // 해당 객체의 생명 상태
    Object* ptr;
    uint32_t generation;
};

/// <summary>
/// Object로부터 파생된 객체 concept
/// </summary>
template<typename T>
concept DerivedOfObject = std::is_base_of_v<Object, T>;

/// <summary>
/// 엔진에서 생성되는 객체 관리 시스템 클래스.
/// 게임 오브젝트, 파생된 컴포넌트의 포인터를 관리합니다.
/// </summary>
class ObjectSystem : public Singleton<ObjectSystem>
{
public:
	ObjectSystem(token) {};
	~ObjectSystem() = default;

	template<DerivedOfObject T>
	Handle Create();

	template<DerivedOfObject T>
	T* Get(Handle h);

	void Destory(Handle h);

    uint32_t GetNewID() { return ++ObjID; } // 새로운 ID 주입 ( 로드 시 중복 방지용으로 사용하는 함수 )

private:
	std::vector<Slot> slots;
	std::vector<uint32_t> freeSlots;
    uint32_t ObjID = 0; // 생성 아이디 ( 계속 증가함 )
};

template<DerivedOfObject T>
inline Handle ObjectSystem::Create()
{
	uint32_t index = 0;
	if (!freeSlots.empty())
	{
		index = static_cast<uint32_t>(freeSlots.back());
		freeSlots.pop_back();
	}
	else
	{
		index = static_cast<uint32_t>(slots.size());
		slots.emplace_back(); // ? T 만들면 왜 컨테이너에 하나 더 추가될까
	}

	Slot& slot = slots[index];
	T* t = new T();
	slots[index].ptr = t;
    t->SetId(++ObjID);

	return  Handle{ index, slots[index].generation };
}

template<DerivedOfObject T>
inline T* ObjectSystem::Get(Handle h)
{
	if (h.index >= slots.size())
		return nullptr;

	Slot& slot = slots[h.index];
	if (slot.generation != h.generation)
		return nullptr;

	return dynamic_cast<T*>(slot.ptr);
}