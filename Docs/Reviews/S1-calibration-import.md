# S1 정렬 가져오기 — Codex 구현 보고

- 명세: `Docs/Specs/S1-calibration-import.md`.
- 브랜치: `codex/S1-calibration-import`, 베이스 `main@8ab1f92`.
- 키트 출처: `bssani/VRCalibrationKit main@e9156478f4ebb35bc25736a3be3171c83087bcad`.
- 상태: 구현·로컬 빌드·합성 자동화 시험 완료. Claude 리뷰 대기(2026-11-24).

## ① 바뀐 파일

- `Plugins/VRCalibrationKit/` — 지정된 `git archive`로 가져온 27개 소스·헤더·모듈 설정·fixture. 키트 파일 수정 없음.
- `ReachStudy.uproject` — `VRCalibrationKit`, `OpenXR` 활성화.
- `Config/DefaultEngine.ini` — 기존 Meta XR 절에 앵커·카메라 지원 두 값 추가.
- `Source/ReachStudy/ReachGameMode.h`, `.cpp` — actor BeginPlay 전에 Stage, `WorldToMeters = 100`, 지정된 로그.
- `Source/ReachStudy/ReachCalibrationGameMode.h` — 부모 동작을 그대로 쓰는 정렬 전용 GameMode.
- `Source/ReachStudy/Screen/ReachScreenMessageSubsystem.cpp` — 정렬 GameMode world에서는 화면 메시지 값을 바꾸지 않고 종료 시에도 복원하지 않음.
- `Source/ReachStudy/ReachStudy.Build.cs` — private 의존에 `VRCalibrationCore` 추가.
- `Source/ReachStudy/Tests/ReachCalibrationTestObserver.h`, `ReachCalibrationTests.cpp` — E1–E6, 새 자동화 시험 6개.
- 이 보고서. `HANDOFF.md`의 Codex 결과는 git 밖에서 갱신.

## ② 완료 조건별 결과

| 조건 | 결과 |
|---|---|
| E1 | 통과. 실제 world의 GameMode 시작 경로에서 actor BeginPlay가 50 대신 100을 봄 |
| E2 | 통과. 정렬 GameMode의 부모·기본 손 Pawn 확인, actor BeginPlay가 100을 봄 |
| E3 | 통과. Game·PIE에서 초기 true/false 모두 시작·종료 후 보존. 다른 곳이 중간에 바꾼 값도 종료 시 덮어쓰지 않음 |
| E4 | 통과. Game·PIE의 일반 GameMode·GameMode 없음, 초기 true/false 모두 비활성화·복원 |
| E5 | 통과. 실제 `AReachHandPawn`에 키트 public 초기화 API 적용. `Applied`, 차에 부착, 상대 위치 `(120,-80,0)` cm 오차 0.1 cm 이내, yaw 90도 |
| E6 | 통과. 같은 Stage 저장값에 현재 Local 원점을 주면 `NeedsCheck`, 미부착, Pawn transform 보존 |
| E7 | `VRCalibrationKit.*` 발견·완료·성공 46/46, 실패 0, 프로세스 exit 0 |
| E8 | `ReachStudy.*` 발견·완료·성공 39/39, 실패 0, 프로세스 exit 0. 기존 33개는 수정하지 않음 |
| non-unity | 최종 수정 후 성공, 경고·오류 0. `-DisableUnity -Module=ReachStudy` |
| unity | 최종 수정 후 실제 `Module.ReachStudy.cpp` 컴파일 확인, 성공, 경고·오류 0. 키트 두 모듈도 최초 unity에서 컴파일 성공 |
| 키트 archive 동일 | 27/27 바이트 일치, 추가 소스·fixture 0개 |
| 화면 출력 호출 없음 | `rg -n 'AddOnScreenDebugMessage\|AddToViewport\|PrintString' Source` 결과 0건(exit 1) |
| 보고·API 근거 | 이 파일 및 이 작업 마지막 커밋 본문에 같은 보고 기록 |

최종 시험은 `Config/UserEngine.ini`가 없는 상태에서 실행했다. 생성하지 않았다. 두 시험 실행의 첫 시험 시작부터 완료까지 `Error:` 0건이며, 시작 전에는 기존 ISDK 샘플 `InfoCardWidget` CDO 오류 2건과 OpenXR 환경 메시지가 있다. 전체 시작 로그가 오류 없이 깨끗했다는 뜻은 아니다. 기기 증거로 해석하지 않는다.

