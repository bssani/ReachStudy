# S4 Door 전환 — 구현 보고·리뷰

## 질문

없음. Claude 코드 리뷰 대기.

## Codex 구현 결과 — 2026-11-23

베이스 `main@cf6e28a`, 작업 브랜치 `codex/S4-door-switch`. 아래는 구현·검증 보고이며 Claude 리뷰 결과가 아니다.

### ① 바뀐 파일

- `Source/ReachStudy/Door/ReachDoorVariants.h`, `.cpp`: variant별 actor 목록·재귀 부착 actor 수집, 시작 숨김/collision 보관·복원, 절대 번호 선택, 변경 이벤트, `Z/X/C/V` 입력. 공유 actor는 항상 원래 값, 목록 밖 actor는 그대로다.
- `Source/ReachStudy/Door/ReachDoorPanel.h`, `.cpp`: ISDK poke 전용 3D 위젯 패널, 기본 420 × 140·scale 0.06. 지정된 Door manager를 위젯에 넘기며 미지정 경고는 BeginPlay 한 번이다.
- `Source/ReachStudy/Door/ReachDoorPanelWidget.h`, `.cpp`: C++ 위젯 트리, variant별 이름·버튼, 공통 `SelectVariant`, 활성 색 이벤트 갱신. 버튼별 번호를 보관하는 작은 `UButton` 자식도 같은 파일에 있다.
- `Source/ReachStudy/Hand/ReachHandPawn.cpp`: poke만 허용, 좌우 rig 생성자에서 poke limiter 등록. ray·grab 비활성, 원본 관절 읽기는 그대로다.
- `Source/ReachStudy/Development/ReachContactProbe.h`, `.cpp`: manager 지정/자동 검색, 전환 즉시 모든 접촉·overlay·손 이력 초기화, `door <Name>` 헤더, 비활성 target 판정 생략·`inactive` 표시. 실제 화면 출력도 사용하는 `BuildReadout`으로 표시 문자열을 검증한다.
- `Source/ReachStudy/ReachStudy.Build.cs`: `UMG`, `Slate`, `SlateCore` 의존 추가.
- `Source/ReachStudy/Tests/ReachDoorTests.cpp`, `ReachDoorTestObserver.h`: 실제 world·collision·CreateWidget·OnClicked를 사용하는 S4 시험 7개와 동적 이벤트 횟수 관찰 객체.
- 이 보고 파일. git 무시 `HANDOFF.md`의 Codex 결과도 갱신한다.

`Contact/`, S2 시험, Config, 레벨·에셋, 플러그인은 변경하지 않았다. 제품 코드에서 `Development/` 참조 없음.

### ② 명세 완료 조건별 결과

- **§4 E1–E17: 자동화 충족.** 최종 `ReachStudy.*` 전체 실행에서 발견 16·완료 16·성공 16·실패 0, `Automation Test Queue Empty 16 tests performed`, 프로세스 종료 코드 0. S2 9개와 S4 7개다. 최종 시험 구간의 Warning/Error는 0이다.

| 시험 | 검증한 명세 항목 |
|---|---|
| `ReachStudy.S4.E1_E4_Selection` | E1 초기 A/B 각 2 actor·IP·이벤트 없음, E2 전환·이벤트 번호/횟수, E3 동일 번호 무시, E4 잘못된 번호 무시·경고 |
| `ReachStudy.S4.E5_E7_E17_MembershipAndRestore` | E5 자식·손자 포함 켬/끔, E6 공유 actor, E7 원래 숨김/collision과 EndPlay 복원, E17 null·전/후 삭제 actor. component material·mesh·transform·collision 설정·response 보존도 확인 |
| `ReachStudy.S4.E8_E10_ProbeResetAndInactive` | E8 비활성 actor의 실제 collision 질의 없음, E9 전환 즉시 Door·IP overlay/결과 초기화와 첫 추적 프레임 Idle, E10 inactive 상태·문구·overlay 없음. 자동 manager 검색·헤더·동일 선택 시 접촉 유지도 확인 |
| `ReachStudy.S4.E11_NoDoorProbe` | E11 manager 없는 world에서 S2 접촉/overlay·door 헤더 없음 |
| `ReachStudy.S4.E12_E14_KeyboardAndWidget` | E12 Z/X/C/V 존재 여부별 선택, E13 키와 버튼의 마지막 호출 우선·실제 변경 횟수만 이벤트, E14 버튼 번호·이름·색과 키 변경 색 갱신 |
| `ReachStudy.S4.E15_MissingPanelConnection` | E15 버튼 없는 안내·경고 한 번, 실제 패널→위젯 연결, 초기 번호 1·manager보다 먼저 만든 패널의 초기 색·버튼 선택, 위젯 ray 금지 |
| `ReachStudy.S4.E16_HandPokeOnly` | E16 좌우 poke 자동 활성·ray/grab 금지·grab detector 세 종류 차단·limiter 등록 |

