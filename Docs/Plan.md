# ReachStudy 계획서

작성 2026-11-22 (Claude). 결정이 바뀌면 이 문서를 고치고 아래 "개정 이력"에 남긴다.

## 1. 한 줄

참가자가 실물 운전석에 앉아 VR 속 Driver Door·IP의 target mesh를 손으로 만지면, **닿아 있는 동안** 그 target의 material이 바뀐다. Driver Door는 두 종류(A/B)를 진행자가 바꾼다. 결과는 화면으로만 확인한다.

## 2. 결정 (사용자 2026-11-22)

| ID | 결정 | 내용 |
|---|---|---|
| D-01 | **정렬 = 마커 + 앵커** | vBoR의 마커 정렬(M1/M2, ArUco + Passthrough Camera Access + `FitPoints`)과 앵커 유지(M3b)를 쓴다. 앵커가 Link에서 되는지는 사용자가 회사에서 확인한다(2026-11-23 예정). 안 되면 앵커 없이 마커만 쓴다 |
| D-02 | **닿음 부위 = 드롭다운** | `IndexTip`(검지 끝) / `Fingertips`(다섯 손가락 끝, **기본값**) / `WholeHand`(손 전체 관절 26개). 에디터 드롭다운(UENUM) (사용자 2026-11-22 Q-03) |
| D-03 | **허용 거리 조절 가능** | 손 관절과 target 표면 사이 거리 ≤ `ContactDistanceMm`이면 닿음. 값은 프로퍼티로 고칠 수 있다 (권장 기본값 §4). 관절 중심에서 잴지 손 겉면(관절 반지름을 뺀 곳)에서 잴지는 S2에서 정한다. 손이 target 표면 뒤로 깊이 들어간 경우의 처리는 S2에서 정한다(CAD target이 닫힌 부피인지 얇은 면인지에 달림) |
| D-04 | **dwell 조절 가능** | 닿음 조건이 `DwellSeconds` 동안 이어져야 닿음으로 바뀐다. 0이면 즉시. 손 추적이 잠깐 끊긴 동안은 dwell을 멈춘다(0으로 되돌리지 않음, §4·S2 D4) |
| D-05 | **닿는 동안만 material 변경** | 떨어지면 바로 원래 material로 돌아간다. 누적·완료 상태 없음 |
| D-06 | **다른 면 통과는 따지지 않음** | Door trim·IP를 통과해 닿아도 닿음 |
| D-07 | **기록 없음, 화면 확인만** | 평가 결과 파일·로그를 만들지 않는다 |
| D-08 | **Door 전환 = 진행자 키보드 + 3D widget 버튼** | 같은 명령. 전환하면 target 상태를 초기화한다. 패널은 이전·다음 버튼 + 지금 Door 이름, 끝에서 처음으로 돌아감, 전환 뒤 0.5 s 상대 입력 무시. Door는 늘어날 수 있다(사용자 2026-11-23, S4 개정 1) |
| D-09 | **두 Door와 IP는 같은 차 원점** | CAD export 원점이 같다. 같은 레벨에 두고 켜고 끈다 |
| D-10 | **관전(TV) 화면 없음** | |
| D-11 | **프로젝트** | `C:\Git\ReachStudy`, GitHub `bssani/ReachStudy` **public**. UE 5.7.4, Meta XR·ISDK 1.205.0(vBoR과 같은 버전, git에는 넣지 않음) |
| D-12 | **작업 방식 = vBoR과 같음** | Claude 명세·리뷰, Codex 앱 구현, 사용자 에디터·Quest 시험 |
| D-13 | **target = mesh 밑에 붙이는 component** | `BP_Door_A`·`BP_Door_B`·`BP_IP` 같은 BP 안에서 닿아야 할 mesh 밑에 `UReachTouchComponent`를 붙인다. 닿는 동안 그 mesh만 색이 바뀐다. target 개수는 BP마다 다르고 제한이 없다(사용자 2026-11-23, [S3 명세](Specs/S3-touch-component.md)) |

## 3. 열린 결정 → 결정 (사용자 2026-11-22)

