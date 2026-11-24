# S1 정렬 가져오기 — VRCalibrationKit을 넣고 실물 좌석에 맞춘다

| 항목 | 값 |
|---|---|
| 단계 | 계획서 §6 S1. 시험 T-01 |
| 결정 근거 | 계획서 D-01(마커 + 앵커), Q-01(플러그인을 먼저 뺀 뒤 쓴다 → **풀림**: VRCalibrationKit 완성). **사용자 2026-11-24:** ① 정렬 레벨에서만 화면 글을 보이게 한다 ② 확인점은 만들지 않는다 — 정렬 로그의 `Max XY`·`Marker fit vs pawn`으로 오차를 본다 ③ 시험용 정렬 점은 키트 기기 시험 때의 배치를 그대로 쓴다 |
| 키트 출처 | `C:\Git\VRCalibrationKit`(GitHub `bssani/VRCalibrationKit`, public) `main@e915647`. 플러그인 폴더는 `864946c` 뒤로 바뀌지 않았다(그 뒤 커밋은 문서뿐, `git diff --stat 864946c e915647` = `Docs/Integration-Guide.md`·`README.md`만) |
| 붙이는 법의 기준 | 키트 `Docs/Integration-Guide.md` §4–§10 |
| 베이스 브랜치 | `main@1b955db` 이후 이 명세가 들어간 `main` |
| 작업 브랜치 | `codex/S1-calibration-import` |
| 리뷰 파일 | `Docs/Reviews/S1-calibration-import.md` |
| 에디터 작업 | **Codex는 하지 않는다.** 머지 뒤 Claude가 MCP로 한다(§6) |

## 0. 확인한 사실

### 키트

- **플러그인 구성:** `Plugins/VRCalibrationKit/VRCalibrationKit.uplugin` — 모듈 `VRCalibrationCore`(모든 플랫폼), `VRCalibrationMarker`(Win64만). 이 `.uplugin`이 `OculusXR`·`XRBase`·`OpenCV`(Win64)를 켠다. Interaction SDK는 필요 없다(가이드 §2).
- **의존 모듈:** Core = `Json`·`HeadMountedDisplay`·`XRBase`·`OculusXRAnchors`(`VRCalibrationCore.Build.cs:9-10`). Marker = `MRUtilityKit`·`OculusXRAnchors`·`OpenCVHelper`·`OpenCV`·`RHI`·`RenderCore`(`VRCalibrationMarker.Build.cs:9-10`). `MRUtilityKit`·`OculusXRAnchors`는 이 리포의 Meta XR 1.205.0 안에 있다(`Plugins/MetaXR/OculusXR.uplugin:51`, `:122`).
- **정렬 component가 Pawn을 옮기는 조건** (`Source/VRCalibrationCore/Private/VRCalAlignmentComponent.cpp`):
  - BeginPlay에서 player 0의 Pawn을 잡는다(`:20-26`).
  - 차 actor root scale이 1이 아니면 `NeedsCheck`(`:92-96`). 추적 원점이 Stage가 아니면 `Stage tracking origin required`(`:105-110`).
  - Pawn root·차 root가 있고 `bUseControllerRotationYaw` false이고 `WorldToMeters` 100일 때만 옮긴다(`:122-124`).
  - 옮길 때 Pawn을 **차 actor에 attach**하고(`:129`) 상대 위치를 정한다(`:131`). 매 프레임 `TG_PostUpdateWork`에서 돈다(`:17`).
- **정렬 도구** `AVRCalMarkerCalibrator` (`Source/VRCalibrationMarker/Private/VRCalMarkerCalibrator.cpp`):
  - 키 `M`·`G`·`H`·`S`를 player 0 입력에 묶는다(`:102-105`).
  - MRUK World Lock을 끄고(`:77-78`) 끝나면 되돌린다(`:123`).
  - 미리보기(`G`)는 Pawn을 차에 붙이고(`:577`) `H`는 원래 부모·위치로 되돌린다(`:600-602`).
  - 결과 글: `Max XY … cm`(`:508`), `Marker fit vs pawn … dX … dY …`(`:510`).
  - 글은 `Publish`가 로그(`:643`)와 `GEngine->AddOnScreenDebugMessage`(`:646`) 둘 다로 보낸다. **화면 메시지가 꺼져 있으면 화면에는 안 보인다.**
  - 마커 구 표시는 `DrawDebugSphere`라 화면 메시지 설정과 상관없이 보인다.
