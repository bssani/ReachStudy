# ReachStudy — 운전석 Reach 확인 (UE 5.7.4 + Meta Quest 3)

참가자가 운전석(실물 좌석)에 앉아 Driver Door와 IP의 target mesh를 손으로 만진다. 닿아 있는 동안 그 target의 material이 바뀐다. Driver Door는 두 종류(A/B)를 바꿔 가며 보고, IP는 하나 그대로다.

**작업 전에 [Docs/Plan.md](Docs/Plan.md)를 읽을 것.** 범위·결정·단계·시험이 모두 거기 있다. 이 파일은 코드를 쓸 때 매번 필요한 규칙만 뽑은 것이다.

작업 방식과 규칙의 원본은 vBoR 리포다. 이 파일은 그중 ReachStudy에 맞는 것만 옮겼다. Codex와 Claude Code가 함께 읽고, `CLAUDE.md`는 이 파일을 가져와 Claude 전용 내용만 덧붙인다.

> **이 리포는 public이다.** 차량 CAD·회사 이름·프로그램명·일정·평가 결과를 커밋·PR·이슈·문서에 넣지 않는다. CAD는 `Content/CAD/`에만 두고(.gitignore), Meta 플러그인은 커밋하지 않는다(Oculus SDK License).

## 작업 방식 — 명세·리뷰는 Claude, 구현은 Codex

- 작업 단위는 [Docs/Specs/](Docs/Specs/)의 명세 파일 하나다. 명세의 범위와 완료 조건만 구현한다. 명세에 없는 기능·리팩터링·"나중에 쓸" 추상화를 더하지 않는다.
- 명세가 이 파일·계획서·플러그인 헤더와 어긋나면 임의로 해석하지 말고 멈춰서 보고한다. 재량은 명세의 "구현자가 정할 것"에 적힌 항목뿐이다.
- 브랜치는 `codex/<명세 파일 이름>`, 베이스는 명세에 적힌 브랜치다.
- 작업을 끝낼 때 보고한다: ① 바뀐 파일 ② 명세의 완료 조건별 충족 여부 ③ 명세와 다르게 한 것과 이유 ④ 쓴 플러그인·엔진 API를 헤더로 확인했는지(경로:줄) 추정인지 ⑤ "로컬에서 확인할 것" — 빌드, 에디터 작업, Quest 3 시험(계획서의 T-ID).
- 같은 보고를 그 작업 마지막 커밋 메시지 본문에도 쓴다.
- 리뷰 지적은 같은 브랜치에 새 커밋으로 반영한다. amend·rebase로 리뷰한 커밋을 고쳐 쓰지 않는다.
- 리뷰 통과와 로컬 빌드 확인 전에는 `main`에 머지하지 않는다.
- **주고받는 내용은 파일로 넘긴다.** 리뷰는 [Docs/Reviews/](Docs/Reviews/)`<명세 이름>.md`에 회차별로(항목 상태 `열림` / `반영함 (<커밋>)`). 항목 형식은 **문제**(반드시 해결)·**제안**(권장)·**확인할 시험**, 필요할 때만 짧은 스케치. 질문은 리뷰 파일 "질문" 절에 쓰고 멈춘다.
- **진입점은 리포 루트의 `HANDOFF.md`(git 무시).** Claude가 지금 할 일·브랜치·명세·리뷰 파일을 적는다. Codex는 먼저 이 파일을 읽고, "지금 할 일"이 없으면 아무것도 하지 않는다. 끝나면 "Codex 결과" 절을 채운다.
- 구현은 Codex 데스크톱 앱에서 한다. Claude는 `codex exec`로 구현을 시작하지 않는다.
- **일을 키우지 않는다.** 하위 에이전트·자체 리뷰 루프 없음. 빌드(non-unity `-Module=ReachStudy` + unity)와 `ReachStudy.*` 전체 시험은 작업 끝에 한 번. 가장 단순한 방법으로 명세를 채우고, 막히면 질문한다.
- 절차·명령·로그 위치는 [Docs/Agent-Workflow.md](Docs/Agent-Workflow.md).

## 코드·에셋은 영어만 — 주석은 짧은 조각문

코드와 에셋에는 한글을 쓰지 않는다. 주석, `Category`·`DisplayName`·`ToolTip`·`UMETA`, 로그, 화면 문구가 모두 영어다. 문서(`Docs/`)·커밋 메시지·리뷰 파일은 한국어.

- 영어 조각문으로 짧게. **왜** 그런지를 쓴다: `Ignore if already running (double click)`.
- 근거 결정·명세 ID를 괄호로: `(D-04)`, `(S2 §3)`.
- 단위를 적는다(`mm`, `cm`, `s`). UE는 cm다. 표시 단위가 다르면 변환하는 곳에 적는다: `UE is cm, setting is mm`.
- 코드를 그대로 읽어 주는 주석은 쓰지 않는다.

## 확정된 제약 — 바꾸려면 사용자 승인