- **빌드 성공 조건: 충족.** 최종 non-unity `-DisableUnity -Module=ReachStudy`와 실제 unity `Module.ReachStudy.cpp` 컴파일 모두 `Result: Succeeded`, 종료 코드 0. C++/UHT 컴파일 경고·오류 0.
- **빌드 전체 로그의 무경고 조건: 환경 항목 남음.** 두 빌드 모두 git 밖 MCP 도구의 `StructUtils` 폐기 예정 경고 2줄이 있다. 전체 로그까지 경고 0이라고 보지 않는다. S4 범위 밖 도구·프로젝트 의존은 수정하지 않았다.
- **보고 ①–⑤:** 이 파일·마지막 구현 커밋 본문·HANDOFF에 기록.
- **§6 레벨 배치·PIE 경고 없음:** 미확인. 명세대로 Codex는 에디터 MCP·레벨·에셋 작업을 하지 않았다. Claude 리뷰 뒤 수행한다.
- **Quest 3 T-02·T-03:** `not-tested`. 합성 자동화는 실제 poke·손 모델·원본 관절·reach 검증이 아니다.

### ③ 명세와 다르게 한 것과 이유

- 제품 동작·범위 변경 없음. 버튼은 가로 한 줄, 글 크기 기본 24, 활성 초록/비활성 회색으로 §5 재량을 적용했다. 화면 문구·색·크기는 프로퍼티다. DrawSize/WidgetScale은 ISDK 컴포넌트의 기존 편집 프로퍼티를 사용한다.
- `-ForceUnity` 인자만 사용한 빌드는 ReachStudy unity 컴파일 증거가 없었다. 기존 S2 절차대로 git 무시 `Saved/UnrealBuildTool/BuildConfiguration.xml`에 `bForceUnityBuild=true`, `bUseAdaptiveUnityBuild=false`를 잠시 넣고 실제 `Module.ReachStudy.cpp` 컴파일을 확인했다. XML은 원래 바이트로 복원했다.
- 처음 non-unity 빌드에서 `Slot` 이름 충돌과 protected `RigModifiersToSpawn` 접근 오류를 수정했다. limiter는 파생 rig 생성자로 옮겼고 시험은 reflection으로 등록 목록을 확인한다. 제품에 시험 전용 getter를 추가하지 않았다.
- 전체 자동화 초기 시도 2회는 E15 시험 fixture 종료에서 ISDK 이벤트 구독 잔류로 중단됐다. 엔진 `RouteEndPlay`는 actor 초기화가 끝나야 종료를 전달한다. fixture에 `InitializeActorsForPlay`, world context, 대응 `RouteEndPlay`를 갖춘 뒤 E15 집중 시험 1개 성공·종료 0을 확인했다. 최종 소스로 non-unity·unity를 다시 빌드하고 전체 16개를 끝까지 실행해 위 결과를 얻었다. 최초 한 번에 통과한 것으로 기록하지 않는다.
- 자동화 시작에는 기존 ISDK 샘플 `/OculusInteractionSamples/Widgets/InfoCardWidget` 누락 CDO 오류 2건, 헤드셋 미연결·OpenXR·Meta 에셋 버전 경고가 있다. 최종 시험 구간은 경고·오류 없이 성공했다. 샘플 플러그인 추가·원본 플러그인 수정 없음.