- **정렬 점 표시:** `UVRCalAlignPointComponent`의 구·번호 글은 game에서 숨겨져 있다(`VRCalAlignPointComponent.cpp:20-21`). 정렬 레벨의 미리보기 때만 보인다(`VRCalMarkerCalibrator.cpp:585`, `:612`). 본 레벨 헤드셋에는 안 보인다.
- **앵커:** 콘솔 변수 `VRCal.AnchorAlignment`, 기본 0(`VRCalAnchorAlignment.cpp:18`). 로그:
  - 저장: `anchor saved … created vs requested …`(`:107`)
  - 복원: `anchor found … Stage drift since save …`(`:219`)
  - 실패: `anchor not found …; using saved Stage points`(`:226`)
- **키트 시험:** 이름 `VRCalibrationKit.*`(Anchors·Core·K1·Marker·Profiles). fixture 경로는 `FPaths::ProjectPluginsDir()/VRCalibrationKit/Tests/Fixtures`(`KitBoundaryTests.cpp:26`)라 이 리포 `Plugins/VRCalibrationKit/`에 그대로 두면 찾는다. 가이드 §7: **`Config/UserEngine.ini`가 있으면 시험 기본값을 덮으므로 시험 전에 치운다.**
- **키트 샘플의 Stage 설정:** `Source/VRCalibrationKit/VRCalSampleGameMode.cpp:10-16` — `StartPlay`에서 `WorldToMeters = 100`, `SetTrackingOrigin(Stage)`를 하고 나서 `Super::StartPlay()`.
- **샘플 차의 정렬 점 기본값:**
  - 위치: `(4,-60,50)`, `(4,-14,50)`, `(-44,-14,50)`, `(-44,-60,50)` cm (`Source/VRCalibrationKit/VRCalSampleCar.cpp:32-40`).
  - `FloorLocalZCm` 기본 0.
  - BP_SampleCar·레벨 인스턴스에서 바꿨는지는 §6에서 Claude가 읽는다.
- **기기 결과**(Quest 3 + Link, 2026-11-24, 키트 가이드 §1·§7):
  - Link 재연결로 원점이 137 cm·14° 옮겨진 뒤, 앵커로 복원하고 마커로 다시 재면 0.6 cm 안이었다(dX +0.61, dY −0.20 cm).
  - 가디언을 다시 그리면 약 6 cm 어긋난다.
  - 재부팅 뒤·다음 날은 아직 확인하지 않았다.

### 엔진

- `UWorld::BeginPlay`의 순서(`Engine/Source/Runtime/Engine/Private/World.cpp`):
  - world subsystem `OnWorldBeginPlay`(`:6058`)가 먼저다.
  - 그다음 `GameMode->StartPlay()`(`:6065`)다.
  - GameMode는 그 전에 `SetGameMode`(`:5823`)로 만들어진다. 그래서 subsystem의 `OnWorldBeginPlay`에서 `GetAuthGameMode()`를 볼 수 있다.
- `AGameModeBase::StartPlay` = `GameState->HandleBeginPlay()`(`Engine/Source/Runtime/Engine/Private/GameModeBase.cpp:204-207`). actor BeginPlay가 여기서 시작된다. 그래서 Stage·`WorldToMeters`는 `Super::StartPlay()` **전에** 둔다(가이드 §5).
- `UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin`(`Engine/Plugins/Runtime/XRBase/Source/XRBase/Public/HeadMountedDisplayFunctionLibrary.h:192`). XR 장치가 없으면(`-nullrhi` 시험) 아무 일도 하지 않는다.
- `APawn` 기본 `bUseControllerRotationYaw = false`(`Engine/Source/Runtime/Engine/Private/Pawn.cpp:97`).
- **손 관절 world 좌표는 Pawn을 따라간다:**
  - S3 판정의 손 읽기는 `GetHandTrackingState(..., UnrealWorldSpace, ...)`다(`Source/ReachStudy/Contact/ReachContactQuery.cpp:50-55`).
  - XR의 tracking→world 변환은 view target의 활성 camera component의 부모(= Pawn root)다(`Engine/Plugins/Runtime/XRBase/Source/XRBase/Private/XRTrackingSystemBase.cpp:262-300`). 매 프레임 갱신된다(`Engine/Plugins/Runtime/OpenXR/Source/OpenXRHMD/Private/OpenXRHMD.cpp:3919`).
  - 그래서 키트가 Pawn root를 옮기면 손 관절·닿음 판정이 같이 옮겨진다. **판정 코드는 고칠 것이 없다.**

