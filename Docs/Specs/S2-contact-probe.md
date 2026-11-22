# S2 닿음 판정 시험대 — 손 관절과 target mesh 사이 거리를 어떻게 잴까

| 항목 | 값 |
|---|---|
| 단계 | 계획서 §6 S2. 시험 T-02 |
| 결정 근거 | 계획서 D-02(부위 드롭다운)·D-03(허용 거리)·D-04(dwell)·D-05(닿는 동안만)·Q-03(`Fingertips` = 다섯 손가락 끝). 사용자 2026-11-22 |
| 베이스 브랜치 | `main` |
| 작업 브랜치 | `codex/S2-contact-probe` |
| 리뷰 파일 | `Docs/Reviews/S2-contact-probe.md` (첫 리뷰 때 생김) |
| 에디터 작업 | **사용자:** 빈 레벨 `Content/Development/S2/L_S2_ContactProbe` 하나 (§6). Codex는 레벨·에셋을 만들지 않는다 |
| 정렬 | 없음. 이 시험대는 Buck 정렬과 무관하다(정렬은 S1, 계획서 Q-01) |

## 0. 왜 시험대부터 하나 — 엔진 사실

- **엔진의 "표면까지 거리" 함수는 오목 형상을 재지 않는다.** `UPrimitiveComponent::GetSquaredDistanceToCollision`(`Engine/Source/Runtime/Engine/Classes/Components/PrimitiveComponent.h:2271`)은 Chaos `FPhysInterface_Chaos::GetSquaredDistanceToBody`(`Engine/Source/Runtime/Engine/Private/PhysicsEngine/Experimental/PhysInterface_Chaos.cpp:1190`)로 가는데, convex가 아닌 shape는 건너뛴다(`:1227` `if (!Shape.GetGeometry().IsConvex()) continue;`). 그래서 CAD mesh에 "Use Complex as Simple"을 켜도 거리가 나오지 않는다. 쓰려면 convex 형상(simple collision)이 있어야 한다.
- **CAD mesh에는 보통 쓸 만한 simple collision이 없고**, 곡면 trim을 convex로 덮으면 오목한 자리가 메워진다(허공에서 닿음).
- **complex(삼각형) collision으로 할 수 있는 건 sweep이다.** `UPrimitiveComponent::SweepComponent(..., const FCollisionShape&, bool bTraceComplex)`(`PrimitiveComponent.h:2986`)에 구(`FCollisionShape::MakeSphere`, `Engine/Source/Runtime/PhysicsCore/Public/CollisionShape.h:300`)를 넣으면 "이 반지름 안에 표면이 있나"는 알 수 있지만, 거리 값은 없다. 손이 얇은 표면 뒤로 반지름보다 깊이 들어가면 닿지 않은 것이 된다.
- 어느 쪽이 실제 CAD target과 Quest 손 떨림에서 쓸 만한지는 **헤드셋에서 숫자로 봐야** 안다. 이 명세는 두 방식을 같은 화면에서 비교하는 시험대와, 방식과 무관한 상태 규칙(순수 구조체)을 만든다. S3는 이 결과로 하나를 고른다.

## 1. 결정