로그(git 밖): `Saved/Logs/S4-NonUnity.log`, `S4-Unity.log`(인자만 사용한 빌드), `S4-Unity-Actual.log`, `S4-Automation.log`, `S4-Panel-Focused.log`. 초기 실패: `S4-NonUnity-initial.log`, `S4-Automation-initial-crash.log`, `S4-Automation-teardown-crash.log`.

### ④ 사용 API — 로컬 헤더·구현 직접 확인

엔진 기준 경로: `C:/Program Files/Epic Games/UE_5.7/Engine/`. 아래는 추정이 아니라 로컬 파일 확인이다.

- actor 상태·부착 목록: `Source/Runtime/Engine/Classes/GameFramework/Actor.h:1925` (`SetActorHiddenInGame`), `:1929` (`SetActorEnableCollision`), `:1933` (`GetActorEnableCollision`), `:3558` (`GetAttachedActors`, 재귀 인자), `:4521` (`IsHidden`). component collision 소유자 조건은 `Source/Runtime/Engine/Private/PrimitiveComponentPhysics.cpp:1564`에서 확인.
- ISDK 위젯: `Plugins/MetaXRInteraction/Source/OculusInteractionPrebuilts/Public/IsdkInteractableWidgetComponent.h:57` (`GetWidget`), `:209` (poke), `:230` (ray), `:237` (DrawSize), `:244` (WidgetScale), `:251` (WidgetClass). 인스턴스 생성·할당 시점은 같은 모듈 `Private/IsdkInteractableWidgetComponent.cpp:175`, `:386`, `:404`, `:424`에서 확인.
- UMG: `Source/Runtime/UMG/Public/Blueprint/UserWidget.h:1568` (`RebuildWidget`), `:1516` (WidgetTree), `Source/Runtime/UMG/Public/Blueprint/WidgetTree.h:102` (`ConstructWidget`), `:142` (RootWidget), `Source/Runtime/UMG/Public/Components/Button.h:76` (`OnClicked`), `:129` (`SetBackgroundColor`), `:131` (색 getter), `Source/Runtime/UMG/Public/Components/WidgetComponent.h:180` (`GetUserWidgetObject`). world만 가진 CreateWidget 경로도 `Source/Runtime/UMG/Private/UserWidget.cpp:2737`에서 확인.
- 손 rig: `Plugins/MetaXRInteraction/Source/OculusInteractionPrebuilts/Public/Rig/IsdkRigComponent.h:200` (ray), `:207` (poke), `:214` (grab), `:193` (grabber), `:381` (protected modifier 목록). `Public/Rig/IsdkPokeLimiterRigModifier.h:43` (limiter). `Private/Rig/IsdkPokeInteractionRigComponent.cpp:38`의 poke 기본 자동 활성, `Private/Rig/IsdkRigComponent.cpp:84`의 modifier 생성도 확인. vBoR 파생 rig 생성자 방식과 대조했다.
- grab 차단: `Plugins/MetaXRInteraction/Source/OculusInteraction/Public/Interaction/IsdkGrabberComponent.h:192`, `:200` (허용 여부 읽기·설정).
- fixture 수명: `Source/Runtime/Engine/Classes/Engine/World.h:3842` (`InitializeActorsForPlay`), `Source/Runtime/Engine/Private/Actor.cpp:3194` (`RouteEndPlay`의 초기화 조건), `:3232` (`EndPlay`의 컴포넌트 종료). ISDK 구독/해제는 `Plugins/MetaXRInteraction/Source/OculusInteraction/Private/Interaction/IsdkPokeInteractable.cpp:135`, `:183`에서 확인.

### ⑤ 로컬에서 확인할 것