| ID | 결정 |
|---|---|
| Q-01 | **정렬은 vBoR에서 Calibration 플러그인을 먼저 뺀 뒤 그 플러그인을 쓴다.** 플러그인 추출은 vBoR 리포의 작업이다(vBoR `Docs/Design/2026-11-20-calibration-plugin-handoff.md`). S1은 그 뒤에 한다. 그동안 S2·S3·S4는 정렬 없이 진행한다 |
| Q-02 | 회사 vBoR 정렬 코드와 이 PC `main@1a17da9`의 차이 — 플러그인 추출 전에 확인(vBoR 작업) |
| Q-03 | `Fingertips` = 다섯 손가락 끝(`ThumbTip`·`IndexTip`·`MiddleTip`·`RingTip`·`LittleTip`). `WholeHand` = 손 관절 전부(`EHandKeypoint` 26개) |
| Q-04 | (제안대로) 설정 D-02~D-04는 레벨에 하나 두는 설정 Actor에 모은다. target별 덮어쓰기 없음 |
| Q-05 | **3D 버튼은 참가자가 누른다.** Door·IP 밖의 빈 공간(예: 조수석 쪽)에 둔다. 버튼은 판정 대상이 아니다 |
| Q-06 | **진행자 PC 화면 = 현재 Door 이름 + IP 이름만, PC 창에만**(vBoR 스펙테이터 방식). **헤드셋에는 어떤 메시지도 보이지 않는다** — 화면 메시지 전체 끔, S2 시험대 화면 글 없앰(사용자 2026-11-23, 전의 정렬·닿은 target·손 추적 줄은 뺌). [S4b 명세](Specs/S4b-pc-screen.md) |

## 4. 허용 거리·dwell 권장 기본값

**허용 거리 `ContactDistanceMm` 기본 10 mm (조절 범위 0–50).**
- 오차 원천: 마커 정렬 점별 XY 1.5–1.8 cm(vBoR M2 집 시험), 광학 손 추적 수 mm–1 cm, CAD target 형상 자체.
- 정렬 오차(~1.5 cm)를 다 덮을 만큼 키우면 "1.5 cm 앞에서 닿음"이 정상 동작이 된다 → reach가 실제보다 길게 나온다. 반대로 0이면 정렬이 안쪽으로 어긋난 쪽 target은 거의 안 닿는다.
- 그래서 **기본은 10 mm로 두고, Buck마다 확인점으로 실제 오차를 보고 조정**하는 것을 권한다. vBoR M1의 확인점 기능(`Align.Check` 태그 점, 손끝 원본과의 차이 표시)을 같이 가져오면 설치 때 "이 Buck은 오차 8 mm" 같은 근거로 값을 정할 수 있다.
- **떨어짐 히스테리시스 `ReleaseMarginMm` 기본 5 mm**: 닿음은 ≤ 10 mm, 떨어짐은 > 15 mm. 경계에서 손이 떨려 색이 깜빡이는 것을 막는다(vBoR 몰린 버튼 규칙과 같은 이유).

**dwell `DwellSeconds` 기본 0.15 s (조절 범위 0–2).**
- 팔을 뻗다 다른 target을 스치는 것(지나감)은 보통 0.1 s 안쪽이다. 0.15 s면 스침은 거르고, 의도한 접촉은 늦게 느껴지지 않는다.
- 떨어질 때는 dwell 없이 즉시 원래 material(D-05, 히스테리시스로 충분).
- **추적이 끊기면 그 손의 마지막 결과를 `TrackingGraceSeconds`(기본 0.1 s, 조절 범위 0–1) 동안 보류한다**(vBoR "추적 손실은 보류" 원칙). 보류 중에는 dwell이 더해지지도 0이 되지도 않고(멈춤), `Touching`은 유지된다. 보류 시간을 넘으면 그 손은 "안 닿음"이다. 다시 추적된 첫 프레임은 판정하지 않는다(보류 시간에 포함). 규칙 전체는 S2 D4.

모든 값은 근거 없는 초기값이다. S2 Quest 실측 뒤 다시 정한다.

## 5. 구성