### 이 리포

- **Pawn** `AReachHandPawn`(`APawn`):
  - root `VROrigin`이 있다(`Source/ReachStudy/Hand/ReachHandPawn.cpp:47`).
  - Camera `bLockToHmd = true`이다(`:50`).
  - `bUseControllerRotationYaw`는 건드리지 않는다(엔진 기본 false).
  - **키트 조건에 맞다.**
- **Stage·`WorldToMeters`를 정하는 코드가 없다.**
  - `AReachGameMode`는 `DefaultPawnClass`만 정한다(`Source/ReachStudy/ReachGameMode.cpp:4-7`).
  - grep `SetTrackingOrigin|WorldToMeters` → `Source/`에 0건.
  - 키트는 Stage가 아니면 Pawn을 옮기지 않는다.
- **Pawn 위치를 직접 정하는 코드가 없다.**
  - grep `SetActorLocation|SetActorTransform|TeleportTo|AttachToActor` → `Source/`에 0건.
  - Pawn은 레벨의 `SeatStart`(PlayerStart)에서 생기고, 키트가 `Applied`일 때 그 위치를 덮는다. 덮어쓰기 순서를 정할 코드가 없다. 앞으로도 만들지 않는다(§2).
- **키:**
  - Door 전환 `Z`·`X`·`C`·`V`·`N`·`B`(`Source/ReachStudy/Door/ReachDoorVariants.cpp:59`) — 키트 `M`·`G`·`H`·`S`와 **겹치지 않는다**.
  - S2 시험대가 **`M`을 쓴다**(`Source/ReachStudy/Development/ReachContactProbe.cpp:42-44`, `:121`). 그래서 시험대를 정렬 레벨에 두지 않는다(§6).
- **화면 메시지:** S4b subsystem이 모든 Game·PIE world에서 화면 메시지를 끈다(`Source/ReachStudy/Screen/ReachScreenMessageSubsystem.cpp:15-22`). 그래서 정렬 도구 글이 지금은 안 보인다.
- **설정:**
  - `Config/DefaultEngine.ini`의 `[/Script/OculusXRHMD.OculusXRHMDRuntimeSettings]`에 `XrApi=NativeOpenXR`·`HandTrackingSupport`가 있다. `bAnchorSupportEnabled`·`bPassthroughCameraAccessEnabled`가 없다.
  - `ReachStudy.uproject`에 `XRBase`·`OpenCV`가 있다. `VRCalibrationKit`·`OpenXR`가 없다.
- **git 무시:** `.gitignore`가 `Plugins/**/Binaries/*`, `Plugins/**/Intermediate/*`, `Saved/*`(정렬 저장 `Saved/Calibration/`), `Config/User*.ini`(앵커 켜기)를 이미 무시한다.

## 1. 결정

