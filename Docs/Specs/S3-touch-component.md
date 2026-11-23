# S3 닿음 component — mesh 밑에 붙이면 닿는 동안 색이 바뀐다

| 항목 | 값 |
|---|---|
| 단계 | 계획서 §6 S3. 시험 T-02(제품 경로)·T-03 일부 |
| 결정 근거 | 계획서 D-02~D-05, D-08, **D-13(사용자 2026-11-23: target = mesh 밑에 붙이는 component, BP마다 개수가 다름)**, Q-04(설정은 레벨에 하나). S2 리뷰 제안 S-1·S-2. **진행자 PC 화면 없음(Q-06)** — 헤드셋에도 PC에도 메시지 없음, 범위는 닿으면 색 바뀜까지(사용자 2026-11-23) |
| 베이스 브랜치 | **S4b가 `main`에 머지된 뒤의 `main`** (`AReachDoorVariants`·S4 개정 1, 화면 메시지 꺼짐 S4b) |
| 작업 브랜치 | `codex/S3-touch-component` |
| 리뷰 파일 | `Docs/Reviews/S3-touch-component.md` |
| 에디터 작업 | **Codex는 하지 않는다.** 구현·리뷰 뒤 Claude가 MCP로 한다(§6) |
| 바뀐 계획 | 계획서의 "`AReachTarget` actor 하나 = target mesh 하나"는 없앤다. CAD Door 하나에 target이 여러 개라 actor를 따로 만들어 붙이는 일이 생긴다(사용자 2026-11-23) |

## 0. 확인한 사실

- S2 판정은 **actor 단위**다. `ReachContact::Query(AActor*)`는 actor의 모든 `UStaticMeshComponent`를 합쳐 본다(`Source/ReachStudy/Contact/ReachContactQuery.cpp:49-98`). `ApplyOverlay(AActor*)`도 actor의 모든 mesh에 overlay를 건다(`:100-105`). 한 BP 안의 mesh 하나만 바꾸려면 component 단위 함수가 필요하다(D4).
- 판정에 쓰는 함수는 모두 `UPrimitiveComponent`에 있다: `GetBodyInstance`, `IsQueryCollisionEnabled`(`Engine/Source/Runtime/Engine/Classes/Components/SceneComponent.h:1265`), `GetSquaredDistanceToCollision`(`Classes/Components/PrimitiveComponent.h:2271`). overlay는 `UMeshComponent::SetOverlayMaterial`(`Classes/Components/MeshComponent.h:56`).
- 손 읽기는 Pawn과 무관한 엔진 함수다: `AReachHandPawn::ReadHand`는 `GetHandTrackingState(GetWorld(), ...)` + `ReachContact::ReadSample`뿐이다(`Source/ReachStudy/Hand/ReachHandPawn.cpp:66-71`).
- body 경계 상자: `FBodyInstance::GetBodyBounds()`(`Classes/PhysicsEngine/BodyInstance.h:927`). 그리기 경계: `USceneComponent::Bounds`(`SceneComponent.h:129`).
- 매 프레임 도는 world 단위 객체: `UTickableWorldSubsystem`(`Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h:75`), world 종류 제한 `DoesSupportWorldType`(`:61`).
- 꺼진 Door는 actor collision이 꺼져 질의 결과가 "collision 없음"이다(S4 §0). 그 Door에 붙은 자식 actor도 같이 꺼진다(S4 D1·D3).

## 1. 결정

