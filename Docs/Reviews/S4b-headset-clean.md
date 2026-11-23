# S4b 구현 결과 — Claude 리뷰 대기

작성 2026-11-24 (Codex). 명세: `Docs/Specs/S4b-headset-clean.md`.
브랜치: `main@44999a3`에서 만든 `codex/S4b-headset-clean`.

## ① 바뀐 파일

- `Source/ReachStudy/Screen/ReachScreenMessageSubsystem.h`, `.cpp` (신규): Game·PIE world 시작 때 화면 메시지 설정을 기억하고 끔, Deinitialize에서 복원. 시작하지 않은 world는 복원하지 않음.
- `Source/ReachStudy/Development/ReachContactProbe.h`, `.cpp`: DrawReadout·BuildReadout, 글 조립 전용 함수, 화면 메시지 추가·제거 호출 삭제. 판정·overlay·키 입력은 유지.
- `Source/ReachStudy/Door/ReachDoorPanelWidget.h`, `.cpp`: NoDoorVariantsText 프로퍼티·안내 글 제거. 연결 없음 또는 Variants 빈 배열이면 자식 위젯 0개.
- `Source/ReachStudy/Door/ReachDoorPanel.cpp`: 연결된 manager의 Variants가 비어도 패널 시작 경고 한 줄.
- `Source/ReachStudy/Tests/ReachDoorTests.cpp`: D2의 글 검사를 활성 번호·target 연결 값 검사로, D3의 빈 패널 검사를 자식 0개로 변경. 기존 bInactive·Phase·overlay 검사는 유지.
- `Source/ReachStudy/Tests/ReachScreenMessageTests.cpp` (신규): E1–E4 자동화 4개.
- `Docs/Reviews/S4b-headset-clean.md`: 본 보고.
- `HANDOFF.md` (git 무시): Codex 결과 갱신.

## ② 명세 완료 조건

| 조건 | 결과 |
|---|---|
| E1 Game world 시작·종료, 시작 전 true/false 복원 | 충족. Game·PIE 각각 true/false, BeginPlay 전 설정 유지, 시작하지 않은 world의 종료도 확인 |
| E2 Editor world | 충족. subsystem 없음, true/false 모두 생성·종료 시 설정 유지 |
| E3 시험대 몇 프레임, 메시지 없음·S2 판정 유지 | 충족. 화면 메시지를 켠 상태로 ProcessFrame·Tick 뒤 시험대 키 없음. 진입·Touching·overlay·이탈·overlay 복원 확인 |
| E4 빈 Door 패널 | 충족. 연결 없음·Variants 빈 배열 모두 버튼·글 0개. 각 패널 BeginPlay 경고 한 줄을 기대 메시지 수로 확인 |
| S2·S4 기존 시험 유지 | 충족. S2 9개·S4 11개 성공. 명세가 지정한 글 검사만 변경 |
| 전체 시험 한 번 | `ReachStudy.*` 24개(S2 9 + S4 11 + S4b 4) Success, 실패 0, `TEST COMPLETE. EXIT CODE: 0`, 프로세스 종료 0 |
| non-unity 한 번 | `-DisableUnity -Module=ReachStudy`, `Result: Succeeded`, 종료 0, 컴파일 경고·오류 0 |
| 실제 unity 한 번 | `Module.ReachStudy.cpp` 컴파일, `Result: Succeeded`, 종료 0, 컴파일 경고·오류 0 |
| 소스 전체 화면 출력 호출 없음 | `rg -n 'AddOnScreenDebugMessage|AddToViewport|PrintString' Source` 결과 0건(일치 없음 종료 1). DrawReadout·BuildReadout도 0건 |
| 새 모듈 의존 없음 | 충족. Build.cs 변경 없음 |

빌드 로그의 UHT `-WarningsAsErrors`는 경고가 아닌 인자다. 로컬 MCP 플러그인의 StructUtils 폐기 예정 안내는 기존 환경 메시지다.

자동화 시작 시 기존 `IsdkInfoCardComponent` CDO 오류 2건(미사용 ISDK 샘플 InfoCardWidget 누락), Meta XR 확장·에셋 엔진 버전 등 환경 경고가 있었다. 첫 Test Started부터 완료까지 Warning/Error 0건이며 24개 모두 성공했다.

