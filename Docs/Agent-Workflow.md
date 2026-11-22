# 에이전트 작업 방식

Codex·Claude·사용자가 이 리포에서 어떻게 일하는지 적는다. 규칙은 [AGENTS.md](../AGENTS.md), 이 문서는 절차다. vBoR 리포의 같은 문서에서 ReachStudy에 필요한 것만 옮겼다.

작성 2026-11-22 (Claude).

## 1. 역할과 흐름

| 누가 | 하는 일 | 못 하는 일 |
|---|---|---|
| **Claude** (Claude Code, 로컬) | 명세, 코드 리뷰, 조사, 문서·리포 설정, 빌드·시험 재현 | 요청이 없으면 제품 C++를 쓰지 않는다 |
| **Codex** (데스크톱 앱, 로컬 Windows) | 명세대로 구현, 로컬 빌드, 자동화 시험, 커밋 | 명세 밖 기능, push·`main` 머지 |
| **사용자** | 결정, 에디터 작업(Blueprint·에셋·레벨·Project Settings), Quest 3 시험, push·머지 승인 | — |

1. Claude가 `Docs/Specs/<명세>.md`를 쓰고 `main`에 넣는다. `HANDOFF.md`의 "지금 할 일"을 갱신한다.
2. 사용자가 Codex 앱에 보낸다: `HANDOFF.md 읽고 '지금 할 일' 진행해.`
3. Codex가 `codex/<명세>` 브랜치에서 구현·커밋한다(보고 ①~⑤는 커밋 본문). `HANDOFF.md` "Codex 결과"를 채운다.
4. 사용자가 Claude에 보낸다: `Codex 끝났어. 리뷰해.` Claude는 `git diff main...codex/<명세>`를 읽고 빌드·시험을 직접 다시 돌린다. 결과는 `Docs/Reviews/<명세>.md`에 회차로.
5. 리뷰 통과 + 로컬 빌드 확인 뒤 사용자 승인으로 PR·머지.

- **작업 폴더 하나를 둘이 같이 쓴다.** 한 번에 한쪽만. 작업 전에 `git branch --show-current`, `git status`.
- 명세가 모호하면 구현자는 멈추고 묻는다.
- Claude가 코드를 쓰면 리뷰는 Codex가 한다. 쓴 쪽이 자기 코드를 리뷰하지 않는다.

## 2. Unreal 작업 절차

### 2.1 시작 전

- AGENTS.md·CLAUDE.md, [Plan.md](Plan.md), 해당 명세를 읽는다.
- 에디터가 켜져 있는지: `tasklist /FI "IMAGENAME eq UnrealEditor.exe"`. **켜져 있으면 빌드·브랜치 전환·머지를 하지 않는다.**
- 엔진: `C:\Program Files\Epic Games\UE_5.7` (5.7.4).
- `Plugins/MetaXR`, `Plugins/MetaXRInteraction`이 있는지(git에 없다, README "Setup").

### 2.2 API는 기억이 아니라 소스로

- 플러그인 API는 `Plugins/` 아래 헤더, 애매하면 `.cpp`까지. 엔진 API는 `UE_5.7/Engine/Source/`.
- 근거는 `경로:줄`로 남긴다. 명세·커밋 보고·리뷰 모두.
- 이름·주석을 믿지 않는다(예: MetaXR 새끼손가락 키 문자열은 `Pink`, vBoR 조사 기록).

### 2.3 빌드

에디터를 닫고 백그라운드로 실행한다.

```
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ReachStudyEditor Win64 Development -Project="C:\Git\ReachStudy\ReachStudy.uproject" -WaitMutex
```

- 통과 기준: `Result: Succeeded`, 로그에 `warning`·`error` 0(UHT 인자 `-WarningsAsErrors`는 제외).
- non-unity(빠진 include 찾기): `-DisableUnity -Module=ReachStudy`를 붙인다.
- 처음 빌드는 MetaXR+ISDK까지 컴파일한다(약 100 s, 2026-11-22).

### 2.4 자동화 시험

- 판정 로직은 엔진 없이 돌릴 수 있는 구조체로 분리하고 `WITH_DEV_AUTOMATION_TESTS` 시험을 붙인다. 손 추적 입력도 합성 입력으로 시험한다.

```
UnrealEditor-Cmd.exe "C:\Git\ReachStudy\ReachStudy.uproject" -ExecCmds="Automation RunTests ReachStudy.<접두어>;Quit" -unattended -nullrhi -nosplash -nosound -log=<이름>.log
```

- 결과: `Saved/Logs/<이름>.log`의 `Result={Success}`, `TEST COMPLETE. EXIT CODE: 0`. `-log=`에는 파일 이름만(경로를 넣으면 안 생김).
- 합성 시험 통과 ≠ 기기 시험 통과. Quest 3 확인 전까지 `not-tested`.

### 2.5 맵 로드 확인

- GUI 에디터로 맵을 연다(headless `-game -nullrhi`는 맵 로드 전 크래시로 판단 불가, vBoR 경험).
- 무시해도 되는 로그: 프로파일러·태블릿 DLL 로드 실패, MetaXR Content `empty engine version`, 헤드셋 미연결 `xrGetSystem() failed`, OpenXR API 1.1 → 1.0 폴백.
- Meta XR 설문 창이 모달로 떠 종료를 막는다. 에이전트는 대신 답하지 않고 강제 종료한 뒤 `git status`를 본다.

### 2.6 커밋·push

- 브랜치에서 작업. 커밋 본문에 보고 ①~⑤.
- **public 리포:** push 전에 diff에 CAD·회사·프로그램 정보, `Content/CAD/`, Meta 플러그인 파일이 없는지 본다.
- push·PR·`main` 머지는 사용자 승인 후.

### 2.7 에이전트가 못 하는 것 — 사용자 몫

- Quest 3 착용 시험, VR Preview
- Blueprint·레벨·에셋 편집(`.uasset`은 바이너리), Project Settings, Meta XR Project Setup Tool

## 3. 기록 체계

| 무엇 | 어디 |
|---|---|
| 범위·결정·단계·시험 | [Plan.md](Plan.md) |
| 작업 단위 | `Docs/Specs/<ID>-<이름>.md` |
| 리뷰 회차 | `Docs/Reviews/<명세 이름>.md` |
| 지금 할 일(휘발) | `HANDOFF.md` (git 무시) |
