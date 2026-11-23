# S2 닿음 판정 시험대 — 손 관절과 target mesh 사이 거리를 어떻게 잴까

| 항목 | 값 |
|---|---|
| 단계 | 계획서 §6 S2. 시험 T-02 |
| 결정 근거 | 계획서 D-02(부위 드롭다운)·D-03(허용 거리)·D-04(dwell)·D-05(닿는 동안만)·Q-03(`Fingertips` = 다섯 손가락 끝). 사용자 2026-11-22 |
| 베이스 브랜치 | `main` |
| 작업 브랜치 | `codex/S2-contact-probe` |
| 리뷰 파일 | `Docs/Reviews/S2-contact-probe.md` |
| 에디터 작업 | 빈 레벨 `Content/Development/S2/L_S2_ContactProbe` 하나 (§6). Codex는 레벨·에셋을 만들지 않는다. **2026-11-23 Claude가 MCP로 만듦(§6)** |
| 정렬 | 없음. 이 시험대는 Buck 정렬과 무관하다(정렬은 S1, 계획서 Q-01) |

## 0. 왜 시험대부터 하나 — 엔진 사실

- **엔진의 "표면까지 거리" 함수는 오목 형상을 재지 않는다.** `UPrimitiveComponent::GetSquaredDistanceToCollision`(`Engine/Source/Runtime/Engine/Classes/Components/PrimitiveComponent.h:2271`)은 Chaos `FPhysInterface_Chaos::GetSquaredDistanceToBody`(`Engine/Source/Runtime/Engine/Private/PhysicsEngine/Experimental/PhysInterface_Chaos.cpp:1190`)로 가는데, convex가 아닌 shape는 건너뛴다(`:1227` `if (!Shape.GetGeometry().IsConvex()) continue;`). 그래서 CAD mesh에 "Use Complex as Simple"을 켜도 거리가 나오지 않는다. 쓰려면 convex 형상(simple collision)이 있어야 한다.
- **CAD mesh에는 보통 쓸 만한 simple collision이 없고**, 곡면 trim을 convex로 덮으면 오목한 자리가 메워진다(허공에서 닿음).
- **complex(삼각형) collision으로 할 수 있는 건 구 overlap이다.** `FBodyInstance::OverlapTest(Position, Rotation, CollisionShape, OutMTD, bTraceComplex)`(`Engine/Source/Runtime/Engine/Classes/PhysicsEngine/BodyInstance.h:1342`, 기본값 `bTraceComplex = false`)에 구(`FCollisionShape::MakeSphere`, `Engine/Source/Runtime/PhysicsCore/Public/CollisionShape.h:300`)와 `bTraceComplex = true`를 주면 complex shape만 검사한다(`PhysInterface_Chaos.cpp:1145-1147` `bShapeIsComplex` 필터). "이 반지름 안에 표면이 있나"는 알지만 거리 값은 없다. 삼각형 mesh는 표면뿐이라, 손이 표면 뒤로 반지름보다 깊이 들어가면 닿지 않은 것이 된다.
  - `UPrimitiveComponent::OverlapComponent`(`PrimitiveComponent.h:3049`)는 `bTraceComplex`를 넘기지 않아(`Engine/Source/Runtime/Engine/Private/Components/PrimitiveComponent.cpp:3732-3736`) simple만 본다 — **쓰지 않는다.** component의 `GetBodyInstance()`로 직접 부른다.
  - sweep(`SweepComponent`, `PrimitiveComponent.h:2986`)은 쓰지 않는다. 한 프레임 통과를 잡아도 dwell(D4)이 있으면 `Touching`이 되지 않고, 빠른 손 이동과 복구 튐을 가르는 규칙이 하나 더 필요해진다.
- 어느 쪽이 실제 CAD target과 Quest 손 떨림에서 쓸 만한지는 **헤드셋에서 숫자로 봐야** 안다. 이 명세는 두 방식을 같은 화면에서 비교하는 시험대와, 방식과 무관한 상태 규칙(순수 구조체)을 만든다. S3는 이 결과로 하나를 고른다.

## 1. 결정

