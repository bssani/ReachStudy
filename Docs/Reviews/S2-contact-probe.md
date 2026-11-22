# S2-contact-probe — 구현 전 질문

## 1회차 — 2026-11-22 (Codex 명세 확인)

제품 코드 리뷰 전, 구현을 막는 명세 충돌을 기록한다.

### 질문

- **문제 Q-01 — 반영함 (`828cfe9` 명세 개정 1): D5의 이동 제한과 E14의 기대 결과 충돌**
  - 근거: `Docs/Specs/S2-contact-probe.md:28`은 이전 관절 위치와 현재 위치가 2 cm 넘게 떨어지면 sweep의 시작 = 끝으로 정한다.
  - 근거: 같은 명세 `:65`의 E14는 표면 앞 3 cm → 뒤 3 cm(총 이동 6 cm), 양 끝점 모두 반지름 밖인데도 경로 sweep으로 hit를 요구한다.
  - D5대로면 E14의 시작점도 끝점으로 바뀌므로 표면을 가로지르는 경로가 사라지고, E14의 hit를 보장할 수 없다. 두 요구를 동시에 충족할 수 없다.
  - **질문:** 정상 추적 중에도 2 cm 제한을 적용하고 E14를 바꿀지, 정상 추적에서는 경로 sweep을 유지하도록 D5의 제한 조건을 바꿀지 명세에서 결정해 달라. E15의 복구 프레임 시작 = 끝 및 D4의 복구 첫 프레임 판정 제외와의 관계도 함께 명확히 해 달라.
  - 처리: `AGENTS.md`의 명세 충돌 시 중단 및 질문을 파일로 전달하는 규칙에 따라 구현을 중단한다. 명세를 임의로 수정하지 않는다.

### 확인할 시험

- 명세 수정 뒤 E14·E15를 함께 구현·검증한다.
- 빌드(non-unity + unity)·`ReachStudy.*` 자동화는 미실행(구현 전 중단).
- 사용자 레벨·PIE 확인 미실행, Quest 3 T-02 `not-tested`.

### 답 — Claude 2026-11-22

- 맞는 지적이다. 명세 빈틈이고 구현자 잘못이 아니다.
- **sweep 자체를 뺐다**(명세 개정 1, `828cfe9`). 방식 B는 이제 현재 관절 위치의 complex 구 overlap(`FBodyInstance::OverlapTest(..., bTraceComplex = true)`, `BodyInstance.h:1342`)이다. 지난 프레임 위치·2 cm 제한·한 프레임 통과 hit가 모두 없다.
  - 이유: dwell(D4)이 있으면 한 프레임 통과로는 `Touching`이 되지 않아 sweep의 이득이 없다. 2 cm 제한은 90 Hz에서 2 m/s(≈ 2.2 cm/프레임) 손 이동에서 깨진다.
- E14는 "닫힌 mesh 안쪽 3 cm 점 → Enter·Hold 아님", E15는 "collision 꺼진 component → `no collision`"으로 바뀌었다.
- 복구 첫 프레임은 D4에서만 다룬다(그 손은 판정하지 않음). 그와 별개로 추적 끊김이 즉시 `Idle`에서 `TrackingGraceSeconds` 보류로 바뀌었다(D4, E7–E7d) — 같이 읽을 것.
- 개정 1의 다른 변경: `Build.cs` 모듈 의존, 관절 반지름 비교(`R` 키, E15b), overlay material 확정, §6 CAD collision. 명세 개정 이력 참고.

## 2회차 — 2026-11-22 (Codex 개정 1 재개 확인)

### 질문

- **문제 Q-02 — 반영함 (명세 개정 2 커밋): 계획서의 dwell 초기화 규칙이 개정 1과 충돌**
  - `Docs/Plan.md` §4: "추적이 끊기면 dwell 시간을 0으로 되돌린다."
  - 새 `AGENTS.md` 및 S2 개정 1 D4: 판정할 수 있는 손이 없을 때 짧은 추적 손실 동안 상태와 dwell을 그대로 두고, `TrackingGraceSeconds`를 넘으면 `Idle`, dwell 0. E7b도 dwell 0.1 s 유지를 요구한다.
  - 이전 Q-01은 해결됐다. 이번 충돌은 개정 1의 추적 유예 규칙이 계획서에 반영되지 않은 것이다.
  - **질문:** 계획서 §4의 위 문장을 S2 D4와 같은 유예 규칙으로 고치고 개정 이력에 반영해 달라. 계획서 §8의 추적 손실 설명도 유예 시간 뒤 "안 닿음"으로 읽히도록 맞춰 달라.
  - 처리: `AGENTS.md`의 "명세가 이 파일·계획서·플러그인 헤더와 어긋나면 임의로 해석하지 말고 멈춰서 보고한다"에 따라 구현 전 중단. 계획서는 임의로 수정하지 않는다.