| # | 결정 | 내용 |
|---|---|---|
| D1 | **손 Pawn `AReachHandPawn`** | `Source/ReachStudy/Hand/`. vBoR `AVBoRHandRigPawn` 구성을 따른다(vBoR `Source/vBoR/Interaction/VBoRHandRigPawn.cpp:97-115`): 루트 Scene, Camera(`bLockToHmd`), MotionController 좌·우(`Left`/`Right`, `bDisableLowLatencyUpdate`), ISDK 손 rig `UIsdkHandRigComponentLeft/Right`(`Plugins/MetaXRInteraction/Source/OculusInteractionPrebuilts/Public/Rig/IsdkHandRigComponent.h:142,161`)를 MotionController 밑에. **손 모양 표시만 쓴다.** 레이·잡기는 vBoR `ConfigurePokeRig`처럼 끈다(vBoR `VBoRHandRigPawn.cpp:21-33`, 레이 컴포넌트는 지우지 말고 비활성 + `RayVisualClass = nullptr`, `GetRayInteraction`/`GetGrabInteraction`/`GetGrabber` `IsdkRigComponent.h:193,200,214`). `bAutoBindInputActions = false`(`:350`). **poke limiter는 넣지 않는다**(poke 버튼은 S4) |
| D2 | **관절 읽기 = XR 원본** | `UHeadMountedDisplayFunctionLibrary::GetHandTrackingState`(`Engine/Plugins/Runtime/XRBase/Source/XRBase/Public/HeadMountedDisplayFunctionLibrary.h:352`), `EXRSpaceType::UnrealWorldSpace`. 받는 조건: `bValid && TrackingStatus == Tracked`(`Engine/Source/Runtime/HeadMountedDisplay/Public/HeadMountedDisplayTypes.h:421`)이고 쓰는 관절의 `HandKeyLocations`(`:425`)가 유효·NaN 아님. 하나라도 안 맞으면 **그 손은 이번 프레임 추적 안 됨**(일부 관절만 쓰지 않음). ISDK 손 모델·rig 위치를 쓰지 않는다(AGENTS.md "추적된 실제 손 관절") |
| D3 | **부위 모드 `EReachContactPart`** (UENUM, BlueprintType) | `IndexTip` = `EHandKeypoint::IndexTip`. `Fingertips`(**기본**) = `ThumbTip`·`IndexTip`·`MiddleTip`·`RingTip`·`LittleTip`. `WholeHand` = `EHandKeypoint` 전부(`EHandKeypointCount` = 26, `HeadMountedDisplayTypes.h:260-290`). `DisplayName`은 영어 그대로 |
| D4 | **상태 규칙 = 엔진 없는 구조체 `FReachContactState`** | `Source/ReachStudy/Contact/ReachContact.h`. 프레임마다 넣는 것: 손별 `bTracked`, 손별 "닿음 조건" 결과(Enter/Hold/None — §1 D5가 만든다), `DeltaSeconds`. 값: `ContactDistanceMm`(기본 10, 0–50), `ReleaseMarginMm`(기본 5, 0–20), `DwellSeconds`(기본 0.15, 0–2). 상태 `Idle` → `Dwelling` → `Touching`.<br>- `Idle`/`Dwelling`: 어느 손이든 **Enter**(거리 ≤ `ContactDistanceMm`)면 dwell 시간을 더한다. `DwellSeconds` 이상이 되면 `Touching`. 같은 프레임에 Enter가 없으면 dwell 0, `Idle`. `DwellSeconds` 0이면 첫 Enter 프레임에 바로 `Touching`.<br>- `Touching`: 어느 손이든 **Hold**(거리 ≤ `ContactDistanceMm + ReleaseMarginMm`)면 유지. 아니면 즉시 `Idle`(D-05, 떨어질 때 dwell 없음).<br>- **추적 끊김:** 추적되지 않은 손은 Enter·Hold를 낼 수 없다. **다시 추적된 첫 프레임의 그 손도 판정하지 않는다**(복구 튐, AGENTS.md). 두 손 모두 못 내면 `Touching` → `Idle`, dwell 0.<br>- 두 손이 같은 target에 닿으면 하나라도 조건을 만족하면 된다.<br>- 상태가 바뀔 때만 바뀜 신호를 낸다(같은 상태 반복 없음) |
| D5 | **거리 방식 둘 — 시험대에서 같이 계산** | target 하나·손 하나마다, 모드의 관절점 전부에 대해:<br>**A `SimpleDistance`**: `GetSquaredDistanceToCollision`으로 최소 거리(mm, UE cm → mm 변환 자리에 주석). 값이 없으면(`false` — collision 없음·오목만 있음) `n/a`로 보이고 Enter·Hold 없음. 0(convex 안)이면 0 mm.<br>**B `ComplexSweep`**: `SweepComponent`, 구 반지름 = Enter용 `ContactDistanceMm`, Hold용 `ContactDistanceMm + ReleaseMarginMm`, `bTraceComplex = true`. **시작점 = 지난 프레임의 같은 관절 위치, 끝점 = 이번 위치**(손이 한 프레임에 표면을 통과해도 잡는다). 지난 위치가 없거나(첫 프레임·추적 복구) 2 cm 넘게 떨어졌으면(복구 튐) 시작 = 끝. 거리 값은 없으므로 화면에는 `hit`/`hold`/`-`.<br>- 판정에 쓰는 방식은 `ContactMethod`(UENUM, 기본 B) 하나지만, **화면에는 두 방식 결과를 모두** 보인다(D7) |
| D6 | **닿음 표시 = overlay material** | `UMeshComponent::SetOverlayMaterial`(`Engine/Source/Runtime/Engine/Classes/Components/MeshComponent.h:56`). `Touching` = 닿음 material, 그 밖 = `nullptr`. **target 원래 slot material은 건드리지 않는다**(§2). 닿음 material 프로퍼티(`TouchMaterial`)가 비면 엔진 기본 material 기반 MID를 런타임에 만든다(색 = `TouchColor`, 기본 초록). `Dwelling`은 표시하지 않는다(D-05) |
| D7 | **시험대 `AReachContactProbe`** | `Source/ReachStudy/Development/`. 레벨에 놓고 `Targets`(TArray<TObjectPtr<AActor>>)에 시험할 Actor를 넣는다. 각 Actor의 모든 `UStaticMeshComponent`가 그 target의 형상이다. 설정 D3–D5 값은 이 Actor 하나에 있다(계획서 Q-04).<br>**화면(PC 창 `GEngine` 메시지, 매 프레임 덮어씀):** 맨 위에 모드·방식·`ContactDistanceMm`·`ReleaseMarginMm`·`DwellSeconds`, 손별 추적 상태(`L tracked`/`L lost`). target마다 한 줄: 이름, 상태, A 최소 거리 mm(손별)과 가장 가까운 관절 이름, B 결과(손별), 이번 프레임 판정 비용(µs, 모든 target 합은 맨 위에).<br>**키(PIE·Link에서 키보드):** `M` 방식 A↔B, `1`/`2`/`3` 부위 모드, `=`/`-` `ContactDistanceMm` ±1 mm, `]`/`[` `DwellSeconds` ±0.05 s, `.`/`,` `ReleaseMarginMm` ±1 mm. 범위 밖으로 가지 않는다(D4).<br>**기록 파일을 만들지 않는다**(계획서 D-07). 숫자는 사용자가 화면에서 읽는다 |
| D8 | **게임 모드** | `AReachProbeGameMode`(기본 Pawn = `AReachHandPawn`). 추적 원점은 플랫폼 기본(UE 5.7 OpenXR = Stage, vBoR CLAUDE.md)을 바꾸지 않는다 |

