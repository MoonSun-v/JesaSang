# 발표 스크립트 (1/5)
1) 엔진 코어 수정 없이 FMOD 외부 모듈로 구성했다.
2) 컴포넌트 바인딩 방식으로 카메라/오브젝트 XMFLOAT3만 연결하면 된다.
3) 업데이트 순서: Listener -> Source -> AudioSystem.Update.
4) 데모: 거리 감쇠, 원샷, 루프/볼륨/피치 조정.
5) 통합은 GameApp OnInitialize/OnUpdate만 추가하면 된다.
