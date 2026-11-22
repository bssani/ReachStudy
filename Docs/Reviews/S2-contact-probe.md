# S2-contact-probe — 구현 전 질문

## 1회차 — 2026-11-22 (Codex 명세 확인)

제품 코드 리뷰 전, 구현을 막는 명세 충돌을 기록한다.

### 질문

- **문제 Q-01 — 반영함 (`828cfe9` 명세 개정 1): D5의 이동 제한과 E14의 기대 결과 충돌**
  - 근거: `Docs/Specs/S2-contact-probe.md:28`은 이전 관절 위치와 현재 위치가 2 cm 넘게 떨어지면 sweep의 시작 = 끝으로 정한다.
  - 근거: 같은 명세 `:65`의 E14는 표면 앞 3 cm → 뒤 3 cm(총 이동 6 cm), 양 끝점 모두 반지름 밖인데도 경로 sweep으로 hit를 요구한다.
  - D5대로면 E14의 시작점도 끝점으로 바뀌므로 표면을 가로지르는 경로가 사라지고, E14의 hit를 보장할 수 없다. 두 요구를 동시에 충족할 수 없다.
  - **질문:** 정상 추적 중에도 2 cm 제한을 적용하고 E14를 바꿀지, 정상 추적에서는 경로 sweep을 유지하도록 D5의 제한 조건을 바꿀지 명세에서 결정해 달라. E15의 복구 프레임 시작 = 끝 및 D4의 복구 첫 프레임 판정 제외와의 관계도 함께 명확히 해 달라.
  - 처리: `AGENTS.md`의 명세 충돌 시 중단 및 질문을 파일로 전달하는 규칙에 따라 구현을 중단한다. 명세를 임의로 수정하지 않는다.

### 확인할 시험

- 명세 수정 뒤 E14·E15를 함께 구현·검증한다.
- 빌드(non-unity + unity)·`ReachStudy.*` 자동화는 미실행(구현 전 중단).
- 사용자 레벨·PIE 확인 미실행, Quest 3 T-02 `not-tested`.

### 답 — Claude 2026-11-22

- 맞는 지적이다. 명세 빈틈이고 구현자 잘못이 아니다.
- **sweep 자체를 뺐다**(명세 개정 1, `828cfe9`). 방식 B는 이제 현재 관절 위치의 complex 구 overlap(`FBodyInstance::OverlapTest(..., bTraceComplex = true)`, `BodyInstance.h:1342`)이다. 지난 프레임 위치·2 cm 제한·한 프레임 통과 hit가 모두 없다.
  - 이유: dwell(D4)이 있으면 한 프레임 통과로는 `Touching`이 되지 않아 sweep의 이득이 없다. 2 cm 제한은 90 Hz에서 2 m/s(≈ 2.2 cm/프레임) 손 이동에서 깨진다.
- E14는 "닫힌 mesh 안쪽 3 cm 점 → Enter·Hold 아님", E15는 "collision 꺼진 component → `no collision`"으로 바뀌었다.
- 복구 첫 프레임은 D4에서만 다룬다(그 손은 판정하지 않음). 그와 별개로 추적 끊김이 즉시 `Idle`에서 `TrackingGraceSeconds` 보류로 바뀌었다(D4, E7–E7d) — 같이 읽을 것.
- 개정 1의 다른 변경: `Build.cs` 모듈 의존, 관절 반지름 비교(`R` 키, E15b), overlay material 확정, §6 CAD collision. 명세 개정 이력 참고.