## 2. 지켜야 할 조건

- target mesh의 slot material·mesh·transform·collision 설정을 코드가 바꾸지 않는다. 바꾸는 것은 overlay material 하나뿐이다. `Touching`이 끝나면 overlay가 `nullptr`로 돌아가 원래 모습과 같다.
- 판정 영역을 보이는 형상보다 키우지 않는다. 여유는 `ContactDistanceMm`(+ Hold의 `ReleaseMarginMm`)뿐이다.
- 레이·원거리 잡기·잡기 없음. 손 모델은 보이기만 한다.
- `Source/ReachStudy/Contact/`의 상태 규칙은 시험대(`Development/`)를 참조하지 않는다. S3가 그대로 쓴다.

## 3. 범위 밖

- `AReachTarget` 부모 클래스·BP 자식(S3), Door 전환·poke 버튼·진행자 창(S4), 정렬(S1), CAD import(S5).
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
| E7 | `Touching` 중 그 손 추적 끊김 1프레임 → 복구 | 끊긴 프레임에 `Idle`. 복구 첫 프레임은 Enter 안 됨, 다음 프레임부터 dwell 다시 시작 |
| E8 | 왼손 `Touching` 중 왼손 떨어지고 오른손 Hold | 유지 |
| E9 | 부위 모드 3종에서 쓰는 관절 수 | 1 / 5 / 26 |
| E10 | `Fingertips`에서 손바닥만 표면에 닿음 | 안 닿음. `WholeHand`면 닿음 |
| E11 | A: convex 상자 밖 5 mm 점 | 5 mm (±0.1). 상자 안 점 = 0 |
| E12 | A: complex만 있는 mesh(simple collision 없음) | `n/a`, Enter 없음 |
| E13 | B: complex만 있는 mesh 표면 밖 5 mm 점, `ContactDistanceMm` 10 | hit |
| E14 | B: 한 프레임에 얇은 표면을 통과(표면 앞 3 cm → 표면 뒤 3 cm, 두 점 모두 반지름 밖) | hit (sweep) |
| E15 | B: 추적 복구 프레임에 3 cm 튐 (D5의 2 cm 한계 밖) | 시작 = 끝으로 계산(튐 경로로 hit 안 됨) |
| E16 | 값 키 범위 끝 | 0 mm·50 mm·0 s·2 s에서 멈춤 |
| E17 | overlay 복원 | `Touching` → `Idle` 뒤 slot material 배열이 시작 때와 같음, overlay `nullptr` |

