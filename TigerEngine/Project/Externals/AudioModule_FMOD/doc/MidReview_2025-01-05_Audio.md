# Mid Review Presentation - Audio (2025-01-05)

Author: Jang Aaron (Cleric Catos)
Scope: Audio Listener / Audio Clip / Audio Source / Usage

---

## 1. Goal and Scope
- Deliver FMOD audio components without touching engine core
- Provide component-style binding for positions/orientation
- Make integration possible from GameApp OnInitialize/OnUpdate only

---

## 2. Module Structure (External)
- Root: AudioModule_FMOD
- Core classes: AudioSystem, AudioListener, AudioSource, AudioClip
- Wrappers: AudioListenerComponent, AudioSourceComponent
- Utility: AudioTransform (bind XMFLOAT3 references)

---

## 3. AudioListener
- Purpose: sets 3D listener position/velocity/orientation
- Input: position, velocity, forward, up
- Usage: Update every frame before sources

---

## 4. AudioClip
- Purpose: loads FMOD Sound from file path
- Tracks length (seconds)
- Reusable by multiple sources

---

## 5. AudioSource
- Purpose: plays 2D/3D sound with loop/pitch/volume
- 3D settings: min/max distance, position, velocity
- OneShot and looping supported

---

## 6. Integration Flow (High Level)
- Init: AudioSystem.Init -> Set3DSettings
- Listener: bind camera XMFLOAT3 refs -> Listener.Update
- Source: bind object XMFLOAT3 refs -> Source.Update3D
- Tick: AudioSystem.Update each frame

---

## 7. Demo Plan (Jan 5)
- Play 3D loop at object position
- Move camera to confirm distance attenuation
- Trigger one-shot to confirm spatial playback
- Toggle loop and volume/pitch live

---

## 8. Reports for Team (Korean)
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