로그(git 무시):
- `Saved/Logs/S4b-NonUnity.log`
- `Saved/Logs/S4b-Unity.log`
- `Saved/Logs/S4b-Automation.log`
- `Saved/Logs/S4b-Automation-stdout.log`

`-log=S4b-Automation.log`가 엔진 명령행에서 `-log=S4b-Automation .log`로 해석되어 기본 `ReachStudy.log`에 기록됐다. 종료 뒤 그 파일을 `S4b-Automation.log`로 보존 복사했다. stdout 로그에도 24개 결과와 종료 0이 있다.

## ③ 명세와 다르게 한 것과 이유

기능 범위 차이 없음. subsystem은 제품 코드 `Screen/`에 배치했다(§5 재량).
실제 unity를 확인하기 위해 git 무시 `Saved/UnrealBuildTool/BuildConfiguration.xml`에 `bForceUnityBuild=true`·`bUseAdaptiveUnityBuild=false`를 잠시 적용했고, finally에서 원래 바이트로 복원했다(SHA256 일치).
리포 규칙에 따라 빌드 두 종류와 전체 시험은 작업 끝에 각각 한 번 실행했다. 에디터·에셋·설정·S3 구현 작업은 하지 않았다.

## ④ API 직접 확인

추정으로 쓴 API 없음. UE 5.7.4 설치 소스와 로컬 ISDK 헤더를 직접 확인했다.

- `C:/Program Files/Epic Games/UE_5.7/Engine/Source/Runtime/Core/Public/CoreGlobals.h:426`: GAreScreenMessagesEnabled 선언.
- `Engine/Source/Runtime/Engine/Private/UnrealEngine.cpp:10310`, `:13286` (위 UE 설치 경로): 콘솔 끄기의 false 대입, 화면 그리기의 전역 플래그 조건.
- `Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h:30`, `:45`: ShouldCreateSubsystem·OnWorldBeginPlay 가상 함수. 명세 §0의 헤더 줄 번호와 현재 설치 헤더는 다르며 실제 선언은 이 줄에서 확인.
- `Engine/Source/Runtime/Engine/Public/Subsystems/Subsystem.h:67`: Deinitialize 가상 함수.
- `Engine/Source/Runtime/Engine/Private/Subsystems/WorldSubsystem.cpp:28`, `:70`: world 생성 조건과 BeginPlay의 Super 호출 확인.
- `Engine/Source/Runtime/Engine/Classes/Engine/World.h:3259`, `:3264`, `:3855`: 시험의 CreateWorld·DestroyWorld·BeginPlay.
- `Engine/Source/Runtime/Engine/Classes/Engine/Engine.h:2144`: 시험대 키의 OnScreenDebugMessageExists.
- `Engine/Source/Runtime/UMG/Public/Components/PanelWidget.h:28`, `:122`: 자식 수 검사·패널 비우기.
- `Engine/Source/Runtime/UMG/Public/Components/WidgetComponent.h:180`: GetUserWidgetObject.
- `Plugins/MetaXRInteraction/Source/OculusInteractionPrebuilts/Public/IsdkInteractableWidgetComponent.h:57`: GetWidget (빈 패널 시험). 제품 코드에 새 Meta/ISDK API 추가 없음.

## ⑤ 로컬에서 확인할 것

- 빌드 두 종류·자동화 전체: 위와 같이 이 PC에서 확인 완료.
- 명세에 에디터 작업 없음. 실제 PIE 종료 뒤 에디터 뷰포트 메시지 복원은 `not-tested`(자동화의 합성 PIE world 설정 복원과 구분).
- Quest 3 Link VR Preview에서 엔진 경고·시험대 숫자 등 화면 글이 없고 참가자 Door 패널의 이전·Door 이름·다음만 보이는지: `not-tested`(S4b §7, 계획서 T-02·T-03 진행 시 함께).
- Quest 3 T-02 접촉 색 변화와 T-03 Door 전환: 이번 작업에서 기기 시험 안 함, `not-tested`.
- Claude 리뷰 대기. push·PR·main 머지 없음. 다음 명세는 시작하지 않는다.

## 질문

없음.