| # | 결정 | 내용 |
|---|---|---|
| D1 | **`UReachTouchComponent`** (C++ `USceneComponent`, `Source/ReachStudy/Touch/`, BP에서 추가 가능 `BlueprintSpawnableComponent`) | **닿아야 할 mesh 밑에 붙인다. 붙은 부모 mesh가 target이다.** BP 하나에 몇 개든 붙인다(D-13). 부모가 `UMeshComponent`가 아니면 BeginPlay에 `Warning` 한 줄, 판정하지 않는다. 같은 mesh 밑에 두 개 이상이면 `Warning` 한 줄, 먼저 등록된 것만 판정한다 |
| D2 | **component 프로퍼티·이벤트** | `TouchMaterial`(UMaterialInterface, 비면 설정의 기본 색 D3). `OnTouchBegin`·`OnTouchEnd`(dynamic multicast, BlueprintAssignable, 인자 이 component). `GetPhase()`(BlueprintPure, UENUM `EReachTouchPhase` `Idle`/`Dwelling`/`Touching` — S2 `EReachContactPhase`와 1:1), `IsTouching()`. **BP에서는 component를 붙이고 material만 고른다. 판정·상태는 C++** (AGENTS.md "BP에는 표현만") |
| D3 | **설정 `AReachTouchSettings`** (C++ Actor, `Touch/`, 레벨에 하나, Q-04) | `ContactPart`(기본 `Fingertips`), `ContactMethod`(기본 `ComplexOverlap` — **T-02 뒤 확정**, 그때까지 두 방식 모두 남긴다), `ContactDistanceMm` 10(0–50), `ReleaseMarginMm` 5(0–20), `DwellSeconds` 0.15(0–2), `TrackingGraceSeconds` 0.1(0–1), `bUseJointRadius` false(S2 시험대와 같은 기본, T-02 뒤 확정), `TouchColor`(초록), `DoorVariants`(비면 world의 첫 `AReachDoorVariants`, 없으면 Door 없이). 값은 매 프레임 읽는다(PIE 중 Details에서 바꾸면 바로 적용). 범위는 `ClampMin`/`ClampMax`. **target별 덮어쓰기 없음**(Q-04) — `TouchMaterial`은 판정값이 아니라 표현이다.<br>- 레벨에 없으면 위 기본값으로 판정하고 `Warning` 한 줄. 둘 이상이면 첫 번째 + `Warning` 한 줄 |
| D4 | **component 단위 판정 함수** (`Contact/`) | `ReachContact::QueryComponent(UPrimitiveComponent*, Hand, ContactDistanceMm, ReleaseMarginMm, bUseJointRadius)` — 지금 `Query`의 mesh 하나 몫(`ReachContactQuery.cpp:57-89`)을 그대로 옮긴다. `Query(AActor*)`는 actor의 mesh마다 `QueryComponent`를 불러 지금과 같이 합친다(가장 짧은 거리, Enter > Hold > None, collision 하나라도 있으면 true). `ApplyOverlay(UMeshComponent*, Material)` 추가. **S2 결과는 한 비트도 바뀌지 않는다**(S2 시험 그대로 통과 + E13) |
| D5 | **손 읽기 함수를 Pawn 밖으로** | `ReachContact::ReadHand(const UObject* WorldContext, EControllerHand, EReachContactPart)`를 `Contact/`에 두고 `AReachHandPawn::ReadHand`는 그것을 부른다(동작 같음). S3 판정은 Pawn 없이 이 함수로 읽는다 — Pawn이 다른 클래스여도 판정은 된다 |
| D6 | **판정 실행 = `UReachTouchSubsystem`** (`UTickableWorldSubsystem`, `Touch/`) | **Game·PIE world에서만**(`DoesSupportWorldType`) — 에디터 world에서는 돌지 않는다. component는 BeginPlay에 등록, EndPlay에 해제. 매 프레임: ① 손 두 개를 한 번 읽는다(D5) ② 등록된 component마다 — Door가 꺼진 owner(`IsActorActive(GetOwner())` false)면 상태를 비우고 overlay 끔, 판정 생략 — 아니면 넓은 거르기(D7) → `QueryComponent` 두 손 → S2 `FReachContactState::Step`(component마다 하나, 손별 이력 포함) ③ `Touching`이면 부모 mesh에 overlay, 아니면 끔 ④ **모든 component를 갱신한 뒤** 이번 프레임에 바뀐 것의 `OnTouchBegin`/`OnTouchEnd`를 부른다(BP 처리기가 목록을 바꿔도 순회가 깨지지 않게). 방식 A/B 선택은 S2 시험대와 같다(`Simple` / `Complex` 조건) |
| D7 | **넓은 거르기 (결과를 바꾸지 않는 빠른 길)** | component마다 부모의 `Bounds` 상자와 `GetBodyBounds()`를 합친 상자를 (`ContactDistanceMm` + `ReleaseMarginMm` + 그 손 관절 반지름 중 최대값)만큼 키운다. 한 손의 모든 관절이 그 상자 밖이면 그 손은 `QueryComponent`를 부르지 않고 조건 `None`이다. 두 상자를 합치는 이유: simple collision이 그리기 경계 밖으로 나올 수 있다. **거르기가 있든 없든 Enter/Hold/None은 같아야 한다**(E12) |
| D8 | **Door 전환 = 모든 상태 초기화** (D-08) | 설정의 `DoorVariants`가 있으면 `OnVariantChanged`에 묶는다. 바뀌면 그 자리에서 **모든 component**(Door·IP 모두)의 상태·손 이력을 비우고 overlay를 끈다. 그때 `Touching`이던 component는 `OnTouchEnd`를 한 번 부른다. 다음 첫 실측 프레임은 판정하지 않는다(S2 D4 그대로) |
| D9 | **화면 글 없음 — 알림은 로그로** | S3는 화면에 아무것도 쓰지 않는다(S4b §2: 헤드셋·PC 창에 메시지 없음). Pawn이 `AReachHandPawn`이 아니면 시작 때 `Warning` 로그 한 줄 `Pawn is not ReachHandPawn (hands hidden, no poke)` — **S2 리뷰 S-1**. 판정은 Pawn 없이도 된다(D5) |
| D10 | **`AReachGameMode`** (`Source/ReachStudy/`, 제품) | `DefaultPawnClass = AReachHandPawn`. S5 `L_Main`과 §6 시험 레벨이 쓴다. 제품 코드가 `Development/AReachProbeGameMode`를 쓰지 않게 |
| D11 | **dwell 누적 (S2 리뷰 S-2)** | 그대로 둔다. `DwellElapsedSeconds += DeltaSeconds`는 경계를 최대 한 프레임(90 Hz에서 11 ms) 늦게 넘는다 — 프레임 단위로 판정하는 이상 피할 수 없는 크기이고, reach 확인에 의미 있는 차이가 아니다 |