| 요소 | 내용 |
|---|---|
| `UReachTouchComponent` (C++) | **target mesh 밑에 붙이는 component**(D-13). Door·IP BP 안에 몇 개든. 상태 `Idle` / `Dwelling` / `Touching`. Touching이면 부모 mesh 하나에 overlay material을 켜고(slot material·옆 mesh는 건드리지 않음), Idle로 돌아가면 끈다. 이벤트 `OnTouchBegin` / `OnTouchEnd`. BP에서는 component를 붙이고 닿음 material만 고른다. Door BP는 Door 부모 actor라 Door와 같이 켜지고 꺼진다. [S3 명세](Specs/S3-touch-component.md) |
| `AReachTouchSettings` (C++) | 레벨에 하나(Q-04). 부위·방식·mm·dwell·보류 시간·색. 판정은 world subsystem이 매 프레임 한 번에 한다(손 한 번 읽기, 손 근처가 아닌 target은 빠르게 거름) |
| 손 관절 읽기 | XR 원본 관절 위치(`GetHandTrackingState`, vBoR `AVBoRHandRigPawn::ReadRawIndexTip`과 같은 경로, S2 D2). 추적 끊김 = 그 프레임 판정 안 함, 결과는 보류(§4) |
| 닿음 판정 | 관절점마다 target 형상과의 닿음. 방식 A(simple collision까지 거리) / B(complex collision 구 overlap, 거리 값 없음) 중 하나를 **S2 결과로 정한다** |
| `AReachDoorVariants` (C++) | Door마다 Actor 목록 — **Door 하나 = 부모 actor 하나**, 붙은 자식 actor(target mesh 포함)가 같이 켜지고 꺼진다(사용자 2026-11-23). 활성 Door만 보이고 collision이 켜진다 — actor 단위 숨김·collision만 바꾸고 원래 값으로 되돌린다. 전환하면 모든 target 초기화(D-08). IP는 이 목록에 없다. [S4 명세](Specs/S4-door-switch.md) |
| Door 전환 입력 | 진행자 키보드(번호 `Z`·`X`·`C`·`V`, 이전·다음 `B`·`N`) + **참가자가 누르는** 3D widget poke 패널 `[<] Door A (1/2) [>]`(Q-05, vBoR `BP_LightPanel` 방식). 명령은 `SelectVariant(번호)`·`SelectNext`·`SelectPrevious` (S4) |
| 정렬 | vBoR Calibration 플러그인(Q-01). 운전석 하나, 시나리오·좌석 전환 없음 |

## 6. 단계

| ID | 명세 | 핵심 | 완료 조건 |
|---|---|---|---|
| **S0** | 프로젝트 부트스트랩 | 빈 C++ 프로젝트, 규칙 문서, Meta 플러그인 복사, git·GitHub | **완료 2026-11-22** (Claude). 빌드 결과는 개정 이력 |
| **S1** | 정렬 가져오기 | vBoR Calibration 플러그인을 넣는다(Q-01, 플러그인 추출 뒤). 마커 기록·저장·재시작 복원, 앵커 옵션(D-01) | 자동화 시험 통과, Quest에서 마커 정렬 → 재시작 복원 (T-01) |
| **S2** | 닿음 판정 spike | [명세](Specs/S2-contact-probe.md). 손 Pawn, 관절 읽기, 상태 규칙(순수 구조체), 거리 방식 A/B 비교 시험대 | Quest 실측 기록, 방식·기본값 결정 (T-02). 정렬 없이 |
| **S3** | 닿음 component | [명세](Specs/S3-touch-component.md). `UReachTouchComponent`·`AReachTouchSettings`·판정 subsystem, S2 판정의 component 단위 함수, 설정(D-02~D-04, Q-04), `AReachGameMode`. S4b 뒤. 방식 A/B·기본값은 T-02 뒤 확정 | 자동화 시험: 진입·dwell·이탈·히스테리시스·추적 끊김·부위 모드 3종, Door 전환 초기화 |
| **S4** | Door 전환 | [명세](Specs/S4-door-switch.md). `AReachDoorVariants`, 키보드·3D 버튼 패널, 손 Pawn poke 켜기, S2 시험대 연결 | 자동화 시험: 비활성 Door target 판정 안 됨, 전환 시 초기화, 동시 입력. 레벨은 Claude가 MCP로 |
| **S4b** | PC 화면 | [명세](Specs/S4b-pc-screen.md). PC 창에만 Door·IP 이름(Q-06), 화면 메시지 끔, 시험대 글·패널 안내 글 없앰 | 자동화 시험: 글 만들기, 메시지 끔/되돌림. Quest: 헤드셋에 글 없음 |
| **S5** | Main 통합 | CAD Datasmith import(`Content/CAD/`, git 밖), Door·IP를 BP로 묶고(Convert to Blueprint) target mesh에 touch component, `L_Main` | Quest 전체 흐름 (T-03) |