### 명령·로그

- non-unity: `Build.bat ReachStudyEditor Win64 Development -Project=C:\Git\ReachStudy\ReachStudy.uproject -WaitMutex -DisableUnity -Module=ReachStudy`.
- unity: 같은 target·project에 `-WaitMutex -ForceUnity`. git 무시 `Saved/UnrealBuildTool/BuildConfiguration.xml`에서 임시로 `bForceUnityBuild=true`, `bUseAdaptiveUnityBuild=false`를 적용한 뒤 원본 바이트 복원. SHA-256 원본·복원 일치(`Saved/Logs/S1-buildconfiguration-restore-final.log`).
- 자동화: `UnrealEditor-Cmd.exe C:\Git\ReachStudy\ReachStudy.uproject "-ExecCmds=Automation RunTests ReachStudy.;Quit" -unattended -nullrhi -nosplash -nosound -NoLiveCoding "-abslog=C:\Git\ReachStudy\Saved\Logs\S1-automation-reachstudy-final.log"`. 키트는 접두어 `VRCalibrationKit.`와 별도 로그를 사용.
- `Saved/Logs/S1-build-nonunity-final.log` — 수정 후 성공.
- `Saved/Logs/S1-build-unity-final.log` — 수정 후 실제 unity 성공. 최초 키트 컴파일 로그는 `S1-build-unity.log`.
- `Saved/Logs/S1-automation-reachstudy-final.log` — 39/39, `TEST COMPLETE. EXIT CODE: 0`.
- `Saved/Logs/S1-automation-kit-final.log` — 46/46, `TEST COMPLETE. EXIT CODE: 0`.
- `Saved/Logs/S1-kit-archive-compare.log`, `S1-kit-staged-compare.log`, `S1-screen-output-scan.log` — archive 비교·스테이징 blob 비교·화면 호출 검색 결과.

키트 복사 명령:

```powershell
git -C C:/Git/VRCalibrationKit archive --format=tar -o C:/Git/ReachStudy/Saved/S1-kit.tar main Plugins/VRCalibrationKit
tar -xf Saved/S1-kit.tar
```

비교 명령(archive의 파일별 원본 바이트와 설치 파일을 직접 비교, 생성된 Binaries·Intermediate 제외):

```python
from pathlib import Path
import tarfile
with tarfile.open('Saved/S1-kit.tar') as archive:
    files = [member for member in archive.getmembers() if member.isfile()]
    mismatches = [m.name for m in files
                  if archive.extractfile(m).read() != Path(m.name).read_bytes()]
    names = {m.name for m in files}
extras = [p for p in Path('Plugins/VRCalibrationKit').rglob('*')
          if p.is_file() and not {'Binaries', 'Intermediate'}.intersection(p.parts)
          and p.as_posix() not in names]
print(len(files), len(mismatches), len(extras))  # 27 0 0
```

커밋할 파일도 `git -C C:/Git/VRCalibrationKit ls-tree -r main -- Plugins/VRCalibrationKit`의 각 blob을 `git rev-parse :<경로>`와 비교해 27/27 일치를 확인했다. Meta 플러그인·CAD·생성 바이너리·개인 설정은 스테이징에 없다.

## ③ 명세와 다르게 한 것과 이유

