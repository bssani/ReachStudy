# S4 Door 전환 — 진행자 키보드와 참가자가 누르는 3D 버튼

| 항목 | 값 |
|---|---|
| 단계 | 계획서 §6 S4. 시험 T-03의 Door 전환 부분 |
| 결정 근거 | 계획서 D-08(진행자 키보드 + 3D widget 버튼, 같은 명령, 전환하면 target 상태 초기화), D-09(두 Door와 IP는 같은 차 원점, 같은 레벨에서 켜고 끔), Q-05(버튼은 참가자가 누름, Door·IP 밖). 사용자 2026-11-22. **개정 1(사용자 2026-11-23): 패널은 이전·다음 버튼 + 지금 Door 이름, Door는 늘어날 수 있음, 전환 뒤 0.5 s 상대 입력 무시, 끝에서 처음으로 돌아감, 키보드는 번호 키 + 이전·다음 키, 공유 actor·잘못된 시작 번호 처리** |
| 베이스 브랜치 | `main` |
| 작업 브랜치 | `codex/S4-door-switch` |
| 리뷰 파일 | `Docs/Reviews/S4-door-switch.md` (리뷰 1회차 뒤 개정 1을 같은 브랜치에서 반영) |
| 에디터 작업 | **Codex는 하지 않는다**(Codex의 에디터 MCP는 vBoR에 연결돼 있다). 구현·리뷰 뒤 Claude가 MCP로 한다(§6) |
| 범위 밖으로 옮긴 것 | **진행자 PC 창(계획서 Q-06)은 S3로 옮긴다.** "지금 닿아 있는 target"은 S3의 제품 target(`UReachTouchComponent`, 계획서 D-13)이 생겨야 제품 코드에서 읽을 수 있다. 지금 만들면 제품 코드가 개발 시험대(`Development/`)를 읽게 된다(S2 §2) |

## 0. 확인한 사실

- **actor 단위 collision을 끄면 component 질의가 바로 "collision 없음"이 된다.** `UPrimitiveComponent::GetCollisionEnabled`는 owner의 `GetActorEnableCollision()`이 false면 `NoCollision`을 돌려준다(`Engine/Source/Runtime/Engine/Private/PrimitiveComponentPhysics.cpp:1564-1572`). S2의 `ReachContact::Query`는 `IsQueryCollisionEnabled()`가 false인 component를 건너뛴다(`Source/ReachStudy/Contact/ReachContactQuery.cpp:58`). 그래서 꺼진 Door의 target은 손이 들어가도 닿지 않는다. component의 collision 설정 자체는 그대로다.
- 엔진 API: `AActor::SetActorHiddenInGame`(`Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:1925`), `SetActorEnableCollision`(`:1929`), `GetActorEnableCollision`(`:1933`), `GetAttachedActors(..., bRecursivelyIncludeAttachedActors)`(`:3558`).
- **3D widget 버튼 = vBoR `BP_LightPanel`과 같은 구성.** ISDK `UIsdkInteractableWidgetComponent`(`Plugins/MetaXRInteraction/Source/OculusInteractionPrebuilts/Public/IsdkInteractableWidgetComponent.h:49`)에 UMG 위젯을 올리고 poke만 켠다. 속성: `bCreatePokeInteractable`(`:209`), `bCreateRayInteractable`(`:230`), `DrawSize`(`:237`), `WidgetScale`(`:244`), `WidgetClass`(`:251`), `GetWidget()`(`:57`). vBoR 값(커밋 `aedcd6c`): DrawSize 420 × 140, WidgetScale 0.06(약 25 × 8 cm), 레이 끔.
- UMG 버튼: `UButton::OnClicked`(`Engine/Source/Runtime/UMG/Public/Components/Button.h:76`), `SetBackgroundColor`(`:129`). C++만으로 위젯을 만들 때는 `UUserWidget::RebuildWidget`(`Engine/Source/Runtime/UMG/Public/Blueprint/UserWidget.h:1568`) 전에 `WidgetTree`(`:1516`)를 채운다.
- **손이 poke를 하려면 Pawn의 poke를 켜야 한다.** S2의 `AReachHandPawn`은 poke를 끈다(`Source/ReachStudy/Hand/ReachHandPawn.cpp:15-29`). ISDK rig의 poke: `GetPokeInteraction()`(`IsdkRigComponent.h:207`). poke limiter: `UIsdkPokeLimiterRigModifier`(`Rig/IsdkPokeLimiterRigModifier.h:43`)를 `RigModifiersToSpawn`(`IsdkRigComponent.h:381`)에 넣는다 — vBoR와 같은 방법(`C:\Git\vBoR\Source\vBoR\Interaction\VBoRHandRigPawn.cpp:58`).