## 5. 구현자가 정할 것

- 시험에서 관절 위치를 넣는 방법(인터페이스·함수 포인터·테스트 전용 setter 등). 제품 경로에 시험 분기를 넣지 않는다.
- 화면 줄 배치·색 값·가장 가까운 관절 이름 표기.
- 판정 비용 측정 방법(`FPlatformTime::Seconds` 차 등).

## 6. 사용자 에디터 작업 (구현 뒤)

1. 빈 레벨 `Content/Development/S2/L_S2_ContactProbe`, World Settings GameMode Override = `AReachProbeGameMode`.
2. target 넣기 — 각각 Actor로 놓고 `AReachContactProbe`의 `Targets`에:
   - 엔진 Cube(simple 있음), 엔진 Cylinder(곡면), 작은 Cube(2 × 2 × 1 cm, 작은 버튼 크기)
   - 같은 Cylinder 복사본 하나: mesh 에셋 복사 → Collision Complexity `Use Complex Collision As Simple` (CAD처럼 simple 없음 흉내)
   - **회사 PC에서는** CAD target 조각 하나(`Content/CAD/`, git 밖)
3. 앉은 자세로 손이 닿는 높이(바닥 60–90 cm)에, 무릎 옆(Door 아래쪽 흉내) 하나 포함.

## 7. 완료 조건

- E1–E17 자동화 시험 통과. 빌드 non-unity(`-DisableUnity -Module=ReachStudy`)·unity 경고·오류 0. `ReachStudy.*` 전체 한 번.
- `L_S2_ContactProbe`(사용자가 만든 뒤)에서 PIE 로드 시 `AReachContactProbe` 경고 없음.
- 보고 ①~⑤(AGENTS.md). ④에 위 엔진·ISDK 경로:줄을 직접 확인했는지.

## 8. Quest 3에서 볼 것 (T-02, 상태 `not-tested`)

- 방식 A·B 각각, 부위 3종에서 엔진 Cube·Cylinder·작은 Cube·complex만 있는 Cylinder 닿음.
- 손을 표면에 대고 가만히 있을 때 A 거리 값의 흔들림(mm) — `ContactDistanceMm`·`ReleaseMarginMm` 기본값 근거.
- 다른 target을 지나가며 스칠 때 `DwellSeconds` 0.15에서 걸러지는지, 의도한 닿음이 늦게 느껴지는지.
- 무릎 옆 target: 고개를 돌리지 않았을 때 추적 상태(`lost`가 얼마나 뜨나).
- 손을 표면 뒤로 깊이 넣었을 때 A(convex 안 = 0)와 B(빠짐) 차이 → 계획서 D-03 결정.
- 판정 비용(µs) — 회사 CAD 조각에서.

## 개정 이력

- 2026-11-22 — 초안 (Claude).