| # | 결정 | 내용 |
|---|---|---|
| D1 | **손 Pawn `AReachHandPawn`** | `Source/ReachStudy/Hand/`. vBoR `AVBoRHandRigPawn` 구성을 따른다(로컬 `C:\Git\vBoR\Source\vBoR\Interaction\VBoRHandRigPawn.cpp:97-115`. 통째로 복사하지 말고 필요한 부분만 다시 쓴다): 루트 Scene, Camera(`bLockToHmd`), MotionController 좌·우(`Left`/`Right`, `bDisableLowLatencyUpdate`), ISDK 손 rig `UIsdkHandRigComponentLeft/Right`(`Plugins/MetaXRInteraction/Source/OculusInteractionPrebuilts/Public/Rig/IsdkHandRigComponent.h:142,161`)를 MotionController 밑에. **손 모양 표시만 쓴다.** 레이·잡기는 vBoR `ConfigurePokeRig`처럼 끈다(`C:\Git\vBoR\Source\vBoR\Interaction\VBoRHandRigPawn.cpp:21-33`, 레이 컴포넌트는 지우지 말고 비활성 + `RayVisualClass = nullptr`, `GetRayInteraction`/`GetGrabInteraction`/`GetGrabber` `IsdkRigComponent.h:193,200,214`). `bAutoBindInputActions = false`(`:350`). **poke limiter는 넣지 않는다**(poke 버튼은 S4).<br>**모듈 의존(`ReachStudy.Build.cs`):** `OculusInteraction`, `OculusInteractionPrebuilts`(ISDK rig, vBoR `vBoR.Build.cs`와 같음), `HeadMountedDisplay`, `XRBase`(`GetHandTrackingState`). 손 추적 공급은 vBoR과 같은 구성(Meta XR, `XrApi=NativeOpenXR`, `Config/DefaultEngine.ini`)이다 — 손 관절이 `bValid`로 오지 않으면 플러그인을 임의로 더하지 말고 질문 |
| D2 | **관절 읽기 = XR 원본** | `UHeadMountedDisplayFunctionLibrary::GetHandTrackingState`(`Engine/Plugins/Runtime/XRBase/Source/XRBase/Public/HeadMountedDisplayFunctionLibrary.h:352`), `EXRSpaceType::UnrealWorldSpace`. 받는 조건: `bValid && TrackingStatus == Tracked`(`Engine/Source/Runtime/HeadMountedDisplay/Public/HeadMountedDisplayTypes.h:421`)이고 쓰는 관절의 `HandKeyLocations`(`:425`)가 유효·NaN 아님. 하나라도 안 맞으면 **그 손은 이번 프레임 추적 안 됨**(일부 관절만 쓰지 않음). 관절 반지름 `HandKeyRadii`(`HeadMountedDisplayTypes.h:433`, OpenXR이 채움 `Engine/Plugins/Runtime/OpenXR/Source/OpenXRHMD/Private/OpenXRHMD.cpp:387`)도 같이 읽는다(D5). ISDK 손 모델·rig 위치를 쓰지 않는다(AGENTS.md "추적된 실제 손 관절") |
| D3 | **부위 모드 `EReachContactPart`** (UENUM, BlueprintType) | `IndexTip` = `EHandKeypoint::IndexTip`. `Fingertips`(**기본**) = `ThumbTip`·`IndexTip`·`MiddleTip`·`RingTip`·`LittleTip`. `WholeHand` = `EHandKeypoint` 전부(`EHandKeypointCount` = 26, `HeadMountedDisplayTypes.h:260-290`). `DisplayName`은 영어 그대로 |
| D4 | **상태 규칙 = 엔진 없는 구조체 `FReachContactState`** | `Source/ReachStudy/Contact/ReachContact.h`. 프레임마다 넣는 것: 손별 판정 가능 여부, 손별 "닿음 조건" 결과(Enter / Hold / None. Enter면 Hold도 참. §1 D5가 만든다), `DeltaSeconds`. 값: `ContactDistanceMm`(기본 10, 0–50), `ReleaseMarginMm`(기본 5, 0–20), `DwellSeconds`(기본 0.15, 0–2), `TrackingGraceSeconds`(기본 0.1, 0–1, 계획서 §4). 상태 `Idle` → `Dwelling` → `Touching`.<br>- **손마다 이번 프레임 결과는 셋 중 하나다** (vBoR "추적 손실은 보류이지 완료가 아니다"):<br>&nbsp;&nbsp;· **실측**: 판정 가능한 손 → D5 결과(Enter/Hold/None).<br>&nbsp;&nbsp;· **보류**: 판정 불가(추적 안 됨, 또는 **다시 추적된 첫 프레임** — 복구 튐)이고, 그 손의 마지막 실측 프레임에서 지금까지 `TrackingGraceSeconds` 이하 → 마지막 실측 결과를 그대로 쓴다. 보류 시간에는 끊긴 프레임과 복구 첫 프레임이 모두 들어간다.<br>&nbsp;&nbsp;· **없음**: 판정 불가이고 보류 시간을 넘음, 또는 실측이 한 번도 없음 → None.<br>- `Idle`/`Dwelling`: 어느 손이든 **실측 Enter**면 dwell 시간을 더한다. 실측 Enter는 없고 **보류 Enter**만 있으면 dwell을 그대로 둔다(멈춤, 상태 그대로). 둘 다 없으면 dwell 0, `Idle`. dwell이 `DwellSeconds` 이상이 되면 `Touching`. `DwellSeconds` 0이면 첫 실측 Enter 프레임에 바로 `Touching`.<br>- `Touching`: 어느 손이든 **Hold**(실측 또는 보류, 거리 ≤ `ContactDistanceMm + ReleaseMarginMm`)면 유지. 아니면 즉시 `Idle`(D-05, 떨어질 때 dwell 없음).<br>- 두 손은 따로 센다. 한 손이 보류 중이어도 다른 손의 실측은 그대로 쓴다.<br>- 상태가 바뀔 때만 바뀜 신호를 낸다(같은 상태 반복 없음) |
| D5 | **거리 방식 둘 — 시험대에서 같이 계산** | target 하나(그 Actor의 모든 `UStaticMeshComponent`)·손 하나마다, 모드의 관절점 전부에 대해:<br>**A `SimpleDistance`**: component마다 `GetSquaredDistanceToCollision`(`PrimitiveComponent.h:2271`), 그중 최소 거리(mm, UE cm → mm 변환 자리에 주석). 값이 없으면(`false` — simple collision 없음, `PhysInterface_Chaos.cpp:1259-1264`) `n/a`로 보이고 Enter·Hold 없음. 0(convex 안)이면 0 mm. Enter = 거리 ≤ `ContactDistanceMm`, Hold = 거리 ≤ `ContactDistanceMm + ReleaseMarginMm`.<br>**B `ComplexOverlap`**: component마다 `GetBodyInstance()->OverlapTest(관절 위치, 단위 회전, MakeSphere(반지름), nullptr, true)`(§0). 반지름 = Enter용 `ContactDistanceMm`, Hold용 `ContactDistanceMm + ReleaseMarginMm`. 거리 값이 없으므로 화면에는 `enter`/`hold`/`-`. body instance가 없으면(collision 꺼짐) `no collision`으로 보이고 Enter·Hold 없음.<br>**관절 반지름 `bUseJointRadius`(기본 false, 키 `R`):** 켜면 A는 거리에서 그 관절 반지름을 빼고(0 아래면 0), B는 구 반지름에 관절 반지름을 더한다 — "관절 중심"이 아니라 "손 겉면"에서 잰다. 반지름이 없거나 0이면 0으로 본다. A 줄에는 켬/끔과 상관없이 관절 반지름(mm)도 보인다. 어느 쪽을 쓸지는 Quest에서 정한다(§8).<br>- 판정에 쓰는 방식은 `ContactMethod`(UENUM, 기본 B `ComplexOverlap`) 하나지만, **화면에는 두 방식 결과를 모두** 보인다(D7) |
| D6 | **닿음 표시 = overlay material** | `UMeshComponent::SetOverlayMaterial`(`Engine/Source/Runtime/Engine/Classes/Components/MeshComponent.h:56`). `Touching` = 닿음 material, 그 밖 = `nullptr`. **target 원래 slot material은 건드리지 않는다**(§2). 닿음 material은 vBoR 상태 overlay와 같은 방식: ISDK `IsdkContentAssetPaths::Materials::GrabbableDefault`(`/OculusInteraction/Materials/GrabbableLit`, `Plugins/MetaXRInteraction/Source/OculusInteraction/Public/IsdkContentAssetPaths.h:137`)의 MID, 벡터 파라미터 `ColorParam`(vBoR `VBoRAxisPart.cpp:251-252,665`) = `TouchColor`(기본 초록). 프로퍼티 `TouchMaterial`이 있으면 그것을 MID 부모로 쓴다. `Dwelling`은 표시하지 않는다(D-05) |
| D7 | **시험대 `AReachContactProbe`** | `Source/ReachStudy/Development/`. 레벨에 놓고 `Targets`(TArray<TObjectPtr<AActor>>)에 시험할 Actor를 넣는다. 각 Actor의 모든 `UStaticMeshComponent`가 그 target의 형상이다. 설정(D3 부위 모드, D4 값 넷, D5 방식·관절 반지름)은 이 Actor 하나에 있다(계획서 Q-04). 손별 보류 기록은 target마다 따로 둔다(같은 손이 target마다 다른 결과를 낸다).<br>**화면(PC 창 `GEngine` 메시지, 매 프레임 덮어씀):** 맨 위에 모드·방식·`ContactDistanceMm`·`ReleaseMarginMm`·`DwellSeconds`·`TrackingGraceSeconds`·관절 반지름 켬/끔, 손별 추적 상태(`L tracked`/`L lost`). target마다 한 줄: 이름, 상태, A 최소 거리 mm(손별)과 가장 가까운 관절 이름, B 결과(손별), 이번 프레임 판정 비용(µs, 모든 target 합은 맨 위에).<br>**키(PIE·Link에서 키보드):** `M` 방식 A↔B, `1`/`2`/`3` 부위 모드, `=`/`-` `ContactDistanceMm` ±1 mm, `]`/`[` `DwellSeconds` ±0.05 s, `.`/`,` `ReleaseMarginMm` ±1 mm, `0`/`9` `TrackingGraceSeconds` ±0.05 s, `R` 관절 반지름 켬/끔(D5). 범위 밖으로 가지 않는다(D4).<br>**기록 파일을 만들지 않는다**(계획서 D-07). 숫자는 사용자가 화면에서 읽는다. `GEngine` 메시지는 헤드셋 화면에도 겹쳐 보일 수 있다 — 시험대에서만 쓰고, 진행자 창은 S4에서 따로 정한다 |
| D8 | **게임 모드** | `AReachProbeGameMode`(기본 Pawn = `AReachHandPawn`). 추적 원점은 플랫폼 기본(UE 5.7 OpenXR = Stage, vBoR CLAUDE.md)을 바꾸지 않는다 |