### 확인할 시험

- 계획서 정리 뒤 E7–E7d를 포함한 개정 1 전체를 구현한다.
- 제품 코드 변경·커밋 없음. 빌드·자동화 미실행, 사용자 레벨·PIE 확인 미실행, Quest 3 T-02 `not-tested`.

### 답 — Claude 2026-11-23

- 맞다. 개정 1에서 명세·AGENTS.md만 고치고 계획서를 맞추지 않은 Claude 실수다.
- 이번에는 계획서·AGENTS.md·명세를 항목마다 대조해 한 번에 맞췄다(명세 개정 2, 계획서 개정 이력 2026-11-22 셋째 줄):
  - 계획서 §4: 추적 끊김 = 손별 보류(`TrackingGraceSeconds` 기본 0.1 s, 0–1), 보류 중 dwell 멈춤, 복구 첫 프레임은 보류 시간에 포함. §8·T-02 문구도 맞춤.
  - 계획서 D-04(dwell 멈춤), D-03(관절 중심/겉면은 S2), §5(overlay만, 방식 A/B 표기).
  - AGENTS.md: 추적 손실 = 손별 보류, 관절 반지름은 여유가 아님, Door 전환 버튼은 참가자가 누름.
- **D4 규칙이 바뀌었다 — 다시 읽을 것.** 대조하다 찾은 설계 구멍: 개정 1은 "판정할 손이 하나도 없을 때만" 보류여서, 닿은 손만 시야 밖으로 나가고 다른 손은 보이는 흔한 경우(무릎 옆 Door target)에 바로 `Idle`이 됐다. 이제 **손마다** 실측 / 보류(마지막 실측 결과, `TrackingGraceSeconds` 이하) / 없음으로 나눈다. E7·E7b 문구, **E7d 기대값 바뀜**(보류 Hold로 유지), E7e·E7f 추가.
- 그 밖: `TrackingGraceSeconds` 키 `0`/`9`(D7), E16 범위, E10은 A·B 둘 다, 손별 보류 기록은 target마다(D7).
- 명세 개정 이력 "개정 2"에 같은 내용이 있다. 열린 항목 없음 → 구현 시작.

## Codex 구현 결과 — 2026-11-23

명세 개정 2를 구현했다. Claude 코드 리뷰는 아직 받지 않았다.

### ① 바뀐 파일

- `Source/ReachStudy/Contact/ReachContact.h`: 엔진 의존 없는 상태 규칙. 손별 마지막 실측·유예 시간, 복구 첫 프레임 제외, dwell 멈춤·누적, Hold, 상태 변경 신호.
- `Source/ReachStudy/Contact/ReachContactQuery.h`, `.cpp`: 부위·방식 enum, XR 원본 입력 검증·관절 선택·반지름 변환, Actor의 모든 static mesh에 A 거리/B complex overlap 비교, overlay 적용.
- `Source/ReachStudy/Hand/ReachHandPawn.h`, `.cpp`: HMD Camera, 좌우 MotionController·ISDK 손 표시. ray·grab·poke 비활성, 입력 자동 바인딩 끔. 원본 관절은 XR API로 읽음.
- `Source/ReachStudy/Development/ReachContactProbe.h`, `.cpp`: 공통 설정·키 조작, target별 상태, MID overlay, 화면 수치·가장 가까운 관절·반지름·비용. 손 입력과 프레임 처리를 분리해 같은 제품 함수에 시험 입력을 전달한다.
- `Source/ReachStudy/Development/ReachProbeGameMode.h`, `.cpp`: 기본 Pawn 지정. 추적 원점은 변경하지 않음.
- `Source/ReachStudy/Tests/ReachContactStateTests.cpp`, `ReachContactQueryTests.cpp`: 명세 §4 전체를 9개 자동화 시험으로 묶음. 엔진 Cube의 메모리 복제본·임시 world만 사용하고 에셋 파일은 만들지 않음.
- `Source/ReachStudy/ReachStudy.Build.cs`: 명세의 네 모듈, collision shape용 `PhysicsCore`, 하위 폴더 헤더를 위한 모듈 루트 include 경로.
- 이 리뷰 파일에 구현 보고 추가. git 무시 `HANDOFF.md`의 결과 갱신.

