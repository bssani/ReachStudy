# S3-touch-component — Codex 구현 결과

- 기준: `main@60b1da2`
- 브랜치: `codex/S3-touch-component`
- 상태: 구현·로컬 빌드·합성 자동화 시험 완료, Claude 리뷰 대기.
- 명세: `Docs/Specs/S3-touch-component.md`
- 에디터·Quest 3 시험: `not-tested`.

## ① 바뀐 파일

| 파일 | 내용 |
|---|---|
| `Source/ReachStudy/Touch/ReachTouchComponent.h/.cpp` | mesh 부모 하나에만 overlay, Blueprint phase·이벤트, BeginPlay 등록·EndPlay 해제 |
| `Source/ReachStudy/Touch/ReachTouchSettings.h` | 레벨 공통 판정 설정·색·Door 참조, 범위 메타데이터 |
| `Source/ReachStudy/Touch/ReachTouchSubsystem.h/.cpp` | Game·PIE 판정, 손별 원본 입력, 넓은 거르기, 지연 이벤트, Door 전환 전체 초기화 |
| `Source/ReachStudy/Contact/ReachContactQuery.h/.cpp` | QueryComponent·mesh overlay·ReadHand·경계 거르기, 기존 actor Query는 component 결과 합산 |
| `Source/ReachStudy/Hand/ReachHandPawn.cpp` | ReadHand를 Contact 함수로 위임 |
| `Source/ReachStudy/ReachGameMode.h/.cpp` | 제품 GameMode, 기본 Pawn은 ReachHandPawn |
| `Source/ReachStudy/Tests/ReachTouchTests.cpp` | S3 E1–E16을 묶은 자동화 시험 9개 |
| `Source/ReachStudy/Tests/ReachTouchTestObserver.h` | dynamic 이벤트 횟수·삭제 관찰용 시험 객체 |
| `Docs/Reviews/S3-touch-component.md` | 이 보고 |

`HANDOFF.md`의 Codex 결과도 갱신했다(git 무시). 기존 S2·S4·S4b 시험, 에셋, Config, 플러그인 파일은 바꾸지 않았다.

## ② 완료 조건별 충족 여부

| 완료 조건 | 결과 |
|---|---|
| D1·D2: mesh별 component, 부모 검증·중복 경고, phase·overlay·이벤트 | 구현. E1·E3·E6·E9·E15 통과 |
| D3: 공통 설정, 없을 때 기본값·여럿일 때 첫 Actor, 경고 한 번, 실시간 값 적용 | 구현. E5·E6·E7·E10 통과 |
| D4·D5: component 판정·Pawn 밖 원본 손 읽기, S2 유지 | 구현. E13과 기존 S2 9개 통과 |
| D6·D7: Game·PIE, 일괄 갱신 뒤 이벤트, 결과를 유지하는 거르기 | 구현. E11·E12·E16 통과. E12는 일반/큰 simple body × 반지름 켬/끔 × 격자 180점(총 720 조합)에서 두 방식 비교 |
| D8: Door 전환 즉시 Door·IP 상태·손 이력 초기화 | 구현. E8에서 붙은 자식 Actor 포함, End 한 번·다음 첫 실측 보류·비활성 Door 제외 확인 |
| D9·D10: 화면 글 없음, 일반 Pawn 로그, 제품 GameMode | 구현. E14·E16 및 기존 S4b 4개 통과 |
| D11: S2 dwell 누적 규칙 유지 | FReachContactState 수정 없음. E1·E2·E4와 기존 S2 상태 시험 통과 |
| §4 전체 E1–E16 | 신규 S3 시험 9개 전부 통과 |
| §7 non-unity·unity 빌드 | 둘 다 Result: Succeeded, warning/error 진단 0 |
| §7 ReachStudy.* 전체 | 33/33 Success: S2 9 + S3 9 + S4 11 + S4b 4. TEST COMPLETE. EXIT CODE: 0 |
| 보고 ①–⑤·API 근거 | 이 파일 및 이 작업 마지막 커밋 본문에 기록 |
| Claude §6 이후 PIE 경고 없음 | 대기. Codex는 에디터 작업을 하지 않음 |

최종 근거(git 무시):

- `Saved/Logs/S3-build-nonunity-final.log` — `-DisableUnity -Module=ReachStudy`, 성공.
- `Saved/Logs/S3-build-unity-final.log` — 일반 unity 빌드, 성공.
- `Saved/Logs/S3-automation-final.log` — 전체 33개 Success, exit 0.

빌드 로그에는 문서화된 로컬 MCP 플러그인의 StructUtils 폐기 예정 안내가 있다. warning/error 진단은 없으며 UHT의 `-WarningsAsErrors` 옵션 문자열은 진단으로 세지 않았다. 자동화 시작 시의 OpenXR·ISDK 환경 메시지는 기기 확인 결과가 아니다.

## ③ 명세와 다르게 한 것과 이유

기능 범위 변경 없음. 명세에 세부 방식이 없던 처리는 다음과 같다.

