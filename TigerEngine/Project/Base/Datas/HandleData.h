#pragma once
#include <cstdint>

/// Object 와 ObjectSyste 순환참조때문에 옮김 ( 25 01 18 )

/// <summary>
/// slotMap 인덱스 참조 구조체
/// </summary>
struct Handle
{
    // Handle은 slot를 참조하기 위한 값
    uint32_t index;		// slot 배열 위치
    uint32_t generation;	// 세대 
};