## 2. 지켜야 할 조건

- 시험대 코드는 target mesh의 slot material·mesh·transform·collision 설정을 바꾸지 않는다. 바꾸는 것은 overlay material 하나뿐이다. (S4부터 Door 전환 `AReachDoorVariants`가 꺼진 Door의 actor 단위 숨김·collision을 끄고 되돌린다 — 시험대가 아니다, S4 §2.) `Touching`이 끝나면 overlay가 `nullptr`로 돌아가 원래 모습과 같다.
- 판정 영역을 보이는 형상보다 키우지 않는다. 여유는 `ContactDistanceMm`(+ Hold의 `ReleaseMarginMm`)뿐이다. 관절 반지름(D5)은 여유가 아니라 관절 중심 → 손 겉면 위치 보정이다(AGENTS.md).
- 레이·원거리 잡기·잡기 없음. 손 모델은 보이기만 한다. (S4부터 Door 전환 버튼용 poke와 poke limiter를 켠다 — 닿음 판정은 그대로 XR 원본 관절, S4 D7.)
- `Source/ReachStudy/Contact/`의 상태 규칙은 시험대(`Development/`)를 참조하지 않는다. S3가 그대로 쓴다.

## 3. 범위 밖

- 제품 target(S3에서 `UReachTouchComponent`로 바뀜, 계획서 D-13)·진행자 창(S3), Door 전환·poke 버튼(S4), 정렬(S1), CAD import(S5).
- 방식 C(mesh 삼각형으로 직접 거리 계산, CPU 접근 필요). A·B가 둘 다 실패하면 리뷰 파일 "질문"에 결과와 함께 적고 멈춘다.
- target 표면 뒤로 깊이 들어간 손 처리(계획서 D-03). 이번에는 화면에 보이는 대로 둔다 — 사용자가 Quest에서 보고 정한다.