## 1. 결정

| # | 결정 | 내용 |
|---|---|---|
| D1 | **`AReachDoorVariants`** (C++ Actor, `Source/ReachStudy/Door/`) | 레벨에 하나 놓는다. `Variants`(TArray<`FReachDoorVariant`>): 항목마다 `Name`(FText, 화면·버튼 글, 예 `Door A`)과 `Actors`(TArray<TObjectPtr<AActor>>). `bIncludeAttachedActors`(기본 true): 목록의 actor에 붙은 자식 actor까지(재귀) 같은 Door로 본다 — Datasmith CAD는 부모 actor 밑에 mesh actor가 붙어 온다. `InitialVariant`(기본 0). IP 등 어느 Door에도 없는 actor는 건드리지 않는다.<br>- **쓰는 방식(사용자 2026-11-23): Door 하나 = 부모 actor 하나.** 그 밑에 붙은 mesh actor(닿아야 할 target 포함)가 같이 켜지고 꺼진다. Door끼리 actor를 나누지 않는다. Door 수는 2개로 시작해 늘어날 수 있다.<br>- `InitialVariant`가 범위 밖이면 `Warning` 한 줄 + **0번으로 시작**(Door가 하나 이상일 때). Door가 0개면 경고만, 아무것도 바꾸지 않는다 |
| D2 | **명령 세 개, 적용은 한 곳** | 키보드·3D 버튼은 모두 이 셋만 부른다(AGENTS.md "같은 명령").<br>- **`SelectVariant(int32 Index)` (절대 번호):** 지금과 같은 번호 → 아무것도 하지 않는다(초기화·이벤트 없음). 범위 밖 → 무시, `Warning` 한 줄, 상태 그대로. 쿨다운에 막히지 않는다(같은 번호를 여러 번 불러도 결과가 같다).<br>- **`SelectNext()` / `SelectPrevious()` (상대):** 활성 번호 ±1, **끝에서 반대쪽 끝으로 돌아간다**(Door 2개면 둘 다 A↔B). Door가 1개 이하면 아무것도 하지 않는다.<br>- **쿨다운 `SwitchCooldownSeconds`(기본 0.5 s, 프로퍼티, 0이면 끔):** 실제로 Door가 바뀐 뒤(절대·상대 어느 쪽이든) 마지막 변경 뒤 지난 시간이 이 값보다 **작으면 상대 명령은 무시**한다(로그 없음, 같거나 크면 받음). BeginPlay의 시작 적용(D3)은 변경이 아니므로 쿨다운을 시작하지 않는다. 손가락이 판에서 떨려 두 번 눌리거나 키와 버튼이 겹쳐도 두 칸 넘어가지 않게(사용자 2026-11-23).<br>- 바뀌면: §D3대로 적용 → `OnVariantChanged`(dynamic multicast, BlueprintAssignable, 인자 새 번호) 한 번 |
| D3 | **켜고 끄기 = actor 단위 숨김·collision만** | BeginPlay에서 모든 Door actor(D1의 자식 포함)의 **원래** `IsHidden()`·`GetActorEnableCollision()`을 기억한다. 활성 Door의 actor → 원래 값으로. 그 밖의 Door actor → `SetActorHiddenInGame(true)`, `SetActorEnableCollision(false)`. **actor는 활성 Door에 들어 있으면 활성이다**(원래 값). 여러 Door에 들어 있는 actor는 그 Door들 중 하나가 활성일 때만 활성이고, BeginPlay에서 actor마다 `Warning` 한 줄(D1 "Door끼리 actor를 나누지 않는다"를 어긴 신호). component의 material·mesh·collision 설정·transform은 건드리지 않는다(§2). BeginPlay에서 `InitialVariant`를 이벤트 없이 적용한다. EndPlay에서 모든 actor를 원래 값으로 되돌린다 |
| D4 | **진행자 키보드** | `AReachDoorVariants`가 첫 PlayerController 입력을 받는다(S2 시험대와 같은 방식 `EnableInput`). `Z` → 0, `X` → 1, `C` → 2, `V` → 3(그 번호의 Door가 있을 때만, `SelectVariant`). **`N` → `SelectNext`, `B` → `SelectPrevious`.** S2 시험대 키(`M 1 2 3 = - [ ] . , 0 9 R`)와 겹치지 않는다 |
| D5 | **3D 버튼 패널 `AReachDoorPanel`** (C++ Actor, `Source/ReachStudy/Door/`) | 루트 밑에 `UIsdkInteractableWidgetComponent`. 생성자에서 poke 켬, **레이 끔**(AGENTS.md 레이 포인팅 금지), `WidgetClass` = `UReachDoorPanelWidget`, `DrawSize`·`WidgetScale` 기본은 vBoR 값(420 × 140, 0.06)을 시작점으로 프로퍼티. `DoorVariants`(TObjectPtr<`AReachDoorVariants`>, 레벨에서 지정). 비어 있으면 버튼 없이 `No door variants` 글만 보이고 `Warning` 로그 한 줄 |
| D6 | **위젯 `UReachDoorPanelWidget`** (C++ `UUserWidget`, WBP 에셋 없음) | `RebuildWidget` 전에 C++로 트리를 만든다: **가로 한 줄 `[이전 버튼] [지금 Door 글] [다음 버튼]`**(사용자 2026-11-23). 이전 버튼 → `SelectPrevious`, 다음 버튼 → `SelectNext`. 가운데 글은 `<Name> (<번호+1>/<개수>)`, 예 `Door A (1/2)`, `OnVariantChanged`로 갱신. 버튼 글(기본 `<`·`>` — 기본 글꼴에 없는 기호는 쓰지 않는다)·색·글 크기는 영어·프로퍼티. Door별 버튼과 `ActiveColor`/`IdleColor`는 없앤다. Door가 하나뿐이어도 같은 모양(버튼은 D2대로 아무 일 없음) |
| D7 | **손 Pawn에 poke 켜기** | `AReachHandPawn`(S2 D1)에서 **poke interaction만 켠다.** 레이·잡기는 계속 끔. poke limiter를 넣는다(vBoR U-06과 같음 — 버튼을 누를 때 손 모델이 표면에서 멈춰 보인다). **닿음 판정은 바뀌지 않는다**: S2 판정은 XR 원본 관절을 쓰고 손 모델·limiter를 보지 않는다(S2 D2, AGENTS.md) |
| D8 | **S2 시험대와 연결** | `AReachContactProbe`(S2 D7)에 `DoorVariants` 프로퍼티(비면 world의 첫 `AReachDoorVariants`를 쓰고, 없으면 지금과 같이 동작). 연결되면: ① `OnVariantChanged` → 시험대 상태 전부 초기화(지금 `ResetContacts`, 계획서 D-08) ② 헤더에 `door <Name>` ③ 꺼진 Door에 속한 target 줄은 `inactive`로 보이고 판정하지 않는다(상태 `Idle`, overlay 없음). 꺼진 Door 판단은 `AReachDoorVariants::IsActorActive(AActor*)`(어느 Door에도 없으면 true) |
| D9 | **모듈 의존** | `ReachStudy.Build.cs`에 `UMG`, `Slate`, `SlateCore` |

