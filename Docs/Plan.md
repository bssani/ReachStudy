# ReachStudy 계획서

작성 2026-11-22 (Claude). 결정이 바뀌면 이 문서를 고치고 아래 "개정 이력"에 남긴다.

## 1. 한 줄

참가자가 실물 운전석에 앉아 VR 속 Driver Door·IP의 target mesh를 손으로 만지면, **닿아 있는 동안** 그 target의 material이 바뀐다. Driver Door는 두 종류(A/B)를 진행자가 바꾼다. 결과는 화면으로만 확인한다.

## 2. 결정 (사용자 2026-11-22)

| ID | 결정 | 내용 |
|---|---|---|
| D-01 | **정렬 = 마커 + 앵커** | vBoR의 마커 정렬(M1/M2, ArUco + Passthrough Camera Access + `FitPoints`)과 앵커 유지(M3b)를 쓴다. 앵커가 Link에서 되는지는 사용자가 회사에서 확인한다(2026-11-23 예정). 안 되면 앵커 없이 마커만 쓴다 |
| D-02 | **닿음 부위 = 드롭다운** | `IndexTip`(검지 끝) / `Fingers`(손가락 전부, **기본값**) / `WholeHand`(손 전체). 에디터 드롭다운(UENUM) |
| D-03 | **허용 거리 조절 가능** | 손 관절과 target 표면 사이 거리 ≤ `ContactDistanceMm`이면 닿음. 표면 안쪽(관통)은 언제나 닿음. 값은 프로퍼티로 고칠 수 있다 (권장 기본값 §4) |
| D-04 | **dwell 조절 가능** | 닿음 조건이 `DwellSeconds` 동안 끊기지 않아야 닿음으로 바뀐다. 0이면 즉시 |
| D-05 | **닿는 동안만 material 변경** | 떨어지면 바로 원래 material로 돌아간다. 누적·완료 상태 없음 |
| D-06 | **다른 면 통과는 따지지 않음** | Door trim·IP를 통과해 닿아도 닿음 |
| D-07 | **기록 없음, 화면 확인만** | 평가 결과 파일·로그를 만들지 않는다 |
| D-08 | **Door 전환 = 진행자 키보드 + 3D widget 버튼** | 같은 명령. 전환하면 target 상태를 초기화한다 |
| D-09 | **두 Door와 IP는 같은 차 원점** | CAD export 원점이 같다. 같은 레벨에 두고 켜고 끈다 |
| D-10 | **관전(TV) 화면 없음** | |
| D-11 | **프로젝트** | `C:\Git\ReachStudy`, GitHub `bssani/ReachStudy` **public**. UE 5.7.4, Meta XR·ISDK 1.205.0(vBoR과 같은 버전, git에는 넣지 않음) |
| D-12 | **작업 방식 = vBoR과 같음** | Claude 명세·리뷰, Codex 앱 구현, 사용자 에디터·Quest 시험 |

## 3. 열린 결정