| # | 결정 | 내용 |
|---|---|---|
| D1 | **키트 플러그인 복사** | 리포 루트에서 `git -C C:/Git/VRCalibrationKit archive main Plugins/VRCalibrationKit`를 풀어 넣는다(`Plugins/VRCalibrationKit/`). **커밋한다** — Meta 코드가 없는 사용자의 public 키트다. 커밋 메시지에 출처 커밋 `e915647`을 적는다.<br>- **키트 파일은 한 글자도 고치지 않는다.** 빌드·시험이 키트 쪽 문제로 막히면 멈추고 리뷰 파일 "질문"에 적는다. 고치는 것은 키트 리포의 일이다 |
| D2 | **프로젝트 설정** | `ReachStudy.uproject` Plugins에 `VRCalibrationKit`·`OpenXR`를 켠다(가이드 §4-2). `DefaultEngine.ini`의 위 절에 `bAnchorSupportEnabled=True`·`bPassthroughCameraAccessEnabled=True`를 더한다(가이드 §4-3). 다른 줄은 그대로 |
| D3 | **`AReachGameMode::StartPlay`** | `Super::StartPlay()` 전에 `WorldSettings->WorldToMeters = 100`, `SetTrackingOrigin(EHMDTrackingOrigin::Stage)`. 로그 한 줄 `Tracking origin Stage, WorldToMeters 100 (S1)`. 키트 샘플(`VRCalSampleGameMode.cpp:10-16`)과 같다 |
| D4 | **`AReachCalibrationGameMode`** (`AReachGameMode` 자식, 제품 코드) | 정렬 레벨 전용. **동작은 부모와 같다**(Pawn·Stage). 다른 점은 D5의 화면 메시지 예외의 표시뿐이다 |
| D5 | **정렬 레벨에서만 화면 글을 보인다** (사용자 2026-11-24, S4b D1 예외) | `UReachScreenMessageSubsystem::OnWorldBeginPlay`에서 그 world의 `GetAuthGameMode()`가 `AReachCalibrationGameMode`면 `GAreScreenMessagesEnabled`를 **바꾸지 않는다**(끄지도, 켜지도 않는다). 로그 한 줄 `Screen messages kept (calibration level, S1)`. 그 world가 끝날 때도 되돌릴 것이 없다. 그 밖의 world는 S4b와 똑같다 |
| D6 | **Pawn은 그대로** | `AReachHandPawn`은 키트 조건에 맞다(§0). 코드 변경 없음 |
| D7 | **시험용 모듈 의존** | §4 E4를 위해 `ReachStudy.Build.cs`의 **private** 의존에 `VRCalibrationCore`만 더한다(모든 플랫폼). `VRCalibrationMarker`는 더하지 않는다. 제품 코드는 키트 API를 부르지 않는다 — 정렬은 레벨에 놓은 키트 component가 한다 |
| D8 | **앵커는 PC마다 켠다** | `Config/UserEngine.ini`(git 무시)의 `[ConsoleVariables] VRCal.AnchorAlignment=1`. 사용자가 시험 PC에서 한다(가이드 §7). Codex는 이 파일을 만들지 않는다 |

## 2. 지켜야 할 조건

- **키트 파일 수정 금지**(D1). `Plugins/VRCalibrationKit/`는 키트 `main@e915647` archive와 같아야 한다.
- **ReachStudy 코드는 Pawn을 옮기거나 붙이지 않는다**(`SetActorLocation`·`SetActorTransform`·`AttachToActor`·`TeleportTo` 등). Pawn 위치는 키트 하나만 정한다.
- 닿음 판정·Door 전환·S4b 동작은 바뀌지 않는다. 예외는 D5(정렬 GameMode인 world만) 하나다. ReachStudy 코드는 여전히 화면에 글을 쓰지 않는다(AGENTS.md).
- 카메라(정렬 도구 `AVRCalMarkerCalibrator`)는 정렬 레벨에만 둔다(가이드 §6).
- `Saved/Calibration/`·`Config/UserEngine.ini`·Meta 플러그인은 커밋하지 않는다.
- 명세에 없는 것을 더하지 않는다 — 다시 적용 키(키트 샘플의 `A`), 진행자 UI, Buck profile(`Bucks`), 상태 표시.
- `AReachProbeGameMode`(S2 시험대)는 바꾸지 않는다.

## 3. 범위 밖

- **확인점(손끝으로 정렬 오차 재기)** — 키트에 없다. 사용자 결정(2026-11-24)으로 만들지 않는다. 오차는 정렬 레벨 로그로 본다(§8, 계획서 §4).
- 실제 Buck의 정렬 점 좌표 — CAD에서 나오므로 public 리포에 넣지 않는다. S5에서 `Content/CAD/` 안의 차 BP에 둔다.
- Buck profile 여러 개, S2 시험대 레벨 정렬, Android.
- 재부팅 뒤·다음 날 복원 — 키트도 아직 확인하지 않았다. §8에서 할 수 있으면 같이 본다.