- Claude 코드 리뷰. 로컬 MCP 도구의 `StructUtils` 경고와 §7 무경고 조건도 함께 확인한다.
- 리뷰 뒤 Claude가 명세 §6대로 기존 `L_S2_ContactProbe`에 BP 패널·가짜 Door B·manager를 구성하고 PIE Z/X·inactive 표시·경고·배치를 확인한다. 현 상태 `not-tested`.
- 사용자 Quest 3 Link T-03 일부: 실제 손 poke로 절대 번호 전환, 중복 누름, limiter 손 모델, 원본 손 관절과 판정 독립성, 좌석에서 패널 위치·크기, 키보드 전환·접촉 초기화. 현 상태 `not-tested`.
- T-02는 기존 S2 상태 그대로 `not-tested`. 판정 방식·기본값 결정은 실제 Quest 확인 뒤다.
- 로컬 커밋만 남긴다. push·PR·main 머지 없음.

## 코드 리뷰 1회차 — Claude 2026-11-23 (`73e3f49`) — 통과

### 재현

- non-unity(`-DisableUnity -Module=ReachStudy`): 소스 시간을 갱신해 6개 cpp를 다시 컴파일, `Result: Succeeded`, 경고·오류 0.
- 실제 unity: 빌드 XML(`bForceUnityBuild`, `bUseAdaptiveUnityBuild=false`)을 잠시 넣고 `Module.ReachStudy.cpp` 컴파일 확인, `Succeeded`, 경고 0. XML은 원래대로 되돌림.
- `ReachStudy.*` 16개(S2 9 + S4 7) 모두 `Success`, `EXIT CODE: 0`.
- `StructUtils` 폐기 예정 메시지는 git 밖 `Plugins/McpAutomationBridge/McpAutomationBridge.uplugin:207`의 플러그인 의존에서 나온다. ReachStudy 코드·uproject와 무관한 환경 항목으로 본다(§7 무경고 조건은 ReachStudy 컴파일 기준으로 충족). Agent-Workflow 무시 목록에 적음.

### 확인한 것

- D2: 범위 검사 → 같은 번호 무시 → 적용 → 이벤트 한 번 순서(`ReachDoorVariants.cpp` `SelectVariant`). 토글 없음. 키·버튼 모두 `SelectVariant`.
- D3: 원래 숨김·collision을 BeginPlay에서 한 번 기억하고 EndPlay에서 되돌림. `GetAttachedActors(Actors, false, true)`는 `bResetArray=false`라 목록 actor 자신이 남는다(`Actor.h:3558`). component 설정은 건드리지 않음 — 시험이 material·mesh·transform·response까지 확인.
- D5: ISDK `Widget`은 생성자 기본 subobject라(`IsdkInteractableWidgetComponent.cpp:46`) 패널 BeginPlay의 `GetWidget()`가 null일 수 없다.
- D7: poke만 삭제 목록에서 빠지고 ray·grab·grab 검출 세 종류는 그대로 꺼짐. limiter는 파생 rig 생성자에서 등록(vBoR과 같은 방식).
- D8: 전환 → `ResetContacts`가 그 자리에서 overlay·상태·손 이력을 비움. 다음 첫 실측은 `Idle`(S2 D4), 그다음 `Touching` — 시험 E9가 프레임 단위로 확인. 꺼진 target은 질의 자체를 건너뜀.
- 제품 코드(`Door/`, `Hand/`)에서 `Development/` 참조 없음. 주석·문구 영어.

### 문제

없음.

### 제안 (머지를 막지 않음, S3 명세에서 같이 정리)