| ID | 질문 | Claude 제안 |
|---|---|---|
| **Q-01** | **정렬 코드를 어떻게 가져오나** | vBoR 쪽에서 이미 "Calibration을 재사용 플러그인으로 빼기"를 논의 중이다(vBoR `Docs/Design/2026-11-20-calibration-plugin-handoff.md`). 그 문서의 성공 기준 후보가 "차량·시나리오 없는 작은 별도 프로젝트에서 보정 → 저장 → 재시작 후 복원"인데, **ReachStudy가 바로 그 프로젝트다.** <br>**A (권장): vBoR에서 Calibration 플러그인을 먼저 추출하고 ReachStudy는 그 플러그인을 쓴다.** 코드가 하나라 두 프로젝트가 갈라지지 않는다. vBoR 쪽 작업이 먼저 필요하다. <br>B: 필요한 파일을 ReachStudy로 복사한다. 빠르지만 vBoR 수정이 ReachStudy에 따라오지 않아 곧 두 벌이 된다. 특히 **회사 PC의 vBoR 코드가 이 PC와 다르다**(인계 문서 §1)는 점 때문에 복사본이 어느 쪽 기준인지부터 흐려진다 |
| Q-02 | 회사 vBoR의 정렬 코드가 이 PC `main@1a17da9`와 다른가 | 다르면 추출 기준을 회사 쪽으로 해야 한다. 회사에서 `Source/vBoR/Interaction/VBoR*Alignment*`, `VBoRBuckProfiles.cpp`, `M1Probe/M1Marker*` 의 `git log`/diff만 확인하면 된다 |
| Q-03 | `Fingers`의 정의 | 제안: 다섯 손가락의 **모든 관절점**(끝·마디). `WholeHand` = 그것 + 손바닥·손목 관절. 관절 사이(마디 중간)는 점이 없으므로 허용 거리로 메운다. 캡슐(뼈 길이) 판정은 S2 결과를 보고 필요하면 넣는다 |
| Q-04 | 설정 위치 | 제안: 레벨에 하나 두는 `AReachStudySettings`(또는 게임 모드)에 D-02~D-04 값을 모은다(target마다 따로 두지 않음 — 한 곳만 고치면 되게). target별 덮어쓰기는 필요해지면 |
| Q-05 | 3D widget 버튼 자리 | 참가자 reach 영역 밖(예: 조수석 쪽 대시 위). 누가 누르나 — 참가자가 진행자 말을 듣고 누르는지, 진행자가 헤드셋 없이 키보드만 쓰는지 |
| Q-06 | 진행자 화면(PC 창)에 무엇을 보이나 | 제안: 현재 Door(A/B), 정렬 상태, 지금 닿은 target 이름 목록, 손 추적 상태(좌·우). 텍스트만 |

## 4. 허용 거리·dwell 권장 기본값

**허용 거리 `ContactDistanceMm` 기본 10 mm (조절 범위 0–50).**
- 오차 원천: 마커 정렬 점별 XY 1.5–1.8 cm(vBoR M2 집 시험), 광학 손 추적 수 mm–1 cm, CAD target 형상 자체.
- 정렬 오차(~1.5 cm)를 다 덮을 만큼 키우면 "1.5 cm 앞에서 닿음"이 정상 동작이 된다 → reach가 실제보다 길게 나온다. 반대로 0이면 정렬이 안쪽으로 어긋난 쪽 target은 거의 안 닿는다.
- 그래서 **기본은 10 mm로 두고, Buck마다 확인점으로 실제 오차를 보고 조정**하는 것을 권한다. vBoR M1의 확인점 기능(`Align.Check` 태그 점, 손끝 원본과의 차이 표시)을 같이 가져오면 설치 때 "이 Buck은 오차 8 mm" 같은 근거로 값을 정할 수 있다.
- **떨어짐 히스테리시스 `ReleaseMarginMm` 기본 5 mm**: 닿음은 ≤ 10 mm, 떨어짐은 > 15 mm. 경계에서 손이 떨려 색이 깜빡이는 것을 막는다(vBoR 몰린 버튼 규칙과 같은 이유).

**dwell `DwellSeconds` 기본 0.15 s (조절 범위 0–2).**
- 팔을 뻗다 다른 target을 스치는 것(지나감)은 보통 0.1 s 안쪽이다. 0.15 s면 스침은 거르고, 의도한 접촉은 늦게 느껴지지 않는다.
- 떨어질 때는 dwell 없이 즉시 원래 material(D-05, 히스테리시스로 충분).
- 추적이 끊기면 dwell 시간을 0으로 되돌린다.

모든 값은 근거 없는 초기값이다. S2 Quest 실측 뒤 다시 정한다.

## 5. 구성