## 4. 경계 상황과 시험 (자동화 `ReachStudy.S1.*`)

| # | 상황 | 기대 |
|---|---|---|
| E1 | `AReachGameMode` world, WorldSettings `WorldToMeters`를 100이 아닌 값(예: 50)으로 두고 시작 | 시험 actor의 BeginPlay가 보는 `WorldToMeters` = 100 (`Super::StartPlay` 전에 정함) |
| E2 | `AReachCalibrationGameMode` | `AReachGameMode`의 자식, `DefaultPawnClass` = `AReachHandPawn`, E1과 같은 결과 |
| E3 | 화면 메시지 — 정렬 GameMode world, 시작 전 true / false 각각 | 시작 뒤에도, 끝난 뒤에도 시작 전 값 그대로 |
| E4 | 화면 메시지 — `AReachGameMode` world, GameMode 없는 world | S4b E1과 같음(시작 뒤 false, 끝난 뒤 시작 전 값) |
| E5 | 키트 + 이 리포 Pawn — 차 시험 actor(root scale 1, `UVRCalAlignmentComponent` 하나, `UVRCalAlignPointComponent` 4개 `Order` 1–4, 20 cm 이상 벌어짐), `AReachHandPawn` 하나. 점이 맞는 저장값으로 `InitializeAlignment(Pawn, &Saved, Stage, PlayArea)` | `State` = `Applied`. Pawn의 attach 부모 actor = 차. Pawn 상대 위치 = 맞춘 값(허용 0.1 cm) |
| E6 | E5와 같고 원점이 Stage가 아님 | `NeedsCheck`, Pawn이 붙지 않고 위치도 그대로 |
| E7 | 키트 시험 `VRCalibrationKit.*` 전체를 이 프로젝트에서 (`Config/UserEngine.ini` 없이) | 전부 통과 |
| E8 | 기존 `ReachStudy.*`(S2·S3·S4·S4b) | 고치지 않고 전부 통과 |

## 5. 구현자가 정할 것

- `AReachCalibrationGameMode` 파일 위치(제품 코드 폴더, `Development/` 아님).
- E1–E4의 world 만드는 방법(GameMode를 world에 넣는 방법), E5의 차 점·저장값 숫자. `InitializeAlignment`는 키트 public API(`VRCalAlignmentComponent.h:43`)다. 키트 private 시험 헬퍼(`Private/Tests/`)는 쓰지 않는다.
- 로그 카테고리 이름.

## 6. 에디터 작업 (머지 뒤 Claude가 MCP로 — Codex는 하지 않음)

1. **키트 기기 시험 배치 읽기(읽기만):**
   - 키트 프로젝트의 `BP_SampleCar` 정렬 점 4개 위치·`FloorLocalZCm`을 읽는다.
   - `L_Calibrate`·`L_Sample`의 차 인스턴스에 바꾼 값이 있는지 본다.
   - `L_Calibrate` 정렬 도구의 `MarkerArrowLocal` 등 바꾼 값을 읽는다.
   - 키트 리포는 고치지 않는다. 읽지 못하면 사용자에게 묻는다.
2. **`Content/Development/S1/BP_S1Car`** (Actor):
   - root scale 1. `UVRCalAlignmentComponent` 하나, `UVRCalAlignPointComponent` 4개(`Order` 1–4, 1에서 읽은 위치), `FloorLocalZCm`.
   - 레벨에서 **world 원점**(두 Door·IP와 같은 차 원점, D-09)에 둔다.
   - `DoorVariants` 목록에 넣지 않는다.
3. **`Content/Development/S1/L_S1_Calibrate`:**
   - 바닥·조명, `BP_S1Car`, `AVRCalMarkerCalibrator` 하나(1에서 읽은 값).
   - GameMode override `AReachCalibrationGameMode`.
   - S2 시험대(`M` 키)는 두지 않는다.
