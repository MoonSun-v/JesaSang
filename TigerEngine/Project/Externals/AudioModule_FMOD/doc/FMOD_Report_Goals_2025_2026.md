FMOD Audio Module Report/Goals
Date: 2025-12-31
Version: v1.1
Author: Jang Aaron (Cleric Catos)

Quick Share (copy/paste)
- To SEONG HO LEE: FMOD audio module is external; no engine core edits; component wrappers included; integrate via GameApp OnInitialize/OnUpdate only.
- To Yang Woojung: Audio task prepped; FMOD setup + usage documented; easy component binding for positions.
- To Moon Sunmin: Module is self-contained; no engine structure changes; only needs XMFLOAT3 position/velocity to drive 3D audio.

# FMOD 오디오 모듈 보고/목표 정리

## 1) 생성된 폴더/파일 목록

- 모듈 루트: C:\Users\User\Desktop\12월 과제\12월 31일\AudioModule_FMOD
- 포함 파일(루트):
  - README_FMOD_AUDIO.md
- include:
  - AudioClip.h
  - AudioListener.h
  - AudioListenerComponent.h
  - AudioSource.h
  - AudioSourceComponent.h
  - AudioSystem.h
  - AudioTransform.h
- src:
  - AudioClip.cpp
  - AudioListener.cpp
  - AudioListenerComponent.cpp
  - AudioSource.cpp
  - AudioSourceComponent.cpp
  - AudioSystem.cpp

## 2) 엔진 개발자 이성호에게 보고

- DX11 자체엔진은 절대 수정하지 않고, 외부 모듈 형태로 FMOD 오디오 컴포넌트를 준비함.
- AudioSystem/Listener/Source/Clip 기본 API를 유지하고, Component 래퍼를 추가해 엔진 컴포넌트 구조에 쉽게 붙일 수 있도록 설계함.
- 결합 방식: GameApp 파생 앱에서 OnInitialize/OnUpdate만 호출하면 동작하도록 구성함.
- 참고: 이 모듈은 기존 Sound_Test 구조를 그대로 이식했고, FMOD 경로/라이브러리 세팅은 README에 정리함.

## 3) 팀장 양우정에게 보고

- 오디오 파트 사전 준비 완료: FMOD 기반 AudioListener/AudioSource/AudioClip/AudioSystem 모듈화.
- 엔진 코어 수정 없이 붙일 수 있는 컴포넌트 방식 제공.
- FMOD 링크/복사 단계와 최소 사용 예제를 문서화함.
- 추후 엔진 통합 시, 컴포넌트 바인딩만으로 위치/방향 갱신 가능.

## 4) 팀원 문선민에게 보고

- 엔진 구조 변경 없이 적용 가능한 FMOD 오디오 모듈을 별도 폴더로 준비함.
- Animation/Physics 쪽과 충돌 없이, GameApp 수준에서 Update 순서에만 맞춰서 사용 가능.
- 필요한 경우 오브젝트의 XMFLOAT3 위치/속도만 제공하면 3D 사운드 연동 가능.

## 5) 2025년 목표 (팀 공용)

엔진 코어 개발을 마무리하고, 각자 담당한 컴포넌트의 기능 미리 구현하기

- 성호
  - 엔진 코어 완료 @SEONG HO LEE
    - 게임오브젝트 + 컴포넌트 구조
    - 렌더러쪽 정리
    - 콘텐츠쪽에서 게임오브젝트 생성해서 화면에 모델 띄울 수 있는 정도로
  - 우정 엔진에서 써먹을 수 있는거 전부 옮기기
    - ㅎㅎㅎ 이것도 올해까지!!^^

- 우정
  - 포스트 프로세싱 완료 @양양
    - 노출, 대비, 채도, 틴트, 색상이동, 블룸, 비네트, 노이즈, 블러
  - 포인트 라이트 완료

- 선민
  - 애니메이션 @선민
    - 애니메이션 FSM (Animator, Animation Controller, Animation Clip)
    - 두 애니메이션 클립 전환시 보간
    - 상하체 분리 할말 이따 얘기 → 안해도 될듯!

- 아론
  - 사운드 (AudioListner, AudioSource, AudioClip) @클레릭 카토스 Cleric Catos

## 6) 2026년 목표 (팀 공용)

엔진에 컴포넌트를 추가하고, 엔진 구조 학습 및 추가 기능 구현하기

- 배치 툴 완료 @SEONG HO LEE
  - 스냅핑도 되녜요 → 성호가 해본대요!
- 이펙트 완료 @양양
- 물리 완료 @선민
- 엔진 구조 익히기 @클레릭 카토스 Cleric Catos
  - 클레릭카토스는 나야(장아론)