- 제품 동작·범위의 차이 없음. 정렬 GameMode는 별도 `.cpp` 없이 헤더에 둠(동작 추가 없는 표시용 자식, §5 재량).
- E1–E4는 test world에 `UGameInstance`·WorldSettings GameMode를 설정하고 엔진의 `SetGameMode` → `InitializeActorsForPlay` → `BeginPlay`를 사용한다. E5–E6은 저장 파일·키트 private 시험 헬퍼 없이 메모리의 합성 저장값과 public `InitializeAlignment`만 쓴다.
- 처음 전체 시험은 E1 fixture 종료에서 진행되지 않아 해당 시험 프로세스만 종료했다(exit -1). actor별 `RouteEndPlay` 도중 Mass subsystem이 시각화 actor를 계속 다시 만들었기 때문이다. 실행 중 호출 경로와 엔진 `MassRepresentationSubsystem.cpp:456-491`에서 확인했다. fixture를 `UWorld::EndPlay`로 종료하도록 수정했고 제품·키트 코드는 고치지 않았다. 처음 로그 `Saved/Logs/S1-automation-reachstudy.log`, 진단 `S1-first-run-diagnosis.log`. 수정 뒤 전체 시험 두 묶음은 위 최종 로그로 모두 통과.
- 빌드·전체 시험은 작업 끝에 한 번이라는 규칙에 따라 별도 실패 선행 실행은 하지 않고 시험을 먼저 작성했다. 위 fixture 오류 때문에 필요한 재빌드·전체 시험 재실행이 있었다.
- 작업 도중 다른 프로젝트 Unreal Editor가 열렸다. `Docs/Agent-Workflow.md` §2.1에 따라 후속 unity 재빌드를 중단하고 종료를 요청했다. 이후 해당 에디터가 닫힌 것을 확인해 최종 unity 재빌드까지 완료했다. 에디터를 임의로 닫지 않았다.
- 첫 archive 확인은 raw git blob hash와 archive 바이트를 비교해 27개 차이를 잘못 보고했다. 시스템 `core.autocrlf=true`로 archive가 CRLF를 사용하기 때문이다. 비교 대상을 실제 지정 archive로 바로잡아 27/27 바이트 일치를 확인했다. 키트 파일은 고치지 않았다.

## ④ 직접 확인한 API 근거

엔진 경로는 `C:/Program Files/Epic Games/UE_5.7/Engine/` 기준. 키트 경로는 `Plugins/VRCalibrationKit/` 기준. API를 기억으로 추정한 항목 없음. 명세 §0의 핵심 연결 조건은 아래 소스·헤더를 직접 확인했다.

| 항목 | 직접 확인한 경로:줄 |
|---|---|
| StartPlay override·기본 Pawn | 엔진 `Source/Runtime/Engine/Classes/GameFramework/GameModeBase.h:159`, `:108` |
| cm 설정 | 엔진 `Source/Runtime/Engine/Classes/GameFramework/WorldSettings.h:560` |
| Stage 설정·장치 없음 처리 | 엔진 `Plugins/Runtime/XRBase/Source/XRBase/Public/HeadMountedDisplayFunctionLibrary.h:192`, `Private/HeadMountedDisplayFunctionLibrary.cpp:231-236` |
| GameMode 생성·subsystem과 actor 시작 순서 | 엔진 `Source/Runtime/Engine/Private/World.cpp:5823-5827`, `:6058-6065`; `Private/GameModeBase.cpp:204-206` |
| 시험 world API | 엔진 `Source/Runtime/Engine/Classes/Engine/World.h:3835`, `:4265`, `:3863`; `Private/GameInstance.cpp:1522`, `:1606-1611` |
| 안전한 시험 world 종료 | 엔진 `Source/Runtime/Engine/Private/World.cpp:6080-6105`; `Plugins/Runtime/MassGameplay/Source/MassRepresentation/Private/MassRepresentationSubsystem.cpp:456-491` |
| Meta 설정 | `Plugins/MetaXR/Source/OculusXRHMD/Public/OculusXRHMDRuntimeSettings.h:194`, `:206` |
| 키트 모듈·의존 | 키트 `VRCalibrationKit.uplugin:10-16`; `Source/VRCalibrationCore/VRCalibrationCore.Build.cs:8-9`; `Source/VRCalibrationMarker/VRCalibrationMarker.Build.cs:8-9` |
| 키트 public 초기화·상태·점 | 키트 `Source/VRCalibrationCore/Public/VRCalAlignmentComponent.h:16`, `:43`; `Public/VRCalAlignPointComponent.h:14`; `Public/VRCalAlignment.h:5-33` |
| Pawn 부착·cm·yaw 조건 | 키트 `Source/VRCalibrationCore/Private/VRCalAlignmentComponent.cpp:20-26`, `:92-96`, `:122-132`; 엔진 `Source/Runtime/Engine/Private/Pawn.cpp:97` |
| 키트 샘플 Stage 순서 | 원본 키트 리포 `Source/VRCalibrationKit/VRCalSampleGameMode.cpp:10-16` |
| 키트 정렬 도구·화면 출력 | 키트 `Source/VRCalibrationMarker/Private/VRCalMarkerCalibrator.cpp:77-78`, `:102-105`, `:508-510`, `:643-646` |
| 정렬 점 숨김 | 키트 `Source/VRCalibrationCore/Private/VRCalAlignPointComponent.cpp:20` |
| 앵커 기본 0·상태 로그 | 키트 `Source/VRCalibrationCore/Private/VRCalAnchorAlignment.cpp:18`, `:107`, `:219`, `:226` |
| fixture 탐색 경로 | 키트 `Source/VRCalibrationCore/Private/Tests/KitBoundaryTests.cpp:26` |
| 손 world 좌표·Pawn root 연결 | `Source/ReachStudy/Contact/ReachContactQuery.cpp:53-54`; 엔진 `Plugins/Runtime/XRBase/Source/XRBase/Private/XRTrackingSystemBase.cpp:306-309`; `Plugins/Runtime/OpenXR/Source/OpenXRHMD/Private/OpenXRHMD.cpp:3919` |
| 기존 손 Pawn 조건 | `Source/ReachStudy/Hand/ReachHandPawn.cpp:47-50` |