- **장비는 Meta Quest 3 단독, PCVR(Link).** 컨트롤러 없음, 손 데이터는 광학 핸드 트래킹에서만. Android 독립 실행은 고려하지 않는다.
- **VR이다. 실물은 좌석뿐이다.** Door·IP는 허공이라 손이 가상 형상을 통과한다. 촉감이 없으므로 접촉은 시각(material 변화)으로만 전달된다.
- **닿음 판정은 추적된 실제 손 관절로 한다.** poke limiter·손 모델 보정이 옮긴 위치를 쓰지 않는다. 판정 영역을 보이는 형상보다 키우지 않는다 — 여유는 허용 거리(`ContactDistanceMm`)와 떨어짐 여유(`ReleaseMarginMm`)뿐이다. 관절 반지름을 쓰는 것(S2 D5)은 여유가 아니라 관절 중심 대신 손 겉면에서 재는 위치 보정이다.
- **추적 손실은 보류이지 판정이 아니다**(vBoR 원칙). 추적이 끊긴 프레임과 다시 잡힌 첫 프레임은 판정하지 않는다(복구 순간의 튐으로 닿음이 생기지 않게). 짧은 끊김 동안은 그 손의 마지막 결과를 보류하고(dwell은 멈춤), 정한 시간(`TrackingGraceSeconds`)을 넘으면 그 손은 "안 닿음"이다. 손마다 따로 센다(S2 D4).
- **다른 면을 통과해 닿는 것은 따지지 않는다**(사용자 2026-11-22).
- **기록 없음.** 결과는 화면 확인만 한다(사용자 2026-11-22). 파일·로그로 평가 결과를 남기는 시스템을 만들지 않는다.
- **헤드셋 안에는 어떤 메시지도 보이지 않는다**(사용자 2026-11-23). 디버그·경고·안내 글을 화면에 쓰지 않는다(`AddOnScreenDebugMessage`·`AddToViewport`·`PrintString` 금지, 화면 메시지는 꺼 둔다). 진행자 정보는 PC 창(스펙테이터 화면)에만 — Door·IP 이름(S4b). 헤드셋 안의 글은 참가자가 누르는 Door 패널뿐. 알림은 `UE_LOG`로.
- **원거리 선택·레이 포인팅 금지.** 손을 뻗지 않고 닿게 되면 reach 확인이 의미를 잃는다.
- **Buck 정렬은 설치 때 한 번이다.** 참가자마다 다시 맞추지 않는다(체격 차이가 지워짐). 정렬 방식은 마커 + 앵커(계획서 D-01).
- **Door 전환:** 진행자 키보드 + 참가자가 누르는 VR 안의 3D widget 버튼(poke, 계획서 Q-05). 같은 명령으로 간다. 패널은 이전·다음 버튼(사용자 2026-11-23, S4 개정 1).

## 구조 원칙

- **닿는 target은 C++ component(`UReachTouchComponent`)를 BP(Door·IP) 안의 mesh 밑에 붙여 만든다**(계획서 D-13). 개수·이름을 코드에 두지 않는다. 그 밖의 상호작용 대상(Door 패널 등)은 C++ 부모 Actor + 자식 BP. C++에는 지금 필요한 기능만, BP는 mesh·값만 바꾸면 바로 적용돼야 한다.
- **BP Event Graph에는 표현만.** 닿음 판정·상태는 C++. BP는 이벤트(`OnTouchBegin` 등)에 반응하는 표현만 붙인다.
- **입력(키보드·3D 버튼) → 같은 명령 → 상태 변경.** 두 입력이 동시에 와도 상태가 뒤집히지 않게 한다. 번호로 고르는 명령은 그 자체로 안전하고, 이전·다음 같은 상대 명령은 전환 뒤 쿨다운으로 막는다(S4 D2).
- **움직이는 mesh 피벗에 기대지 않는다.** CAD mesh는 피벗이 차 원점에 묶여 온다. 위치 판정은 mesh 형상(collision) 기준이다.
- **두 Door와 IP는 같은 차 원점이다**(사용자 2026-11-22). Door 전환은 같은 레벨 안에서 켜고 끈다. 레벨 로드로 바꾸지 않는다(Pawn·정렬 유지).
- 합격 수치·reach 기준을 임의로 만들지 않는다. 평가 담당자와 합의할 사항이다.

## 하지 말 것

- VR 닿음 결과를 실차 reach 검증으로 서술하지 않는다(촉감 없음, 다른 면 통과, 추적 오차, 정렬 오차).
- 실제 Quest 3로 확인하기 전까지 시험 상태는 `not-tested`다. 컴파일 = 시험 통과가 아니다.

## 빌드·환경

- UE 5.7.4 (`C:\Program Files\Epic Games\UE_5.7`), C++ 모듈 `ReachStudy`. 빌드: `Engine/Build/BatchFiles/Build.bat ReachStudyEditor Win64 Development -Project=<절대경로>\ReachStudy.uproject`
- **Meta XR 1.205.0(`Plugins/MetaXR/`)과 ISDK 1.205.0(`Plugins/MetaXRInteraction/`)은 git에 없다.** 클론한 뒤 직접 복사한다(README). OculusXR·ISDK API는 기억이 아니라 그 폴더의 헤더로 확인한다.
- 에디터는 `.uasset`을 잠근다. git 브랜치 전환·머지·C++ 리빌드 전에 Unreal Editor를 닫는다.
- **에디터 작업(레벨·에셋·배치)은 에이전트가 MCP로 직접 한다**(사용자 2026-11-23). 에디터 MCP 플러그인 `Plugins/McpAutomationBridge`·`Plugins/UnrealAgent`는 vBoR과 같은 로컬 도구다(git 밖, 각 PC에서 복사). 연결·순서·하지 않는 것은 [Docs/Agent-Workflow.md](Docs/Agent-Workflow.md) §2.8. ReachStudy 코드·에셋은 이 플러그인을 참조하지 않는다.
