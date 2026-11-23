# S4b PC 화면 — Door·IP 이름은 PC에만, 헤드셋에는 어떤 메시지도 없음

| 항목 | 값 |
|---|---|
| 단계 | 계획서 §6 S4b (S4 다음, S3 앞) |
| 결정 근거 | **사용자 2026-11-23:** "vBoR같이 PC 화면에서만 보이는 UI. 현재 Door 이름, 현재 IP 이름. Meta(헤드셋)에서는 어떤 메시지도 보이면 안 된다." IP 이름은 레벨에서 입력하는 글. S2 시험대 숫자 표시는 없앤다("UI 다 빼고 screen message도 안 뜨게"). 계획서 Q-06을 이것으로 바꾼다 |
| 베이스 브랜치 | **S4가 `main`에 머지된 뒤의 `main`** |
| 작업 브랜치 | `codex/S4b-pc-screen` |
| 리뷰 파일 | `Docs/Reviews/S4b-pc-screen.md` |
| 에디터 작업 | **Codex는 하지 않는다.** 구현·리뷰 뒤 Claude가 MCP로 한다(§6) |

## 0. 확인한 사실

- **화면에 바로 올린 위젯(`AddToViewport`)은 헤드셋으로 가고 PC 모니터로는 가지 않는다** — 사용자 다른 프로젝트 AI_Driving의 실측(`C:\Git\AI_Driving\README.md:103-112`). PC에만 보이게 하려면 위젯을 렌더 타깃에 그려 헤드셋의 **스펙테이터 화면**(PC 창)에 눈 영상과 겹쳐 넘긴다. 헤드셋으로 가는 장면에는 들어가지 않는다.
- vBoR이 같은 방법으로 만들었다(vBoR 명세 `Docs/Specs/P5h-spectator-screen.md` D1, 구현 `Source/vBoR/Spectator/VBoRSpectatorSubsystem.cpp`): 스펙테이터 컨트롤러 얻기 `:30-36`, 시작 `:62-81`(위젯 → `FWidgetRenderer` → 렌더 타깃 → 배치 → 텍스처 → 모드), 준비될 때까지 0.5 s마다 30 s까지 다시 시도 `:122-135`, 끝낼 때 모드를 눈 영상으로 되돌림(`Stop` `:259-267`, `SingleEyeCroppedToFill`). **`BeginPlay` 때는 헤드셋·PlayerController가 아직 없어 대개 실패한다**(AI_Driving `README.md:131-134`) — 그래서 다시 시도한다.
- 엔진 API(UE 5.7): `ISpectatorScreenController::SetSpectatorScreenMode`(`Engine/Source/Runtime/HeadMountedDisplay/Public/ISpectatorScreenController.h:34`), `SetSpectatorScreenTexture`(`:42`), `SetSpectatorScreenModeTexturePlusEyeLayout`(`:58`). 모드 `TexturePlusEye`(`HeadMountedDisplayTypes.h:153`), 배치 구조체 `FSpectatorScreenModeTexturePlusEyeLayout`(`:158`, 0–1 비율, `bDrawEyeFirst`·`bUseAlpha`·`bClearBlack`). `FWidgetRenderer::CreateTargetFor`(`Engine/Source/Runtime/UMG/Public/Slate/WidgetRenderer.h:49`), `DrawWidget`(`:53`).
- **화면 메시지 전체 끄기:** 엔진 전역 `GAreScreenMessagesEnabled`(`Engine/Source/Runtime/Core/Public/CoreGlobals.h:426`). 화면 메시지(`AddOnScreenDebugMessage`와 엔진 경고 글)는 이 값이 true일 때만 그려진다(`Engine/Source/Runtime/Engine/Private/UnrealEngine.cpp:13286`). 콘솔 `DisableAllScreenMessages`가 이 값을 false로 바꾼다(`:10308-10312`). 전역이라 에디터의 다음 PIE·에디터 뷰포트에도 남는다 — 끝날 때 되돌린다(D3).
- vBoR P5h의 Quest 확인 기록은 리포 문서에 `not-tested`로 남아 있다. PC에만 보이는 것은 AI_Driving에서 실측됐다.

## 1. 결정