## ⑤ 로컬에서 확인할 것

- 빌드·자동화: 위 최종 로그로 확인 완료. Claude가 리뷰 때 재현한다.
- Claude: 명세 §6의 MCP 작업·PIE. `BP_S1Car`, `L_S1_Calibrate`, `L_S3_Touch` 배치·메시지 예외·저장 없음 동작은 **not-tested**. Codex는 에디터·에셋 작업을 하지 않았다.
- 사용자: Quest 3 T-01(§8 마커 정렬 → 앵커 복원 → Link 재연결 후 재측정), T-02·T-03과 손 위치·접촉·메시지 없는 참가자 레벨 확인은 **not-tested**. `Config/UserEngine.ini`의 앵커 켜기는 시험 PC에서 사용자가 한다.
- 키트의 이전 기기 결과는 ReachStudy 기기 통과 증거가 아니다. Claude 리뷰·push·PR·merge는 하지 않음.

## 질문

- 없음.

---

# Claude 리뷰 1회차 (2026-11-24, `7c4ddbe`) — **통과**

## 확인한 것

- **AGENTS.md 제약:**
  - Meta 플러그인·CAD·`Saved/`·`UserEngine.ini`가 커밋에 없다.
  - `Source/`에 화면 출력 호출 0건, Pawn 이동·부착 호출 0건(grep 직접).
  - 코드 문구는 영어다.
- **키트 동일성:** 키트 `main`의 `ls-tree -r Plugins/VRCalibrationKit`와 브랜치의 같은 tree가 blob 해시까지 같다(27/27). 키트 파일 수정 없음(D1).
- **범위:** 바뀐 파일이 명세 D1–D7과 같다(uproject·ini 두 줄, `StartPlay`, 정렬 GameMode, S4b 예외 5줄, Build.cs private 의존, 시험). 명세 밖 기능 없음.
- **D3 순서:** `WorldToMeters`·Stage → 로그 → `Super::StartPlay()`. E1·E2가 실제 `SetGameMode` → `InitializeActorsForPlay` → `BeginPlay` 경로에서 actor BeginPlay가 100을 보는 것으로 확인한다.
- **D5:**
  - `OnWorldBeginPlay`에서 `GetAuthGameMode<AReachCalibrationGameMode>()`면 바로 돌아간다. `bMessagesDisabled`가 false로 남아 `Deinitialize`가 되돌리지 않는다.
  - E3의 "중간에 바뀐 값을 끝날 때 덮지 않음" 검사가 좋다.
