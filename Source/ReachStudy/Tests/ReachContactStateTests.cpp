#include "Misc/AutomationTest.h"
#include "Contact/ReachContact.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
using C = EReachContactCondition;
using P = EReachContactPhase;
const FReachHandContact Enter{true, C::Enter};
const FReachHandContact Hold{true, C::Hold};
const FReachHandContact Far{true, C::None};
const FReachHandContact Lost{false, C::None};

void Prime(FReachContactState& State, FReachHandContact Left = Enter, FReachHandContact Right = Lost)
{
    State.Step(Left, Right, 0.0, 0.15, 0.1);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachDwellTest, "ReachStudy.S2.State.E1_E3_Dwell", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachDwellTest::RunTest(const FString& Parameters)
{
    FReachContactState State;
    Prime(State);
    TestTrue(TEXT("Enter starts dwelling"), State.Step(Enter, Lost, 0.1, 0.15, 0.1));
    TestTrue(TEXT("Short enter is not touching"), State.Phase == P::Dwelling);
    State.Step(Far, Lost, 0.01, 0.15, 0.1);
    TestTrue(TEXT("E1: leaving resets dwell"), State.Phase == P::Idle && State.DwellElapsedSeconds == 0.0);
    State.Step(Enter, Lost, 0.1, 0.15, 0.1);
    TestTrue(TEXT("E2: threshold changes state"), State.Step(Enter, Lost, 0.05, 0.15, 0.1));
    TestTrue(TEXT("E2: touching at 0.15 s"), State.Phase == P::Touching);
    TestFalse(TEXT("No repeated change signal"), State.Step(Enter, Lost, 0.02, 0.15, 0.1));
    FReachContactState Immediate;
    Prime(Immediate);
    Immediate.Step(Enter, Lost, 0.01, 0.0, 0.1);
    TestTrue(TEXT("E3: zero dwell touches immediately"), Immediate.Phase == P::Touching);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachHoldTest, "ReachStudy.S2.State.E4_E6_E8_Hold", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachHoldTest::RunTest(const FString& Parameters)
{
    FReachContactState State;
    Prime(State, Enter, Far);
    State.Step(Enter, Far, 0.15, 0.15, 0.1);
    TestFalse(TEXT("E4: hold keeps touching"), State.Step(Hold, Far, 0.01, 0.15, 0.1));
    for (int32 Frame = 0; Frame < 20; ++Frame)
    {
        TestFalse(TEXT("E6: enter/hold boundary does not flicker"), State.Step(Frame % 2 ? Enter : Hold, Far, 0.01, 0.15, 0.1));
    }
    TestFalse(TEXT("E8: other hand hold takes over"), State.Step(Far, Hold, 0.01, 0.15, 0.1));
    TestTrue(TEXT("E5: both far release immediately"), State.Step(Far, Far, 0.01, 0.15, 0.1));
    TestTrue(TEXT("E5: idle with zero dwell"), State.Phase == P::Idle && State.DwellElapsedSeconds == 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReachTrackingGraceTest, "ReachStudy.S2.State.E7_TrackingGrace", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FReachTrackingGraceTest::RunTest(const FString& Parameters)
{
    FReachContactState State;
    Prime(State);
    State.Step(Enter, Lost, 0.15, 0.15, 0.1);
    TestFalse(TEXT("E7: short loss has no transition"), State.Step(Lost, Lost, 0.01, 0.15, 0.1));
    TestFalse(TEXT("E7: recovery frame has no transition"), State.Step(Far, Lost, 0.01, 0.15, 0.1));
    TestFalse(TEXT("E7: next measured hold keeps touching"), State.Step(Hold, Lost, 0.01, 0.15, 0.1));
    TestTrue(TEXT("E7: still touching"), State.Phase == P::Touching);

    FReachContactState Dwell;
    Prime(Dwell);
    Dwell.Step(Enter, Lost, 0.1, 0.15, 0.1);
    Dwell.Step(Lost, Lost, 0.05, 0.15, 0.1);
    TestEqual(TEXT("E7b: loss freezes dwell"), Dwell.DwellElapsedSeconds, 0.1);
    Dwell.Step(Far, Lost, 0.01, 0.15, 0.1);
    TestEqual(TEXT("E7b: recovery freezes dwell"), Dwell.DwellElapsedSeconds, 0.1);
    Dwell.Step(Enter, Lost, 0.05, 0.15, 0.1);
    TestTrue(TEXT("E7b: measured enter completes dwell"), Dwell.Phase == P::Touching);

    State.Step(Lost, Lost, 0.1, 0.15, 0.1);
    TestTrue(TEXT("E7c: exact grace boundary retains hold"), State.Phase == P::Touching);
    State.Step(Lost, Lost, 0.001, 0.15, 0.1);
    TestTrue(TEXT("E7c: exceeding grace releases"), State.Phase == P::Idle && State.DwellElapsedSeconds == 0.0);

    FReachContactState PerHand;
    Prime(PerHand, Enter, Far);
    PerHand.Step(Enter, Far, 0.15, 0.15, 0.1);
    PerHand.Step(Lost, Far, 0.05, 0.15, 0.1);
    TestTrue(TEXT("E7d: visible far hand does not cancel held contact"), PerHand.Phase == P::Touching);
    PerHand.Step(Lost, Far, 0.051, 0.15, 0.1);
    TestTrue(TEXT("E7d: lost hand expires independently"), PerHand.Phase == P::Idle);

    FReachContactState OtherEnter;
    Prime(OtherEnter, Enter, Far);
    OtherEnter.Step(Enter, Far, 0.1, 0.15, 0.1);
    OtherEnter.Step(Lost, Enter, 0.05, 0.15, 0.1);
    TestTrue(TEXT("E7e: measured other hand advances dwell"), OtherEnter.Phase == P::Touching);

    FReachContactState NeverTracked;
    NeverTracked.Step(Lost, Lost, 0.01, 0.0, 1.0);
    TestTrue(TEXT("E7f: no measurement cannot be held"), NeverTracked.Phase == P::Idle);
    NeverTracked.Step(Enter, Lost, 0.01, 0.0, 1.0);
    TestTrue(TEXT("First tracked frame is excluded"), NeverTracked.Phase == P::Idle);
    NeverTracked.Step(Enter, Lost, 0.01, 0.0, 1.0);
    TestTrue(TEXT("Next tracked frame is measured"), NeverTracked.Phase == P::Touching);
    return true;
}
#endif