## 2. 지켜야 할 조건

- 닿음 판정은 XR 원본 관절(S2 D2). poke limiter·손 모델을 보지 않는다. 판정 영역을 키우지 않는다 — 여유는 `ContactDistanceMm`·`ReleaseMarginMm`뿐(AGENTS.md). 넓은 거르기는 판정을 줄이기만 하고 늘리지 않는다.
- 바꾸는 것은 **부모 mesh 하나의 overlay material**뿐. slot material·mesh·collision·transform, 같은 BP의 다른 mesh는 건드리지 않는다. EndPlay에서 overlay를 끈다(이벤트 없이).
- 개수 제한 없음. component 목록은 등록으로 정해지고 코드에 개수·이름이 없다.
- 제품 코드(`Touch/`, `Contact/`, `Door/`, `Hand/`)는 `Development/`를 참조하지 않는다. S2 시험대는 그대로 둔다(방식·값 비교용).
- **한 mesh를 S2 시험대 `Targets`와 touch component에 함께 쓰지 않는다** — 둘 다 같은 overlay를 쓴다. 시험 레벨을 나눈다(§6).
- 기록 없음(D-07). 화면 글 없음(S4b §2).

## 3. 범위 밖

- 정렬(S1). 진행자 PC 화면(만들지 않음, 계획서 Q-06).
- CAD import, Datasmith actor를 BP로 묶는 작업(S5, 에디터 Convert to Blueprint).
- 방식 A/B·기본값 확정(T-02 Quest 실측 뒤 별도 개정).
- target별 설정 덮어쓰기(Q-04), 닿음 소리·연출(BP에서 이벤트에 붙임).

## 4. 경계 상황과 시험 (자동화 `ReachStudy.S3.*`)

엔진 world에 actor·component를 직접 놓는다(S2·S4 시험과 같은 방식). 손은 합성 `FXRHandTrackingState`(관절 26개 위치·반지름, 추적 상태)를 넣는다 — 부위 설정(`ContactPart`)이 subsystem 안에서 적용되는 것까지 시험되게, 프레임 처리를 원본 추적 상태를 인자로 받는 함수로 나눈다(§5).