## 4. 경계 상황과 시험 (자동화 `ReachStudy.S2.*`)

순수 구조체(D4)는 합성 입력으로, 방식 A·B(D5)는 엔진 world에 engine 기본 mesh를 놓고 관절 위치를 직접 넣어 시험한다(손 추적 없이 — 관절 공급을 시험에서 바꿀 수 있게 하는 방법은 구현자).

| # | 상황 | 기대 |
|---|---|---|
| E1 | Enter가 `DwellSeconds`보다 짧게 이어짐(0.1 s, 기본 0.15) | `Touching` 안 됨, dwell 0으로 돌아감 |
| E2 | Enter가 0.15 s 이어짐 | 그 프레임에 `Touching`, 바뀜 신호 1번 |
| E3 | `DwellSeconds` 0 | 첫 Enter 프레임에 `Touching` |
| E4 | `Touching` 중 거리 12 mm(Enter 밖, Hold 안) | 유지 |
| E5 | `Touching` 중 거리 16 mm | 그 프레임에 `Idle`, overlay `nullptr` |
| E6 | 경계 9 ↔ 11 mm를 프레임마다 오감 | `Touching` 뒤 계속 유지(깜빡임 없음) |
| E7 | `Touching` 중 그 손 추적 끊김 1프레임 → 복구 첫 프레임(둘 합 < `TrackingGraceSeconds`) → 그다음 실측 Hold | 세 프레임 모두 `Touching` 유지, 바뀜 신호 없음 |
| E7b | `Dwelling` 0.1 s 중 끊김 0.05 s → 복구 첫 프레임 → 실측 Enter 계속 | 끊긴 동안·복구 첫 프레임 dwell 0.1 그대로(멈춤), 그다음 실측 Enter 0.05 s로 `Touching` |
| E7c | `Touching` 중 끊김이 `TrackingGraceSeconds`(0.1 s) 넘게 이어짐 | 넘는 프레임에 `Idle`, dwell 0 |
| E7d | 왼손으로 `Touching` 중 왼손 끊김, 오른손은 추적되지만 멀리 있음(실측 None) | 왼손 보류 Hold로 `Touching` 유지. 0.1 s 넘으면 `Idle` (손마다 따로) |
| E7e | 왼손 보류 Enter만 있고 오른손 실측 Enter | dwell은 오른손 실측으로 더해진다 |
| E7f | 처음부터 추적 안 된 손(실측 없음) | 없음(None) — 보류할 결과가 없다 |
| E8 | 왼손 `Touching` 중 왼손 떨어지고 오른손 Hold | 유지 |
| E9 | 부위 모드 3종에서 쓰는 관절 수와 이름 | 1 / 5(엄지 끝 포함) / 26 |
| E10 | `Fingertips`에서 손바닥만 표면에 닿음 (A·B 둘 다) | 안 닿음. `WholeHand`면 닿음 |
| E11 | A: convex 상자 밖 5 mm 점 | 5 mm (±0.1). 상자 안 점 = 0 |
| E12 | A: complex만 있는 mesh(simple collision 없음) | `n/a`, Enter 없음 |
| E13 | B: complex만 있는 mesh 표면 밖 5 mm 점, `ContactDistanceMm` 10 | enter |
| E13b | B: 같은 mesh 표면 밖 12 mm 점 | Enter 아님, Hold(15 mm 안) |
| E14 | B: complex만 있는 닫힌 mesh 안쪽, 표면에서 3 cm 들어간 점 | Enter·Hold 아님(표면만 검사 — 계획서 D-03 자료) |
| E15 | collision이 꺼진 component | A `n/a`, B `no collision`, Enter·Hold 없음 |
| E15b | 관절 반지름 켬, 반지름 8 mm, A 중심 거리 15 mm | A 7 mm → Enter |
| E16 | 값 키 범위 끝 | `ContactDistanceMm` 0·50, `ReleaseMarginMm` 0·20, `DwellSeconds` 0·2, `TrackingGraceSeconds` 0·1에서 멈춤 |
| E17 | overlay 복원 | `Touching` → `Idle` 뒤 slot material 배열이 시작 때와 같음, overlay `nullptr` |