| # | 결정 | 내용 |
|---|---|---|
| D1 | **`AReachPcScreen`** (C++ Actor, `Source/ReachStudy/Screen/`, 레벨에 하나) | `IPName`(FText, 기본 `IP`, 레벨에서 입력 — 사용자). `DoorVariants`(TObjectPtr<`AReachDoorVariants`>, 비면 world의 첫 `AReachDoorVariants`). 글 두 줄: `Door: <활성 Door Name>`(Door 없으면 `Door: -`), `IP: <IPName>`. 머리말(`Door:`·`IP:`)·글 크기·글색·배경색은 영어·프로퍼티. **이 actor가 레벨에 있을 때만 PC 화면 UI를 띄운다.** 둘 이상이면 첫 번째 + `Warning` 한 줄 |
| D2 | **PC 화면 = vBoR P5h 경로** (`UReachPcScreenSubsystem`, `UTickableWorldSubsystem`, `Screen/`, Game·PIE world만) | §0 순서 그대로: C++ Slate 위젯(에셋 없음) → `FWidgetRenderer` → 렌더 타깃 → `TexturePlusEye` 배치 → 텍스처 → 모드. 0.5 s마다 30 s까지 다시 시도, 헤드셋·렌더가 없으면(`-nullrhi`, 헤드셋 미연결) 조용히 아무것도 하지 않는다. **`AddToViewport`를 쓰지 않는다**(§0 — 헤드셋으로 간다).<br>- 배치: 눈 영상 = 창 전체(`EyeRect` 0–1), 글 = 왼쪽 위 띠(예 `TextureRect` x 0–0.3, y 0–0.1, 프로퍼티), `bDrawEyeFirst` true, `bUseAlpha` true(띠 배경 반투명). vBoR의 4:3 눈 영역·창 크기 다시 계산은 하지 않는다(필요해지면 그때).<br>- 다시 그리기는 글이 바뀔 때만(Door 전환 `OnVariantChanged`, 시작 때 한 번). 매 프레임 그리지 않는다.<br>- 끝날 때(world 종료·`Deinitialize`) 스펙테이터 모드를 `SingleEyeCroppedToFill`로 되돌리고 렌더러를 지운다 |
| D3 | **헤드셋에 어떤 메시지도 없다 — 화면 메시지 끄기** | Game·PIE world가 시작하면(`OnWorldBeginPlay`) `GAreScreenMessagesEnabled`의 지금 값을 기억하고 false로 둔다. 그 world가 끝날 때(`Deinitialize`) 기억한 값으로 되돌린다(에디터 뷰포트 메시지는 PIE 뒤 원래대로). **`AReachPcScreen`이 없어도 끈다**(어느 레벨이든 헤드셋에 메시지 없음). 로그 한 줄 `Screen messages disabled (S4b)` |
| D4 | **S2 시험대 화면 글 없앰** | `AReachContactProbe`의 `DrawReadout`·`BuildReadout`과 `GEngine->AddOnScreenDebugMessage`/`RemoveOnScreenDebugMessage`를 지운다. 판정·overlay·키 입력은 그대로(시험대는 색 변화로만 본다). S4 시험의 글 검사(`ReachDoorTests.cpp:221-222`, `:256`)는 결과 값 검사(`bInactive`, `Phase`)로 바꾼다. 헤더의 `door <Name>`도 없어진다 — Door 이름은 PC 화면(D1) |
| D5 | **Door 패널의 안내 글 없앰** (S4 D5·E15 변경) | `DoorVariants`가 비면 패널에 **아무 글도 없다**(지금은 `No door variants`). 경고 로그 한 줄은 그대로. 헤드셋 안의 글은 참가자가 누르는 패널의 `<`·Door 이름·`>`뿐이다 — 이것은 사용자가 정한 버튼 UI다(S4 개정 1), 메시지가 아니다 |
| D6 | **시험할 수 있게 나눈다** | 글 두 줄을 만드는 함수(`BuildLines` — Door·IP 이름 → 문자열)를 헤드셋·렌더러 없이 시험한다. 렌더 타깃·스펙테이터 경로는 Quest에서만 본다(§8) |
| D7 | **모듈 의존** | `ReachStudy.Build.cs`에 `RenderCore`(렌더 타깃, vBoR과 같음). `UMG`·`Slate`·`SlateCore`·`HeadMountedDisplay`는 이미 있다 |

## 2. 지켜야 할 조건