| # | 상황 | 기대 |
|---|---|---|
| E1 | actor 하나에 큐브 mesh 두 개(M1·M2), M1 밑에만 component. M1 표면에 손끝, dwell 0.15 s, 0.011 s 프레임 | 첫 실측 프레임 판정 안 함 → `Dwelling` → 0.15 s 넘은 프레임에 `Touching`, M1 overlay 켬, **M2 overlay 없음**, `OnTouchBegin` 한 번 |
| E2 | 손을 표면에서 12 mm (허용 10 + 여유 5 안) / 16 mm | `Touching` 유지 / `Idle`, overlay 끔, `OnTouchEnd` 한 번 |
| E3 | actor A에 component 3개, actor B에 7개 | 10개 모두 등록·판정. A의 하나만 닿으면 그것만 `Touching` |
| E4 | 손 추적 0.05 s 끊김 / 0.2 s 끊김 (grace 0.1) | `Touching` 유지 / `Idle` (S2 D4 연결 확인) |
| E5 | 손바닥(`Palm`)만 표면에 — `IndexTip` / `Fingertips` / `WholeHand` | 앞의 둘은 안 닿음, `WholeHand`만 닿음 |
| E6 | `TouchMaterial` 지정 / 비움 | 지정한 material / 설정 `TouchColor`의 기본 material |
| E7 | 방식 `SimpleDistance` / `ComplexOverlap` 설정 변경 | 각 방식 조건으로 판정(S2와 같은 결과) |
| E8 | Door A(부모 actor + 붙은 자식 actor의 component) `Touching` 중 `SelectVariant(1)`, IP component도 `Touching` | 그 자리에서 Door A·IP 모두 `Idle`, overlay 끔, 각 `OnTouchEnd` 한 번. 다음 프레임 첫 실측 판정 안 함. 꺼진 Door A는 손이 있어도 `Idle` |
| E9 | 부모가 mesh가 아닌 component / 한 mesh 밑에 component 둘 | 각각 경고 한 줄, 첫째는 판정 안 함, 둘째는 하나만 판정. 크래시 없음 |
| E10 | 설정 actor 없음 / 둘 | 기본값으로 판정 + 경고 한 줄 / 첫째 사용 + 경고 한 줄 |
| E11 | `OnTouchBegin` 처리기가 다른 component를 지움 | 크래시 없음, 이번 프레임 다른 component 결과 정상 |
| E12 | 넓은 거르기: target 주변 격자 점들(상자 안·밖·경계), 두 방식, 관절 반지름 켬/끔 | 거르기 켬/끔의 Enter/Hold/None 같음. simple collision이 그리기 경계보다 큰 mesh 포함 |
| E13 | mesh 하나 actor: `Query(actor)` vs `QueryComponent(mesh)` | 결과 같음. `ReachStudy.S2.*` 그대로 통과 |
| E14 | Pawn이 일반 `APawn` | 경고 로그 한 줄, 판정은 그대로 됨. 화면 메시지 없음 |
| E15 | component EndPlay(actor 지움) 중 `Touching` | overlay 끔, 등록 해제, 이벤트 없음 |
| E16 | 에디터 world | subsystem이 생기지 않거나 돌지 않음 — overlay 변화 없음 |

## 5. 구현자가 정할 것

- subsystem의 프레임 처리를 시험용으로 나누는 방법(예: `ProcessFrame(const FXRHandTrackingState& Left, const FXRHandTrackingState& Right, float DeltaSeconds)` 공개 함수 — 입력은 원본 추적 상태, 부위 거르기는 안에서 `ReadSample`로).
- component 등록 자료 구조(이름·개수를 코드에 두지 않는 한).
- 기본 material 만들기 — S2 시험대 `EnsureTouchMaterial`(ISDK `GrabbableLit` + `ColorParam`)과 같은 방식 권장.

## 6. 에디터 작업 — Claude가 MCP로 (구현·리뷰 뒤)

새 레벨 `Content/Development/S3/L_S3_Touch`(S2 시험대 레벨과 나눔, §2):

1. `BP_TestDoorA`(큐브 mesh 3개, 그중 2개 밑에 `ReachTouch`), `BP_TestDoorB`(mesh 4개, 3개에 `ReachTouch`), `BP_TestIP`(mesh 2개, 2개에 `ReachTouch`) — 개수가 서로 다르게(D-13). 하나는 `TouchMaterial`을 다른 색으로.
2. `AReachTouchSettings` 하나, `AReachDoorVariants`(Door A {`BP_TestDoorA`}, Door B {`BP_TestDoorB`}), S4 `BP_DoorPanel`, `SeatStart`, GameMode override `AReachGameMode`.
3. PIE: `Z`/`X`·`N`/`B`로 Door 전환, 경고 없음. 스크린샷.

## 7. 완료 조건

- §4 시험 전부, `ReachStudy.*` 전체(S2·S4 포함) 한 번 통과. 빌드 non-unity(`-DisableUnity -Module=ReachStudy`)·unity 경고·오류 0.
- 보고 ①~⑤(AGENTS.md). ④에 §0의 엔진 경로:줄을 직접 확인했는지.
- (Claude, §6 뒤) PIE 경고 없음.

## 8. Quest 3에서 볼 것 (상태 `not-tested`)

- `L_S3_Touch`에서 BP 안의 mesh 하나만 색이 바뀌는지(옆 mesh는 그대로). 개수가 다른 BP들이 모두 동작하는지.
- Door 전환 뒤 닿음 표시가 모두 꺼지는지, IP target은 바로 다시 닿는지.
- target 수가 많을 때 프레임 저하가 없는지(화면 메시지가 꺼져 있으므로 `stat unit`은 쓰지 않는다 — Meta 성능 오버레이나 로그로 본다).

## 개정 이력

- 2026-11-23 — 초안 (Claude). 사용자 결정 D-13(mesh 밑 component, BP마다 개수 다름)으로 계획서의 `AReachTarget` actor 방식을 바꿈. S2 리뷰 S-1(pawn 경고 → 로그)·S-2(dwell 누적 그대로) 정리. 같은 날 진행자 창 뺌(헤드셋·PC 메시지 없음, S4b).