4. **`L_S3_Touch`:** `BP_S1Car`를 원점에 더한다. 레벨에 정렬 component는 이것 하나다(가이드 §5). 나머지는 그대로.
5. PIE로 본다:
   - `L_S1_Calibrate`: 키 안내 글이 화면에 보이고, Stage 로그가 나온다. Link가 없으면 카메라 시작 실패 글이 정상이다.
   - `L_S3_Touch`: 글이 없다. 저장값이 없으면 `NoSavedValue` 로그가 나오고 Pawn은 `SeatStart`에 있다.

## 7. 완료 조건

- §4 E1–E8 전부. `ReachStudy.*` 전체 한 번, `VRCalibrationKit.*` 전체 한 번 통과(로그 파일 이름을 보고에 적는다).
- 빌드 non-unity(`-DisableUnity -Module=ReachStudy`)·unity 경고·오류 0. 키트 플러그인 컴파일에서 경고가 나오면 고치지 말고 보고한다(D1).
- `Plugins/VRCalibrationKit/`가 키트 archive와 같다는 확인(비교 명령과 결과를 보고에 적는다).
- `Source/`에 화면 출력 호출(`AddOnScreenDebugMessage`, `AddToViewport`, `PrintString`)이 없음 — grep 결과(키트 플러그인 폴더는 대상 밖).
- 보고 ①~⑤(AGENTS.md). ④에 §0의 키트·엔진 경로:줄을 직접 확인했는지.

## 8. Quest 3에서 볼 것 (T-01, 상태 `not-tested`) — 키트 가이드 §10 순서

**준비**
- Link 앱 설정:
  - Beta → Spatial Data over Meta Horizon Link 켬
  - Passthrough over Link 켬
  - 카메라 API 권한 허용
- 시험 PC `Config/UserEngine.ini`에 `VRCal.AnchorAlignment=1`.
- 마커 1–4를 100%로 인쇄한다(검은 사각형 60 mm). 키트 기기 시험 때 자리에 붙인다(§6-1의 값).

**순서**
1. **정렬:** `L_S1_Calibrate` VR Preview → `M` → 네 마커가 모두 초록 → `S`.
   - 로그 `Max XY … cm`가 2 cm 아래인지 본다(점별 오차, 값을 적는다).
   - `anchor saved … created vs requested`를 기다린다.
   - 헤드셋에 키트 글이 보이는지 본다(D5).
2. **복원:** `L_S3_Touch` Play.
   - 로그 `anchor found … Stage drift since save …`의 drift가 0 근처인지(헤드셋이 잠들지 않았으면), `Alignment` 상태가 `Applied`인지 본다.
   - **헤드셋에 글이 없는지**(S4b).
   - 가상 손이 실제 손 위치에 있고 큐브를 만지면 색이 바뀌는지 본다.
3. **재연결:** Link를 끊었다 다시 연결 → `L_S1_Calibrate`에서 **`M`만** 누른다(`S`를 누르면 시험 중인 앵커를 바꾼다).
   - 로그 `Marker fit vs pawn` dX·dY가 2 cm 안인지 본다(값을 적는다).
4. (T-01 앵커 끔) `UserEngine.ini`를 치우고 1–2를 다시 하면 재연결 전에는 같은 자리, 재연결 뒤에는 어긋나는 것이 정상이다.
5. PIE를 끝낸 뒤 에디터 뷰포트의 화면 메시지가 원래대로인지 본다.

**운영 규칙**(가이드 §7)
- 가디언을 다시 그렸거나 Buck·마커를 옮겼으면 다시 정렬한다(M → S).
- Play 재시작·Link 재연결·헤드셋 잠듦은 그대로 둔다(앵커가 복원).

**기록**
- 1의 `Max XY`와 3의 dX·dY를 Buck마다 적는다. 이것으로 `ContactDistanceMm`을 정한다(계획서 §4).
- 시험용 배치 값만 이 리포에 적는다. 실제 Buck 값은 public 리포에 넣지 않는다.

## 개정 이력

- 2026-11-24 — 초안 (Claude). 키트 `main@e915647` 기준. 사용자 답: 정렬 레벨만 화면 글, 확인점 없음, 키트 기기 시험 배치 재사용.