## 2. 지켜야 할 조건

- **상대 명령은 쿨다운으로만 막는다.** 버튼·키를 따로 막는 장치(버튼 비활성화 등)를 더하지 않는다 — 막는 규칙은 D2 한 곳.
- **Door 전환은 actor 단위 숨김·collision만 바꾼다.** component의 material·mesh·collision 설정·transform, 그리고 어느 Door에도 없는 actor(IP)는 건드리지 않는다. 전환해 갔다 돌아오면 숨김·collision이 처음과 같다.
- 시험대(`AReachContactProbe`)는 S2 §2를 그대로 지킨다(바꾸는 것은 overlay 하나). target의 collision을 끄는 것은 시험대가 아니라 `AReachDoorVariants`다.
- 레이·원거리 잡기·잡기는 계속 없다. 위젯 레이(`bCreateRayInteractable`)도 끈다.
- 닿음 판정(S2 `Contact/`)은 바꾸지 않는다. poke limiter는 손 모델 표시만 바꾼다.
- 제품 코드(`Door/`, `Hand/`, `Contact/`)는 `Development/`를 참조하지 않는다. 연결은 시험대 쪽에서 `AReachDoorVariants`를 읽는 방향뿐이다.
- 레벨 로드로 Door를 바꾸지 않는다(AGENTS.md).

## 3. 범위 밖

- 진행자 PC 창(계획서 Q-06) → S3. 제품 target `UReachTouchComponent`(S3), 정렬(S1), CAD(S5).
- Door 전환 연출(암전·소리·애니메이션). 필요하면 BP에서 `OnVariantChanged`에 붙인다(AGENTS.md "BP에는 표현만").
- 버튼 패널의 자리·크기 결정 — 레벨에서 Claude가 놓고(§6) Quest에서 사용자가 본다.