- 등록 목록은 weak component 배열, 상태·손 이력은 각 component의 기존 FReachContactState에 둔다. 이벤트 큐도 weak 참조로 만들고 변경 번호를 비교해 삭제·Door 재초기화 뒤 오래된 이벤트를 보내지 않는다(D6·D8).
- 설정 검색과 설정/Pawn 경고는 target이 등록된 뒤 첫 판정 프레임에 한 번 수행한다. target이 없는 S2·S4 레벨은 불필요한 S3 경고를 만들지 않는다. 선택한 설정 Actor의 판정값·색·Door 참조는 매 프레임 읽는다.
- 거르기 함수는 Contact에 둬서 subsystem과 E12 시험이 같은 경로를 쓴다. 판정 자체는 항상 QueryComponent의 S2 계산이다.
- 기본 material은 subsystem의 공용 MID 하나로 만들고 ColorParam을 매 프레임 갱신한다. TouchMaterial이 지정되면 그 material을 직접 쓴다.
- 첫 전체 시험에서 E12·E14가 실패했다. E12는 mm→cm float 환산이 상자를 미세하게 좁혀 정확한 경계를 거른 문제여서 거르기 환산만 double로 바꿨다. 접촉 거리·여유를 추가하지 않았다. E14는 실제 경고가 나왔지만 예상 메시지의 괄호를 정규식으로 처리해 못 찾은 시험 문제였다. 문자열 매칭을 고쳤다.
- 실패 원인 확인에 E12 한정 시험을 한 번 추가했고, 수정 후 두 빌드·전체 시험을 다시 실행했다. 최초 실패 로그는 `Saved/Logs/S3-automation-first.log`, 진단 로그는 `Saved/Logs/S3-diagnostic.log`에 남겼다. 처음 명령의 따옴표 없는 `-log=`가 PowerShell에서 잘못 전달되어 실제 기본 로그를 보존한 것이다. 최종 명령은 인자 전체를 따옴표로 감쌌다.

## ④ 직접 확인한 엔진·플러그인 API

아래 경로의 엔진 루트는 `C:/Program Files/Epic Games/UE_5.7/Engine/`이다. 기억으로 API를 추정한 항목은 없다.

| API | 직접 확인한 헤더 경로:줄 |
|---|---|
| GetHandTrackingState | `Plugins/Runtime/XRBase/Source/XRBase/Public/HeadMountedDisplayFunctionLibrary.h:352` |
| const WorldContext에서 UWorld* 얻기 | `Source/Runtime/CoreUObject/Public/UObject/Object.h:719` — GetWorld() const. 엔진 추적 API는 UObject*를 받으므로 이 world를 전달 |
| GetBodyInstance·GetSquaredDistanceToCollision | `Source/Runtime/Engine/Classes/Components/PrimitiveComponent.h:2248`, `:2271` |
| IsQueryCollisionEnabled·Bounds | `Source/Runtime/Engine/Classes/Components/SceneComponent.h:1265`, `:129` |
| GetBodyBounds·OverlapTest | `Source/Runtime/Engine/Classes/PhysicsEngine/BodyInstance.h:927`, `:1342` |
| GetOverlayMaterial·SetOverlayMaterial | `Source/Runtime/Engine/Classes/Components/MeshComponent.h:52`, `:56` |
| UTickableWorldSubsystem·DoesSupportWorldType·Deinitialize | `Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h:75`, `:61`, `:92` |
| MID 생성·색 변경 | `Source/Runtime/Engine/Public/Materials/MaterialInstanceDynamic.h:176`, `:109` |
| material 색 읽기(시험) | `Source/Runtime/Engine/Public/Materials/MaterialInterface.h:978` |
| DefaultPawnClass | `Source/Runtime/Engine/Classes/GameFramework/GameModeBase.h:108` |
| ISDK 기본 material 경로 | 리포 `Plugins/MetaXRInteraction/Source/OculusInteraction/Public/IsdkContentAssetPaths.h:137` — GrabbableDefault |

ColorParam은 S2의 `Development/ReachContactProbe.cpp:62`와 같은 material parameter를 쓴다(제품 코드에서 Development를 참조하지 않음). E6에서 MID의 실제 값과 실시간 변경을 확인했다.

## ⑤ 로컬에서 확인할 것

- 빌드·자동화: 위 최종 로그로 확인 완료. Claude가 리뷰 때 재현한다.
- Claude: S3 §6의 별도 시험 레벨·BP·설정·제품 GameMode 구성, S4 §6 패널 연결, PIE 경고와 배치 확인. **아직 not-tested**.
- 사용자: Quest 3 T-02 제품 경로 — BP의 부모 mesh 하나만 색 변경, 부위·거리·dwell·추적 보류, target 수에 따른 프레임 확인. **not-tested**.
- 사용자: Quest 3 T-03 — 키보드·poke Door 전환, Door·IP 초기화와 재접촉. S4b §7 화면 글 없음도 함께 확인. **not-tested**.
- T-01 정렬은 S3 범위 밖이다.

## 질문

없음.

## 리뷰

Claude 리뷰 대기. 이 보고는 리뷰 통과 판정이 아니다.