- **S-1 Door가 3개 이상일 때 공유 actor 규칙.** `IsActorActive`는 `VariantIndices.Num() > 1`이면 언제나 활성이다. Door 2개에서는 명세 D3와 같지만, `C`·`V`로 3·4번 Door를 쓰면 A·B에만 있는 actor가 C 활성 때도 보인다. "활성 Door에 들어 있으면 활성"(`!State || State->VariantIndices.Contains(ActiveVariant)`)이 2개일 때 결과가 같고 3개 이상에서도 맞다. 명세 D3 문구("두 Door에 다 들어 있는")가 모호했던 탓이다 — Claude가 명세를 고친다.
- **S-2 `InitialVariant`가 범위 밖일 때.** 경고는 나오지만 `ActiveVariant`가 `INDEX_NONE`이라 모든 Door actor가 보이고 collision도 켜진 채, 시험대는 그 target을 전부 `inactive`로 표시한다(보이는 것과 판정이 어긋남). 0으로 되돌리거나 모두 끄는 것 중 하나로 정한다.

### 확인할 시험

- §6 레벨 작업 뒤 PIE: `Z`/`X`로 `T_KneeSide`/`T_KneeSideB` 전환, 시험대 `inactive` 줄, 패널 버튼 색, 경고 없음 (Claude, MCP).
- Quest 3 T-03 일부(명세 §8) — `not-tested`.

### 다음 → 사용자 결정으로 바뀜 (아래)

## 1회차 뒤 사용자 결정 — 2026-11-23 → 명세 개정 1, 같은 브랜치에서 반영 (반영함, 리뷰 2회차 대기)

사용자가 S-1·S-2를 머지 전에 고치기로 했고, 패널 방식도 바꿨다. 명세 [S4 개정 1](../Specs/S4-door-switch.md)에 반영했다. 아래는 모두 **문제**(머지 전 반영)다.

- **P-1 (S-1)** `IsActorActive` = 활성 Door 소속(`Contains(ActiveVariant)`), 여러 Door에 든 actor는 BeginPlay 경고(D3). 사용자: Door 하나 = 부모 actor 하나, Door끼리 actor를 나누지 않음(D1). — 반영함 (8852a71)
- **P-2 (S-2)** 범위 밖 `InitialVariant` → 경고 + 0번(D1). — 반영함 (8852a71)
- **P-3 패널 = `[<] Door A (1/2) [>]`** Door별 버튼·활성 색을 없애고 이전·다음 버튼 + 가운데 지금 Door 글(D6). — 반영함 (8852a71)
- **P-4 상대 명령** `SelectNext`·`SelectPrevious`, 끝에서 돌아감, 쿨다운 `SwitchCooldownSeconds` 0.5 s(상대 명령만 막음, 절대 번호는 막지 않음), 키 `N`·`B`(D2·D4). — 반영함 (8852a71)
- 시험: E4b·E6(3 Door)·E12·E13·E13b–d·E14 바뀜/추가(§4).

## Codex 개정 1 구현 보고 — 2026-11-23 (`8852a71`, Claude 리뷰 2회차 대기)

① 바뀐 파일
- Docs/Reviews/S4-door-switch.md: P-1~P-4 반영 커밋과 이 보고. HANDOFF.md(git 무시): Codex 결과·리뷰 대기 상태.
- Source/ReachStudy/Door/ReachDoorVariants.h·cpp: 공유 actor 경고·활성 소속 검사, 잘못된 InitialVariant의 0번 복구, 이전·다음 명령·실시간 쿨다운·N/B 입력.
- Source/ReachStudy/Door/ReachDoorPanelWidget.h·cpp: Door별 버튼을 이전·현재 이름(번호/개수)·다음으로 교체. ActiveColor/IdleColor와 번호 버튼 클래스 제거, 버튼 글·색·글 크기 프로퍼티.
- Source/ReachStudy/Tests/ReachDoorTestObserver.h, ReachDoorTests.cpp: 합성 시각과 개정 1 시험.