## 7. 시험 (Quest 3, 상태는 전부 `not-tested`)

| T-ID | 내용 |
|---|---|
| T-01 | 마커 정렬 저장 → 에디터·Link 재시작 → 같은 자리. 앵커 켬/끔 |
| T-02 | 검지 끝·다섯 손가락 끝·손 전체 각각으로 평면·곡면·작은 target 닿음. 경계에서 깜빡임 없음. 스침 거르기(dwell). 손이 잠깐 시야 밖으로 나가도 색이 깜빡이지 않음, 오래 나가면 원래 색(보류 시간) |
| T-03 | 운전석에서 Door A → 전환(키보드·3D 버튼) → Door B, 전환 뒤 초기화, IP target 그대로 동작 |

## 8. 위험 — 결과 해석에 같이 적을 것

- **VR reach는 실차보다 낙관적이다.** 손이 Door trim·IP를 통과하고(D-06), 촉감이 없어 지나치기 쉽고, 허용 거리만큼 앞에서 닿는다.
- **Door 쪽 target은 손 추적이 약하다.** 무릎·옆은 고개를 돌리지 않으면 Quest 카메라 시야 밖이다. 추적이 보류 시간(§4)보다 길게 끊기면 "안 닿음"으로 보이므로 진행자 창의 추적 상태와 같이 본다.
- **정렬 오차(~1.5 cm) 아래의 차이는 판정하지 않는다.** target 사이 거리가 그보다 작으면 결과로 구분하지 않는다.
- CAD mesh에는 보통 쓸 만한 collision이 없다(S2).

## 개정 이력

- 2026-11-22 — 초안. 사용자 답 D-01~D-12 반영, Q-01~Q-06 열림.
- 2026-11-22 — Q-01·Q-03·Q-05·Q-06 사용자 답 반영. D-02 `Fingers` → `Fingertips`(다섯 손가락 끝). D-03 "표면 안쪽 언제나 닿음"은 Claude가 근거 없이 넣은 것이라 빼고 S2에서 정함(엔진 거리 함수가 오목 형상을 지원하지 않음, S2 §0). S2 명세 작성.
- 2026-11-22 — S2 명세 개정 1·2와 맞춤(Codex 질문 S2 Q-02): §4 추적 끊김 = 손별 보류(`TrackingGraceSeconds`), D-04 dwell 멈춤, D-03 관절 중심/겉면은 S2, §5 overlay·방식 A/B 표기, T-02·§8 문구.
- 2026-11-23 — S4 명세 작성. `AReachVariantSet` → `AReachDoorVariants`, Door 전환 키 `Z`·`X`, 명령 `SelectVariant`. 진행자 PC 창(Q-06)을 S4 → S3로 옮김.
- 2026-11-23 — S4 개정 1(사용자): D-08에 이전·다음 패널·돌아가기·쿨다운, Door 수 늘어날 수 있음. §5 Door = 부모 actor 하나, Door target은 Door 밑에.
- 2026-11-23 — D-13(사용자): target = mesh 밑 component, BP마다 개수 다름. §5 `AReachTarget` → `UReachTouchComponent` + `AReachTouchSettings`, §6 S3·S5. S3 명세 작성.
- 2026-11-23 — Q-06 바꿈(사용자): PC 화면은 Door·IP 이름만, 헤드셋 메시지 없음. S4b 명세 작성, 진행 순서 S4 → S4b → S3.