## 5. 구현자가 정할 것

- 시험에서 관절 위치를 넣는 방법(인터페이스·함수 포인터·테스트 전용 setter 등). 제품 경로에 시험 분기를 넣지 않는다.
- 화면 줄 배치·색 값·가장 가까운 관절 이름 표기.
- 판정 비용 측정 방법(`FPlatformTime::Seconds` 차 등).

## 6. 사용자 에디터 작업 (구현 뒤)

> **완료 2026-11-23 — Claude가 MCP로 만듦**(사용자 요청 "에디터 작업은 MCP로 직접", AGENTS.md). 참가자는 원점(`SeatStart`, Stage 바닥 Z=0)에 앉아 +X를 본다. 단위 cm.
> - `T_Cube` 엔진 Cube 20 cm, (50, 15, 80) · `T_Cylinder` 엔진 Cylinder 지름 15·높이 30, (50, −10, 80) · `T_SmallCube` 2 × 2 × 1 cm, (40, 30, 85)
> - `T_CylinderComplex` = `SM_S2_CylinderComplex`(엔진 Cylinder 복제, `CTF_UseComplexAsSimple`, simple 0개), (50, −35, 80) · `T_KneeSide` Cube 10 cm, (20, −40, 55) — Door 쪽(−Y) 무릎 옆
> - `ContactProbe`의 `Targets` = 위 5개, GameMode Override = `ReachProbeGameMode`, 모든 target collision `QueryAndPhysics`. 바닥 Plane·Directional·Sky light는 보기용
> - PIE: `Game class is 'ReachProbeGameMode'`, ReachStudy 경고 없음(헤드셋 없음 `LogHMD` 잡음만). 배치 스크린샷 `Saved/Screenshots/S2_Layout.png`(git 밖)
> - 자리는 근사값이다. 실제 앉은 자세에 안 맞으면 에디터에서 옮긴다. 회사 CAD 조각(아래 2번 마지막 줄)은 아직 없다

