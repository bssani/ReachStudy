# S4b 헤드셋 메시지 없음 — 화면 메시지 끄기, 화면 글 없앰

| 항목 | 값 |
|---|---|
| 단계 | 계획서 §6 S4b (S4 다음, S3 앞) |
| 결정 근거 | **사용자 2026-11-23:** "Meta(헤드셋)에서는 어떤 메시지도 보이면 안 된다", "UI 다 빼고 screen message도 안 뜨게". 이어서 "PC 화면에도 아무것도 안 떠도 된다. touch하면 색이 바뀌는 것까지가 범위" — **PC 화면 UI는 만들지 않는다**. 계획서 Q-06 |
| 베이스 브랜치 | **S4가 `main`에 머지된 뒤의 `main`** |
| 작업 브랜치 | `codex/S4b-headset-clean` |
| 리뷰 파일 | `Docs/Reviews/S4b-headset-clean.md` |
| 에디터 작업 | 없음 |

## 0. 확인한 사실

- **화면 메시지 전체 끄기:** 엔진 전역 `GAreScreenMessagesEnabled`(`Engine/Source/Runtime/Core/Public/CoreGlobals.h:426`). 화면 메시지(`AddOnScreenDebugMessage`와 엔진 경고 글)는 이 값이 true일 때만 그려진다(`Engine/Source/Runtime/Engine/Private/UnrealEngine.cpp:13286`). 콘솔 `DisableAllScreenMessages`가 이 값을 false로 바꾼다(`:10308-10312`). 전역이라 에디터의 다음 PIE·에디터 뷰포트에도 남는다 — 끝날 때 되돌린다(D1).
- **화면에 바로 올린 위젯(`AddToViewport`)은 헤드셋으로 간다** — 사용자 다른 프로젝트 AI_Driving의 실측(`C:\Git\AI_Driving\README.md:103-112`). 그래서 금지한다(§2).
- `UWorldSubsystem::ShouldCreateSubsystem`(`Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h:61`), `OnWorldBeginPlay`(`:75`).

## 1. 결정

| # | 결정 | 내용 |
|---|---|---|
| D1 | **화면 메시지 끄기** (`UReachScreenMessageSubsystem`, `UWorldSubsystem`, Game·PIE world만) | world가 시작하면(`OnWorldBeginPlay`) `GAreScreenMessagesEnabled`의 지금 값을 기억하고 false로 둔다. 그 world가 끝날 때(`Deinitialize`) 기억한 값으로 되돌린다(PIE 뒤 에디터 뷰포트 메시지는 원래대로). 어느 레벨이든 끈다. 로그 한 줄 `Screen messages disabled (S4b)` |
| D2 | **S2 시험대 화면 글 없앰** | `AReachContactProbe`의 `DrawReadout`·`BuildReadout`과 `GEngine->AddOnScreenDebugMessage`/`RemoveOnScreenDebugMessage`를 지운다. 판정·overlay·키 입력은 그대로(시험대는 색 변화로만 본다). S4 시험의 글 검사(`ReachDoorTests.cpp:221-222`, `:256`)는 결과 값 검사(`bInactive`, `Phase`)로 바꾼다 |
| D3 | **Door 패널의 안내 글 없앰** (S4 D5·E15 변경) | `DoorVariants`가 비면 패널에 **아무 글도 없다**(지금은 `No door variants`). 경고 로그 한 줄은 그대로. 헤드셋 안의 글은 참가자가 누르는 패널의 `<`·Door 이름·`>`뿐이다 — 사용자가 정한 버튼 UI다(S4 개정 1), 메시지가 아니다 |

## 2. 지켜야 할 조건

- 헤드셋에도 PC 창에도 디버그·경고·안내 글을 쓰지 않는다. **`GEngine->AddOnScreenDebugMessage`·`AddToViewport`·`UKismetSystemLibrary::PrintString` 화면 출력을 쓰지 않는다** (로그 `UE_LOG`는 된다).
- 판정·Door 전환 동작은 바뀌지 않는다. 바뀌는 것은 글뿐이다.
- 새 모듈 의존을 더하지 않는다.

## 3. 범위 밖

- PC 화면 UI(Door·IP 이름 등) — 사용자가 필요 없다고 했다(2026-11-23). 필요해지면 vBoR P5h 스펙테이터 방식으로 따로 명세한다.
- S2 시험대 숫자를 다른 곳으로 옮기기.

## 4. 경계 상황과 시험 (자동화 `ReachStudy.S4b.*`)

| # | 상황 | 기대 |
|---|---|---|
| E1 | Game world 시작 → 끝 | 시작 뒤 `GAreScreenMessagesEnabled` false, 끝난 뒤 시작 전 값. 원래 false였으면 끝난 뒤도 false |
| E2 | Editor world | 값을 바꾸지 않는다(subsystem 없음) |
| E3 | 시험대 `ProcessFrame` 몇 프레임 | 화면 메시지 없음(`GEngine->OnScreenDebugMessageExists(시험대 키)` false), 판정 결과는 S2와 같음 |
| E4 | Door 패널 `DoorVariants` 없음 | 버튼·글 0개, 경고 한 줄 |

S2·S4 시험은 D2·D3에서 바꾼 글 검사 말고는 그대로 통과해야 한다.

## 5. 구현자가 정할 것

- subsystem 파일 위치(제품 코드 폴더, `Development/` 아님).

## 6. 완료 조건

- §4 시험 전부, `ReachStudy.*` 전체 한 번 통과. 빌드 non-unity(`-DisableUnity -Module=ReachStudy`)·unity 경고·오류 0.
- 소스 전체에 화면 출력 호출(`AddOnScreenDebugMessage`, `AddToViewport`, `PrintString`)이 없음 — grep 결과를 보고에 적는다.
- 보고 ①~⑤(AGENTS.md). ④에 §0의 엔진 경로:줄을 직접 확인했는지.

## 7. Quest 3에서 볼 것 (상태 `not-tested`)

- VR Preview(Link)에서 **헤드셋 안에 글이 하나도 없는지** — 엔진 경고(예: 조명 다시 빌드 필요), 시험대 숫자 모두. 보이는 글은 참가자 패널의 `<`·Door 이름·`>`뿐.
- PIE를 끝낸 뒤 에디터 뷰포트 메시지가 원래대로 돌아오는지.

## 개정 이력

- 2026-11-23 — 초안 (Claude). 사용자 요청(PC 화면 Door·IP 이름, 헤드셋 메시지 없음, 시험대 글 없앰).
- 2026-11-23 — 개정 1 (사용자): **PC 화면 UI 뺌** — "touch하면 색이 바뀌는 것까지가 범위". `AReachPcScreen`·스펙테이터 경로·`RenderCore` 없앰. 파일 이름 `S4b-pc-screen.md` → `S4b-headset-clean.md`.
- 2026-11-24 — D1 예외 (사용자, [S1](S1-calibration-import.md) D5): GameMode가 `AReachCalibrationGameMode`인 정렬 레벨에서는 화면 메시지를 끄지 않는다. 그 밖의 world는 그대로.