## 4. 경계 상황과 시험 (자동화 `ReachStudy.S4.*`)

엔진 world에 actor를 직접 놓고 시험한다(S2 시험과 같은 방식). 위젯은 `CreateWidget`으로 만들어 버튼의 `OnClicked`를 직접 부른다(손 poke 없이).

| # | 상황 | 기대 |
|---|---|---|
| E1 | BeginPlay, `InitialVariant` 0, Door A·B 각 2 actor, IP 1 actor | A 보임·collision 켬, B 숨김·collision 끔, IP 그대로. 이벤트 없음 |
| E2 | `SelectVariant(1)` | A 숨김·끔, B 원래 값. `OnVariantChanged(1)` 한 번 |
| E3 | 다시 `SelectVariant(1)` | 변화·이벤트 없음 |
| E4 | `SelectVariant(-1)`, `SelectVariant(2)`(Door 2개) | 무시, 상태 그대로, 이벤트 없음 |
| E4b | `InitialVariant` 5(Door 2개) / Door 0개 | 경고 한 줄 + 0번으로 시작(A 켬·B 끔), 이벤트 없음 / 경고만, actor 그대로 |
| E5 | Door A 부모 actor에 자식 2단(손자) 붙음, `bIncludeAttachedActors` true | 손자까지 같이 숨김·끔. false면 목록 actor만 |
| E6 | 한 actor가 A·B 둘 다에 있음 (Door 2개) / Door 3개에서 A·B에만 있음 | 경고 한 줄. Door 2개: 어느 쪽이 활성이어도 원래 값 / Door 3개: A·B 활성 때 원래 값, C 활성 때 숨김·끔 |
| E7 | Door B actor 하나가 레벨에서 원래 숨김 + collision 끔 | B 활성 → 원래 값(숨김·끔) 그대로. A로 돌아가도 같음. EndPlay 뒤 모두 원래 값 |
| E8 | 꺼진 Door의 target 표면에 관절 | S2 `Query` 결과 collision 없음, Enter·Hold 없음(§0) |
| E9 | 시험대 연결, Door A target `Touching` 중 `SelectVariant(1)` | 그 프레임에 시험대 상태 전부 `Idle`, overlay `nullptr`, 다음 실측 전까지 손 이력 없음(S2 D4 "첫 프레임 판정 안 함") |
| E10 | 시험대 연결, 꺼진 Door의 target | 줄이 `inactive`, 상태 `Idle`, overlay 없음 |
| E11 | 시험대에 `DoorVariants` 없음, world에도 없음 | S2와 같이 동작(헤더에 door 줄 없음) |
| E12 | 키 `Z`·`X` → 0·1, Door 2개일 때 `C`·`V`. `N`·`B` | `C`·`V` 무시. `N` = `SelectNext`, `B` = `SelectPrevious` |
| E13 | 쿨다운: BeginPlay 직후 A 활성, `SelectNext`(t=0, 바로 받음) → 0.2 s 뒤 `SelectNext` / 첫 변경 0.6 s 뒤 한 번 더 | 첫 번째만 바뀜(B), 이벤트 1 / 다시 바뀜(A, 돌아감), 이벤트 2 |
| E13b | 같은 순간 키 `X`와 다음 버튼 (A 활성) | `X`로 B, 다음 버튼은 쿨다운으로 무시 → B, 이벤트 1. 순서가 반대(다음 → `X`)여도 B, 이벤트 1 |
| E13c | 쿨다운 안의 `SelectVariant(0)` (B 활성) | 절대 명령은 막히지 않음 → A, 이벤트 |
| E13d | Door 3개(각 호출 사이 쿨다운 지남): 0에서 `SelectPrevious` → 2, 2에서 `SelectNext` → 0 / Door 1개: `SelectNext`·`SelectPrevious` | 끝에서 돌아감 / 변화·이벤트 없음 |
| E14 | 위젯: 다음 버튼 `OnClicked` (A 활성, Door 2개) | `SelectNext` → B, 가운데 글 `Door B (2/2)`. 키 `Z`로 바꾸면 글이 `Door A (1/2)`로 갱신 |
| E15 | 위젯: `DoorVariants` 없음 | 버튼 0개, `No door variants` 글, 크래시 없음 |
| E16 | Pawn: rig 좌·우 | poke interaction 활성, 레이·잡기 비활성, poke limiter modifier 있음(`GetActiveRigModifiers` 또는 `RigModifiersToSpawn`) |
| E17 | Door actor 목록에 `nullptr`·지워진 actor | 건너뜀, 크래시 없음 |