1. 빈 레벨 `Content/Development/S2/L_S2_ContactProbe`, World Settings GameMode Override = `AReachProbeGameMode`.
2. target 넣기 — 각각 Actor로 놓고 `AReachContactProbe`의 `Targets`에:
   - 엔진 Cube(simple 있음), 엔진 Cylinder(곡면), 작은 Cube(2 × 2 × 1 cm, 작은 버튼 크기)
   - 같은 Cylinder 복사본 하나: mesh 에셋 복사 → Collision Complexity `Use Complex Collision As Simple` (CAD처럼 simple 없음 흉내)
   - **회사 PC에서는** CAD target 조각 하나(`Content/CAD/`, git 밖)
3. 앉은 자세로 손이 닿는 높이(바닥 60–90 cm)에, 무릎 옆(Door 아래쪽 흉내) 하나 포함.
4. 모든 target의 Collision Presets가 Query를 켠 것(엔진 기본 `BlockAll` 등)인지 본다. **CAD(Datasmith) mesh는 collision이 꺼져 있을 수 있다** — 화면에 `no collision`이 뜨면 그 component의 collision을 켠다(코드는 바꾸지 않는다, §2).

## 7. 완료 조건

- §4 시험 전부 통과. 빌드 non-unity(`-DisableUnity -Module=ReachStudy`)·unity 경고·오류 0. `ReachStudy.*` 전체 한 번.
- (사용자, 레벨을 만든 뒤) `L_S2_ContactProbe` PIE에서 `AReachContactProbe` 경고 없음. Codex는 레벨이 없으므로 확인하지 않는다.
- 보고 ①~⑤(AGENTS.md). ④에 위 엔진·ISDK 경로:줄을 직접 확인했는지.

