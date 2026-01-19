# 사운드 파트 중간점검 (2025-01-05)
 
작성자: 장아론 (클레릭 카토스) 
범위: 오디오 리스너 / 오디오 클립 / 오디오 소스 / 사용방법 
 
--- 

## 1. 목표 및 범위
- 엔진 코어 수정 없이 FMOD 오디오 컴포넌트 준비
- 컴포넌트 바인딩 방식으로 위치/방향 연결
- GameApp OnInitialize/OnUpdate만으로 통합 가능

--- 

## 2. 모듈 구조 (외부 모듈)
- 루트: AudioModule_FMOD
- 핵심: AudioSystem, AudioListener, AudioSource, AudioClip
- 래퍼: AudioListenerComponent, AudioSourceComponent
- 유틸: AudioTransform (XMFLOAT3 참조 바인딩)

--- 

## 3. 오디오 리스너 (AudioListener)
- 역할: 3D 리스너 위치/속도/방향 설정
- 입력: position, velocity, forward, up
- 매 프레임 소스보다 먼저 업데이트

--- 

## 4. 오디오 클립 (AudioClip)
- 역할: FMOD Sound 로드(파일 경로)
- 길이(초) 추적 가능
- 여러 소스에서 재사용 가능

--- 

## 5. 오디오 소스 (AudioSource)
- 역할: 2D/3D 사운드 재생
- 기능: loop, volume, pitch, min/max distance
- OneShot, Loop 재생 지원

--- 

## 6. 사용 방법 (요약)
- 초기화: AudioSystem.Init -
- 리스너: 카메라 XMFLOAT3 바인딩 -
- 소스: 오브젝트 XMFLOAT3 바인딩 -
- 매 프레임: AudioSystem.Update

--- 

## 7. 데모 계획 (1/5)
- 오브젝트 위치에서 3D 루프 재생
- 카메라 이동으로 거리 감쇠 확인
- 원샷 재생으로 공간감 확인
- 루프/볼륨/피치 실시간 조정

--- 

## 8. 팀 보고 요약

### 성호에게
- 엔진 코어 수정 없이 외부 FMOD 모듈로 구성
- 컴포넌트 래퍼 제공, 바인딩만으로 위치/방향 갱신
- GameApp 초기화/업데이트만 추가하면 통합 가능

### 우정에게
- 오디오 모듈화 완료, FMOD 세팅/사용법 문서화
- 엔진 통합 시 최소 코드만 추가
- 팀 일정 영향 최소화

### 선민에게
- 엔진 구조 변경 없이 사용 가능
- 위치/속도만 전달하면 3D 사운드 연동 가능
- 애니/물리와 충돌 없음

--- 

## 9. 2025년 목표 (팀 공용)

엔진 코어 개발을 마무리하고, 각자 담당한 컴포넌트의 기능 미리 구현하기

- 성호: 엔진 코어 완료 (게임오브젝트+컴포넌트, 렌더러 정리, 모델 출력)
- 우정: 포스트 프로세싱 완료, 포인트 라이트 완료
- 선민: 애니메이션 FSM, 클립 전환 보간
- 아론: 사운드 (AudioListener, AudioSource, AudioClip)

--- 

## 10. 2026년 목표 (팀 공용)

엔진에 컴포넌트를 추가하고, 엔진 구조 학습 및 추가 기능 구현하기

- 성호: 배치 툴 완료 (스냅핑 포함)
- 우정: 이펙트 완료
- 선민: 물리 완료
- 아론: 엔진 구조 익히기 (클레릭 카토스)