| 요소 | 내용 |
|---|---|
| `AReachTarget` (C++) → `BP_ReachTarget_<Name>` | target mesh 하나. 상태 `Idle` / `Dwelling` / `Touching`. Touching이면 overlay material 또는 슬롯 material을 바꾸고, Idle로 돌아가면 원래대로. 이벤트 `OnTouchBegin` / `OnTouchEnd`. BP에서는 mesh와 닿음 material만 정한다 |
| 손 관절 읽기 | Pawn에서 추적된 원본 관절 위치(vBoR `AVBoRHandRigPawn::ReadRawIndexTip` / `ReadRawPalmAndTips`와 같은 경로). 신뢰도 낮음·추적 끊김 = 판정 안 함 |
| 닿음 판정 | 관절점마다 target mesh까지의 최소 거리. 방식(Complex collision 거리 / 단순 collision 생성)은 **S2에서 정한다** |
| `AReachVariantSet` (C++) | Door A·Door B 각각의 Actor 목록. 활성 Door만 visible·collision·판정. 전환하면 모든 target 초기화(D-08). IP는 이 목록에 없다 |
| 진행자 입력 | 키보드(키 배치는 S4 명세) + 3D widget poke 버튼. vBoR `BP_LightPanel`(widget poke) 방식 |
| 정렬 | Q-01 결과에 따라 플러그인 또는 이식. 운전석 하나, 시나리오·좌석 전환 없음 |

## 6. 단계

| ID | 명세 | 핵심 | 완료 조건 |
|---|---|---|---|
| **S0** | 프로젝트 부트스트랩 | 빈 C++ 프로젝트, 규칙 문서, Meta 플러그인 복사, git·GitHub | **완료 2026-11-22** (Claude). 빌드 결과는 개정 이력 |
| **S1** | 정렬 가져오기 | Q-01 결과대로. 마커 기록·저장·재시작 복원, 앵커 옵션(D-01) | 자동화 시험 통과, Quest에서 마커 정렬 → 재시작 복원 (T-01) |
| **S2** | 닿음 판정 spike | 시험대 레벨에서 손 관절 ↔ CAD 형상 mesh 거리. collision 방식 비교, 거리·dwell·히스테리시스 실측, 추적 끊김 | Quest 실측 기록, 방식·기본값 결정 (T-02). S1과 병렬 가능(정렬 없이) |
| **S3** | `AReachTarget` | S2 결과로 판정·상태·material 전환, 설정(D-02~D-04, Q-04) | 자동화 시험: 진입·dwell·이탈·히스테리시스·추적 끊김·부위 모드 3종 |
| **S4** | Door 전환 + 진행자 | `AReachVariantSet`, 키보드·3D 버튼, 진행자 창 텍스트(Q-06) | 자동화 시험: 비활성 Door target 판정 안 됨, 전환 시 초기화, 동시 입력 |
| **S5** | Main 통합 | CAD Datasmith import(`Content/CAD/`, git 밖), target BP 배치, `L_Main` | Quest 전체 흐름 (T-03) |

## 7. 시험 (Quest 3, 상태는 전부 `not-tested`)

| T-ID | 내용 |
|---|---|
| T-01 | 마커 정렬 저장 → 에디터·Link 재시작 → 같은 자리. 앵커 켬/끔 |
| T-02 | 검지·손가락·손 전체 각각으로 평면·곡면·작은 target 닿음. 경계에서 깜빡임 없음. 스침 거르기(dwell). 손이 시야 밖(Door 아래쪽)일 때 판정 안 함 |
| T-03 | 운전석에서 Door A → 전환(키보드·3D 버튼) → Door B, 전환 뒤 초기화, IP target 그대로 동작 |

## 8. 위험 — 결과 해석에 같이 적을 것

- **VR reach는 실차보다 낙관적이다.** 손이 Door trim·IP를 통과하고(D-06), 촉감이 없어 지나치기 쉽고, 허용 거리만큼 앞에서 닿는다.
- **Door 쪽 target은 손 추적이 약하다.** 무릎·옆은 고개를 돌리지 않으면 Quest 카메라 시야 밖이다. 추적이 끊기면 "안 닿음"으로 보이므로 진행자 창의 추적 상태와 같이 본다.
- **정렬 오차(~1.5 cm) 아래의 차이는 판정하지 않는다.** target 사이 거리가 그보다 작으면 결과로 구분하지 않는다.
- CAD mesh에는 보통 쓸 만한 collision이 없다(S2).

## 개정 이력

- 2026-11-22 — 초안. 사용자 답 D-01~D-12 반영, Q-01~Q-06 열림.