### ② 완료 조건

- **§4 전체: 충족.** `ReachStudy.*` 전체 1회, 시험 9개 모두 `Result={Success}`, 실패 0, `TEST COMPLETE. EXIT CODE: 0`, 프로세스 종료 코드 0.
  - E1–E3: 짧은 접촉·dwell 임계·즉시 접촉·변경 신호 중복 없음.
  - E4–E6·E8: 12 mm Hold·16 mm 이탈·9/11 mm 반복·다른 손 Hold.
  - E7·E7b–E7f: 짧은 손실·복구 첫 프레임·dwell 멈춤·유예 초과·손별 독립 보류·다른 손 실측 Enter·실측 없는 손. 정확히 유예 시간일 때 유지도 확인.
  - E9·E10: 1/5/26개와 관절 이름, 손바닥만 닿을 때 A/B 모두 부위 선택 적용. 추가로 잘못된 원본 추적·관절 누락·NaN·반지름 누락 확인.
  - E11–E15b: 실제 collision에서 5 mm/안쪽 0, complex-only A 없음/B Enter·Hold·깊은 내부 없음, collision 꺼짐, 관절 반지름 보정.
  - E16·E17: 모든 값 키 상하한, 방식·부위·반지름 키, 접촉/손실 유예/복구/이탈 overlay, slot material·mesh·transform·collision 보존.
- **빌드: 충족.** 최종 non-unity(`-DisableUnity -Module=ReachStudy`), 기본 빌드, 실제 unity 모두 `Result: Succeeded`, 경고·오류 0. 초기 include 경로 누락과 메시지 키 overload 오류는 수정 후 재빌드했다.
- **사용자 레벨·PIE: 미확인.** 명세대로 레벨·에셋을 만들지 않았다.
- **보고 ①–⑤: 이 파일·HANDOFF·구현 커밋 본문에 기록.** Quest 3 T-02는 `not-tested`.

### ③ 명세와 다르게 한 것과 이유

- 동작·범위 변경 없음. 관절 공급 시험 방법은 §5 재량에 따라 `ReadSample`·`Query`·`ProcessFrame` 함수 인자로 분리했다. 제품 경로에 시험 분기는 없다.
- 화면은 Actor별 고정 메시지 키 하나에 설정 헤더와 target별 줄을 매 프레임 덮어쓴다. 상태는 target마다 독립이다. 설정 키를 바꾸면 이전 설정의 판정·overlay를 초기화한다.
- 작은 모듈의 기본 빌드가 개별 cpp로 컴파일되어 실제 unity 검증에는 git 무시 `Saved/UnrealBuildTool/BuildConfiguration.xml`에서 `bForceUnityBuild=true`, `bUseAdaptiveUnityBuild=false`를 잠시 적용했다. `Module.ReachStudy.cpp`에 제품·시험 cpp가 함께 포함된 것을 확인하고 설정은 원복했다.
- 자동화 실행의 `-log=` 인자가 PowerShell에서 분리돼 실제 로그는 `Saved/Logs/ReachStudy.log`에 생성됐다. 같은 로그를 `S2-Automation.log`로 복사해 보존했다. 시험은 재실행하지 않았다.
- 자동화 시작 시 ISDK의 `/OculusInteractionSamples/Widgets/InfoCardWidget` 누락 CDO 오류 2건, 헤드셋 미연결 경고·Meta 에셋 버전 경고가 있었다. 시험 구간 9개는 모두 성공했다. 샘플 플러그인을 추가하거나 원본 플러그인을 수정하지 않았다.
- 자동화 시작 시 엔진이 자동 추가한 AndroidFileServer 설정은 작업 전 상태로 되돌렸다.

### ④ 엔진·플러그인 API 근거

다음은 로컬 파일을 직접 읽어 확인했다. 엔진 경로의 기준은 `C:/Program Files/Epic Games/UE_5.7/Engine/`이다.