- 헤드셋 안에는 디버그·경고·안내 글이 없다. 글을 보이는 방법은 PC 화면(D2)뿐이다. **새 코드에서 `GEngine->AddOnScreenDebugMessage`·`AddToViewport`·`UKismetSystemLibrary::PrintString` 화면 출력을 쓰지 않는다** (로그 `UE_LOG`는 된다).
- 참가자 헤드셋 화면은 바뀌지 않는다(PC 띠는 헤드셋 장면에 들어가지 않는다).
- 판정·Door 전환 동작은 바뀌지 않는다. 바뀌는 것은 표시뿐이다.
- 제품 코드(`Screen/`)는 `Development/`를 참조하지 않는다.
- 기록 없음(D-07) — 화면 글만.

## 3. 범위 밖

- 정렬 상태·닿은 target·손 추적 표시(전 Q-06 내용) — 사용자가 Door·IP 이름만으로 줄였다(2026-11-23).
- 4:3 눈 영역·TV 비율 맞춤(vBoR D3). PC 창을 TV로 쓸 일이 생기면 그때.
- S2 시험대 숫자를 PC로 옮기기 — 사용자가 없애기로 했다.

## 4. 경계 상황과 시험 (자동화 `ReachStudy.S4b.*`)

| # | 상황 | 기대 |
|---|---|---|
| E1 | `BuildLines`: Door 2개 A 활성, `IPName` `IP A` | `Door: Door A`, `IP: IP A` |
| E2 | `SelectVariant(1)` 뒤 | `Door: Door B` (다시 그리기 요청 한 번) |
| E3 | Door 없음 / `IPName` 비움 | `Door: -` / `IP: ` 뒤 빈 글 그대로(크래시 없음) |
| E4 | Game world 시작 → 끝 | 시작 뒤 `GAreScreenMessagesEnabled` false, 끝난 뒤 시작 전 값. 원래 false였으면 끝난 뒤도 false |
| E5 | Editor world | 값을 바꾸지 않는다(subsystem 없음) |
| E6 | `-nullrhi`(자동화) + `AReachPcScreen` 있음 | 시작 시도 안 함·크래시 없음·경고 없음(vBoR P5h E1과 같음) |
| E7 | `AReachPcScreen` 둘 | 첫 번째 사용, 경고 한 줄 |
| E8 | 시험대 `ProcessFrame` 몇 프레임 | 화면 메시지 없음(`GEngine->OnScreenDebugMessageExists(시험대 키)` false), 판정 결과는 S2와 같음 |
| E9 | Door 패널 `DoorVariants` 없음 | 버튼·글 0개, 경고 한 줄 |

S2·S4 시험은 D4·D5에서 바꾼 글 검사 말고는 그대로 통과해야 한다.

## 5. 구현자가 정할 것

- 띠 위치·크기·글 크기·색 기본값(PC 창에서 읽힐 크기).
- subsystem이 `AReachPcScreen`을 찾는 시점(시작 시도 때 world에서 찾기 권장 — vBoR이 러너를 찾는 방식).

## 6. 에디터 작업 — Claude가 MCP로 (구현·리뷰 뒤)

- `L_S2_ContactProbe`(S4 §6 뒤)에 `AReachPcScreen` 하나(`IPName` = `IP Test`). PIE로 크래시·경고 없음 확인(헤드셋 없는 PIE에서는 PC 띠가 뜨지 않는 것이 정상, D2).

## 7. 완료 조건

- §4 시험 전부, `ReachStudy.*` 전체 한 번 통과. 빌드 non-unity(`-DisableUnity -Module=ReachStudy`)·unity 경고·오류 0.
- 소스 전체에 화면 출력 호출(`AddOnScreenDebugMessage`, `AddToViewport`, `PrintString`)이 없음 — grep 결과를 보고에 적는다.
- 보고 ①~⑤(AGENTS.md). ④에 §0의 엔진·vBoR 경로:줄을 직접 확인했는지.

## 8. Quest 3에서 볼 것 (상태 `not-tested`)

- VR Preview(Link)에서 **PC 창** 왼쪽 위에 `Door: Door A` / `IP: ...`가 보이는지. `Z`/`X`·패널로 바꾸면 PC 글이 바뀌는지.
- **헤드셋 안에는 글이 하나도 없는지** — PC 띠, 엔진 경고(예: 조명 다시 빌드 필요), 시험대 숫자 모두. 보이는 글은 참가자 패널의 `<`·Door 이름·`>`뿐.
- PIE를 끝낸 뒤 에디터 뷰포트 메시지가 원래대로 돌아오는지.

## 개정 이력

- 2026-11-23 — 초안 (Claude). 사용자 요청(PC 화면 Door·IP 이름, 헤드셋 메시지 없음, 시험대 글 없앰). 계획서 Q-06 대체.