S2의 `ReachStudy.S2.*` 시험은 그대로 통과해야 한다.

## 5. 구현자가 정할 것

- 쿨다운 시각을 재는 방법(실시간 권장 — 일시정지·time dilation에 묶이지 않게). 시험은 실제로 기다리지 않고 시각을 넣을 수 있어야 한다(예: 시각을 인자로 받는 내부 함수).

- 글 크기·색·버튼 크기 기본값(배치는 D6).
- 위젯이 `AReachDoorVariants`를 받는 방법(패널이 위젯 인스턴스에 넘겨주기 등).
- 시험에서 키 입력을 넣는 방법(S2처럼 키 처리 함수를 직접 부르는 것 등).

## 6. 에디터 작업 — Claude가 MCP로 (구현·리뷰 뒤)

`L_S2_ContactProbe`에 더한다(새 레벨을 만들지 않는다):

1. `BP_DoorPanel`(부모 `AReachDoorPanel`, `Content/Development/S4/`) — AGENTS.md "실제 대상은 자식 Blueprint".
2. 가짜 Door 둘: Door A = 지금 `T_KneeSide`(무릎 옆), Door B = 새 `T_KneeSideB`(같은 크기, 조금 다른 자리 — 예 (25, −45, 60)). 둘 다 `ContactProbe.Targets`에.
3. `DoorVariants`(`AReachDoorVariants`) — Variants: `Door A` {`T_KneeSide`}, `Door B` {`T_KneeSideB`}. 시험대 `DoorVariants`에 지정.
4. `BP_DoorPanel` — 조수석 쪽(+Y) Door·IP 밖, 앉아서 손이 닿는 높이, 참가자를 보게. `DoorVariants` 지정.
5. PIE: 키 `Z`/`X`로 `T_KneeSide`/`T_KneeSideB`가 번갈아 보이고 시험대 줄이 `inactive`로 바뀌는지, 경고 없는지. 스크린샷.

## 7. 완료 조건

- §4 시험 전부, `ReachStudy.*` 전체(S2 포함) 한 번 통과. 빌드 non-unity(`-DisableUnity -Module=ReachStudy`)·unity 경고·오류 0.
- 보고 ①~⑤(AGENTS.md). ④에 위 엔진·ISDK 경로:줄을 직접 확인했는지.
- (Claude, §6 뒤) PIE 경고 없음.

## 8. Quest 3에서 볼 것 (T-03 일부, 상태 `not-tested`)

- 다음·이전 버튼을 손가락으로 눌러 Door가 바뀌는지. 한 번 누름에 한 칸만 가는지(두 칸·안 바뀜). 0.5 s 쿨다운이 빠르게 다시 누를 때 답답하지 않은지 — 값은 Quest에서 정한다.
- 가운데 `Door A (1/2)` 글이 앉은 자리에서 읽히는지.
- 누를 때 손 모델이 판 표면에서 멈춰 보이는지(limiter), 그때 S2 시험대 숫자는 실제 손끝을 따르는지.
- 패널 자리·크기가 앉아서 누르기 편한지, Door·IP target과 헷갈리지 않는지.
- 키보드 `Z`/`X` 전환, 전환하면 닿음 표시가 꺼지는지.

## 개정 이력

- 2026-11-23 개정 1 (사용자 결정, 리뷰 1회차 뒤): 패널을 이전·다음 버튼 + 지금 Door 글로(D6), 상대 명령 `SelectNext`·`SelectPrevious`와 끝에서 돌아가기·쿨다운 0.5 s(D2), 키 `N`·`B`(D4), 공유 actor는 활성 Door 소속으로 판단 + 경고(D3, 리뷰 S-1), 잘못된 `InitialVariant`는 경고 + 0번(D1, 리뷰 S-2), Door 하나 = 부모 actor 하나(D1). 시험 E4b·E6·E12–E14·E13b–d.
- 2026-11-23 — 초안 (Claude). 계획서 §5·§6·Q-06, S2 명세 §2·D1과 같이 맞춤(진행자 창 S4 → S3, Pawn poke 켜기, Door 전환이 actor collision을 끄는 것).
- 2026-11-23 — S4b가 바꿀 것(이 명세 구현에는 영향 없음): D8 ② 시험대 헤더 `door <Name>`·E10 `inactive` 줄 글은 시험대 화면 글과 함께 없어지고, D5·E15 패널 안내 글 `No door variants`는 빈 패널로 바뀐다([S4b](S4b-headset-clean.md) D2·D3). 진행자 PC 창은 만들지 않기로 했다(계획서 Q-06).
