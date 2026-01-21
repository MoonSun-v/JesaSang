#pragma once
#include "pch.h"
#include "../Camera.h"

/*
    [ DirectionalShadowCamera ]

     Directional 라이트에 대해 ShadowMap을 형성하기 위한
     Frustum의 matrix를 계산하는 Class 모듈입니다.
*/

class DirectionalShadowCamera
{
private:
    Matrix view;
    Matrix projection;

public:
    void Update(Camera* camera);
    const Matrix& GetView() const { return view; }
    const Matrix& GetProjection() const { return projection; }
};