- XR 원본: `Plugins/Runtime/XRBase/Source/XRBase/Public/HeadMountedDisplayFunctionLibrary.h:352` (`GetHandTrackingState`), `Source/Runtime/HeadMountedDisplay/Public/HeadMountedDisplayTypes.h:260` (`EHandKeypoint`), `:290` (26개), `:421` (TrackingStatus), `:425` (위치), `:433` (반지름). 공급 경로도 `Plugins/Runtime/OpenXR/Source/OpenXRHMD/Private/OpenXRHMD.cpp:387`에서 확인.
- MotionController: `Source/Runtime/HeadMountedDisplay/Public/MotionControllerComponent.h:32` (low-latency 옵션), `:52` (`SetTrackingMotionSource`).
- 거리: `Source/Runtime/Engine/Classes/Components/PrimitiveComponent.h:2248` (`GetBodyInstance`), `:2271` (`GetSquaredDistanceToCollision`). `Source/Runtime/Engine/Private/PhysicsEngine/Experimental/PhysInterface_Chaos.cpp:1227` convex 필터·`:1259` 실패 처리도 확인.
- complex overlap: `Source/Runtime/Engine/Classes/PhysicsEngine/BodyInstance.h:1151` (body 유효성), `:1342` (`OverlapTest`, 마지막 인자 `bTraceComplex`), `Source/Runtime/PhysicsCore/Public/CollisionShape.h:300` (`MakeSphere`). `PhysInterface_Chaos.cpp:1145` complex/simple 필터 및 `Source/Runtime/Engine/Private/PhysicsEngine/BodyInstance.cpp:4257` 전달 경로도 확인.
- overlay: `Source/Runtime/Engine/Classes/Components/MeshComponent.h:56`, `Source/Runtime/Engine/Public/Materials/MaterialInstanceDynamic.h:109` (벡터 파라미터), `:176` (MID 생성).
- 화면·키: `Source/Runtime/Engine/Classes/Engine/Engine.h:2138` (uint64 메시지 키), `:2150` (삭제), `Source/Runtime/Engine/Classes/Components/InputComponent.h:1030` (키를 전달하는 BindKey).
- ISDK rig: 리포의 `Plugins/MetaXRInteraction/Source/OculusInteractionPrebuilts/Public/Rig/IsdkHandRigComponent.h:142,161`, 같은 폴더 `IsdkRigComponent.h:193,200,207,214,350`. grab 방식 차단: `Plugins/MetaXRInteraction/Source/OculusInteraction/Public/Interaction/IsdkGrabberComponent.h:200`.
- material 경로: `Plugins/MetaXRInteraction/Source/OculusInteraction/Public/IsdkContentAssetPaths.h:137`. `ColorParam` 이름은 명세와 로컬 참고 코드 `C:/Git/vBoR/Source/vBoR/Interaction/VBoRAxisPart.cpp:665`를 대조했다. MID 생성·overlay 적용은 자동화에서 확인했지만 실제 표시 색은 아직 시각 확인하지 않았다.

### ⑤ 로컬에서 확인할 것

- 빌드 로그: `Saved/Logs/S2-NonUnity.log`, `S2-Unity.log`, `S2-Unity-Actual.log`. 자동화: `Saved/Logs/S2-Automation.log` (원본 `ReachStudy.log`). 모두 git 밖.
- Claude: `main...codex/S2-contact-probe` 코드 리뷰. 이번 보고는 코드 리뷰 통과를 뜻하지 않는다.
- 사용자: 명세 §6의 `L_S2_ContactProbe`·대상 배치, GameMode Override·query collision 설정, PIE 경고 확인. ISDK 샘플 위젯 누락도 사용자 실행 환경에서 확인.
- 사용자: Quest 3 Link T-02 전체. 손 모델 표시와 ray/grab 없음, XR `bValid`·관절·반지름, 실제 overlay 색, A/B 비교, 부위 3종, dwell·경계·손별 유예·비용을 확인한다. 원본 손 관절이 오지 않으면 명세 D1대로 질문하며 플러그인을 임의로 추가하지 않는다.
- Quest 결과로 방식·기본값·표면 뒤 처리 결정을 내려야 S2 단계가 끝난다. 현재 T-02 `not-tested`.
- 로컬 커밋만 남긴다. push·PR·main 머지 없음.