② 완료 조건별 충족 여부
- §4 E1–E17 및 E4b·E13b–d: ReachStudy.S4.* 11개 성공. E6은 2/3 Door에서 공유 actor 활성 범위와 actor당 경고 한 번, E4b는 잘못된 시작 번호와 빈 목록, E12–E14는 N/B·순환·0.5 s 경계·0 s·절대 명령 우회·동일 순간 키/버튼 양쪽 순서·현재 이름 갱신·Door 하나의 패널을 확인.
- ReachStudy.* 전체 한 번 실행: S2 9 + S4 11 = 20개 Success, 실패 0, TEST COMPLETE. EXIT CODE: 0, 프로세스 종료 0.
- non-unity(-DisableUnity -Module=ReachStudy) 및 실제 unity(Module.ReachStudy.cpp) 각각 한 번, Result: Succeeded, 종료 0, 두 빌드 로그 경고·오류 0(UHT -WarningsAsErrors 인자 제외).
- 보고 ①–⑤ 충족. §6 Claude 에디터 배치·PIE 경고 없음은 not-tested, Claude 2회차 리뷰 대기.

③ 명세와 다르게 한 것과 이유
- 동작·범위 차이 없음. S4b 화면 글·No door variants와 S3는 손대지 않음. 에디터·에셋 변경 없음.
- §5 재량: FPlatformTime::Seconds(double)로 pause/time dilation과 독립인 시각 사용. protected 시각 함수 하나를 시험 actor에서 재정의하여 실제 기다림 없이 검사. 기본 글 크기 24, 회색 버튼·흰 글.
- unity 증거 확보를 위해 Saved/UnrealBuildTool/BuildConfiguration.xml의 강제 unity·adaptive 끔을 잠시 적용, finally에서 원래 바이트 복원.
- -log=S4-Rev1-Automation.log를 주었으나 실제 출력은 Saved/Logs/ReachStudy.log에 생김. 재실행하지 않고 완료 로그를 S4-Rev1-Automation.log로 복사 보존.
- 자동화 시작 시 기존 ISDK 샘플 InfoCardWidget CDO 오류 2건과 헤드셋 미연결/OpenXR/Meta 에셋·에디터 환경 경고 존재. 시험 실행 구간 경고·오류 0. 평가 결과 기록 기능은 추가하지 않음.
- 로그(git 밖): Saved/Logs/S4-Rev1-NonUnity.log, S4-Rev1-Unity.log, S4-Rev1-Automation.log, S4-Rev1-Automation-stdout.log.

④ 사용 API — 로컬 헤더 직접 확인, 추정 아님
엔진 기준 C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/.
- Core/Public/Windows/WindowsPlatformTime.h:23: Seconds(), QueryPerformanceCounter 기반 double 시각.
- Engine/Classes/GameFramework/Actor.h:1925,1929,1933,3558,4521: actor 숨김·collision·재귀 부착 actor·원래 숨김 값.
- UMG/Public/Components/Button.h:129: SetBackgroundColor; TextBlock.h:114: SetColorAndOpacity; HorizontalBoxSlot.h:58,63: 가운데 정렬.
- Core/Public/Internationalization/Text.h:411,676: AsNumber(int32), Format; SlateCore/Public/Styling/SlateColor.h:62: FLinearColor 변환.
- Plugins/MetaXRInteraction/Source/OculusInteractionPrebuilts/Public/IsdkInteractableWidgetComponent.h:57,209,230,237,244,251: 기존 패널의 widget/poke/ray/DrawSize/WidgetScale/WidgetClass. 이번 변경은 새 플러그인 API를 더하지 않음.

⑤ 로컬에서 확인할 것
- 빌드 두 종류·전체 자동화는 위와 같이 로컬 확인 완료. Claude 리뷰 2회차와 재현 대기.
- Claude: 명세 §6 MCP 에디터 작업과 PIE, 이전·다음 패널 배치·글·경고 확인 — not-tested.
- 사용자 Quest 3 Link T-03: 이전·다음 실제 poke, 한 번에 한 칸, 0.5 s 쿨다운 체감, 가운데 글·패널 크기/위치, limiter와 원본 손 관절 독립, 키보드 전환·상태 초기화 — not-tested.
- T-02 실제 손 닿음은 not-tested. 합성 시험·빌드는 기기 시험이 아님.
- 같은 codex/S4-door-switch 브랜치에 새 커밋. push·PR·main 머지 없음.