- **E5:** 합성 점이 90° 회전 + 이동이라 맞춘 상대 위치 `(120,-80,0)`, yaw 90°가 손으로 계산한 값과 같다.
- **재현:** Claude가 다시 해 봤다.
  - 빌드: non-unity(`-DisableUnity -Module=ReachStudy`)·unity(`Module.ReachStudy.cpp`, 임시 BuildConfiguration 뒤 원본 복원 `cmp` 일치) 모두 경고·오류 0.
  - 시험: `ReachStudy.*` 39/39, `VRCalibrationKit.*` 46/46, exit 0(`UserEngine.ini` 없음).
  - 로그: `Saved/Logs/S1-claude-nonunity.log`, `S1-claude-unity.log`, `S1-claude-ReachStudy.log`, `S1-claude-VRCalibrationKit.log`.

## 문제

- 없음.

## 제안 (반영 안 해도 됨)

- **N-1:** `VRCalibrationCore` 의존은 시험만 쓰지만 제품 모듈에도 링크된다. 명세 D7대로이고 모든 플랫폼 모듈이라 문제없다. 제품 코드가 키트 API를 부르기 시작하면 그때 다시 본다.
- **N-2:** `AReachProbeGameMode`(S2 시험대)는 Stage를 정하지 않는다. 명세 §2대로이고 `L_S2_ContactProbe`는 정렬하지 않는다. 시험대 레벨에 차를 놓아도 Pawn은 옮겨지지 않는다(`Stage tracking origin required`).

## 확인할 시험 (`not-tested`)

- Claude: 명세 §6 에디터 작업·PIE.
- 사용자: Quest 3 T-01(명세 §8).

---

# §6 에디터 작업 (Claude, 2026-11-24)

1. **키트 기기 시험 배치 읽기**
   - 방법: 키트 프로젝트에서 Python commandlet으로 읽기만 했다. 키트 리포는 바뀌지 않았다.
   - `BP_SampleCar` CDO와 `L_Calibrate`·`L_Sample`의 차 인스턴스 모두 C++ 기본값 그대로다(바꾼 값 없음).
     - 점 1–4: `(4,-60,50)`, `(4,-14,50)`, `(-44,-14,50)`, `(-44,-60,50)` cm
     - `FloorLocalZCm` 0, `PointFitLimitCm` 2, `Bucks` 없음
     - 차는 원점, scale 1
   - 정렬 도구도 기본값 그대로다: `MarkerArrowLocal` (1,0,0), `MarkerSizeMm` 60.
2. **`Content/Development/S1/BP_S1Car`**
   - 구성: Actor(`DefaultSceneRoot`, scale 1) + `Alignment`(`UVRCalAlignmentComponent`, `FloorLocalZCm` 0) + `AlignmentPoint1`–`4`(`Order` 1–4, `Label` `Marker N`, 위 위치).
   - Python commandlet(`-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities`)으로 만들었다. uproject는 바꾸지 않았다.
3. **`Content/Development/S1/L_S1_Calibrate`**
   - `L_S3_Touch`를 복제한 뒤 Door·IP·설정·패널을 지웠다.
   - 남은 것: Sun·Sky·Floor·SeatStart + `S1Car`(원점) + `MarkerCalibrator`(기본값).
   - GameMode override `AReachCalibrationGameMode`.
4. **`L_S3_Touch`:** `S1Car`를 원점에 더했다. `DoorVariants` 목록 밖이다.
5. **PIE(에디터, MCP)**
   - `L_S1_Calibrate`:
     - `Game class is 'ReachCalibrationGameMode'`
     - `Screen messages kept (calibration level, S1)`
     - `Tracking origin Stage, WorldToMeters 100 (S1)`
     - 키트 키 안내 로그, `Left camera started`, `Camera model check passed`
     - `S1Car.Alignment.State` = `NoSavedValue`(저장 전이라 정상)
   - `L_S3_Touch`:
     - `ReachGameMode`, `Screen messages disabled (S4b)`, Stage 로그
     - `State` = `NoSavedValue`
   - Ensure·오류 없음. Meta Project Setup Tool·LogHMD 소음뿐이다.
   - 참고: `-game -nullrhi`(XR 없음)로 레벨을 띄우면 ISDK 손 rig가 `UIsdkHandDataSource::SetInboundHandData`에서 멈춘다. 기존 `L_S3_Touch`에서도 같다. ISDK가 tracking subsystem 없이 도는 경우라 S1과 무관하다. 레벨 확인은 에디터 PIE로 한다.
6. Quest 3 T-01은 `not-tested` — 사용자 몫(명세 §8).