## 8. Quest 3에서 볼 것 (T-02, 상태 `not-tested`)

- 방식 A·B 각각, 부위 3종에서 엔진 Cube·Cylinder·작은 Cube·complex만 있는 Cylinder 닿음.
- 손을 표면에 대고 가만히 있을 때 A 거리 값의 흔들림(mm) — `ContactDistanceMm`·`ReleaseMarginMm` 기본값 근거.
- 다른 target을 지나가며 스칠 때 `DwellSeconds` 0.15에서 걸러지는지, 의도한 닿음이 늦게 느껴지는지.
- 무릎 옆 target: 고개를 돌리지 않았을 때 추적 상태(`lost`가 얼마나 뜨나).
- 손을 표면 뒤로 깊이 넣었을 때 A(convex 안 = 0)와 B(빠짐) 차이 → 계획서 D-03 결정.
- 관절 반지름 켬/끔(`R`): 손끝 반지름이 몇 mm로 오는지, 어느 쪽이 "손이 표면에 닿아 보이는 순간"과 색 바뀜이 잘 맞는지 → `ContactDistanceMm`의 의미 결정.
- 한 손을 무릎 옆 target에 댄 채 다른 손은 보이게 두고 고개를 돌려, 닿은 손이 잠깐 시야 밖으로 나갈 때 색이 깜빡이는지(`0`/`9`로 값 바꿔 가며) → `TrackingGraceSeconds` 기본값.
- 판정 비용(µs) — 회사 CAD 조각에서.

## 개정 이력

- 2026-11-22 — 초안 (Claude).
- 2026-11-22 개정 1 (Claude 재검토, 구현 전): ① 방식 B를 sweep → complex 구 overlap(`FBodyInstance::OverlapTest` + `bTraceComplex`). sweep은 dwell 때문에 이득이 없고, 2 cm 튐 한계가 빠른 손 이동(90 Hz에서 2 m/s ≈ 2.2 cm/프레임)에서 깨짐. `OverlapComponent`는 complex를 보지 않아 제외. ② 추적 끊김을 즉시 `Idle` → `TrackingGraceSeconds` 보류(vBoR 원칙, 1프레임 끊김 깜빡임 방지). ③ `Build.cs` 모듈 의존 명시. ④ 관절 반지름 `HandKeyRadii` 켬/끔 비교. ⑤ overlay material을 vBoR 방식(ISDK GrabbableLit `ColorParam`)으로 확정. ⑥ CAD collision 꺼짐 처리, vBoR 경로를 로컬 절대경로로, Codex가 할 수 없는 레벨 확인을 완료 조건에서 분리.
- 2026-11-22 개정 2 (Codex 질문 Q-02 + Claude 전체 대조): ① 계획서 §4·D-04·§5·§8·T-02, AGENTS.md를 이 명세와 맞춤. ② **D4 보류를 손별로** — 개정 1은 "판정할 손이 하나도 없을 때만" 보류라서, 닿은 손만 시야 밖으로 나가고 다른 손은 보이는 흔한 경우(무릎 옆 Door target)에 바로 `Idle`이 됐다. E7d 기대값 바뀜, E7e·E7f 추가. 복구 첫 프레임이 보류 시간에 들어감을 명시. ③ `TrackingGraceSeconds` 키 `0`/`9`, E16. ④ 관절 반지름은 여유가 아님(§2·AGENTS.md).
- 2026-11-23 개정 3 (S4 명세와 맞춤, 동작 변경 없음): §2 collision·손 모델 문장에 S4의 Door 전환·poke를 괄호로 적음, §3 진행자 창 S4 → S3.
- 2026-11-23 개정 4 (사용자 결정, S4b): **시험대 화면 글(D7 "화면")을 없앤다** — 헤드셋에 어떤 메시지도 없다. 판정·키·overlay는 그대로. T-02는 숫자 대신 색 변화와 키로 바꾼 값으로 본다. 구현은 S4b D4.
