#pragma once

enum class EReachContactCondition { None, Hold, Enter };
enum class EReachContactPhase { Idle, Dwelling, Touching };

struct FReachHandContact
{
    bool bTracked = false;
    EReachContactCondition Condition = EReachContactCondition::None;
};

struct FReachContactState
{
    EReachContactPhase Phase = EReachContactPhase::Idle;
    double DwellElapsedSeconds = 0.0;

    bool Step(FReachHandContact Left, FReachHandContact Right, double DeltaSeconds,
        double DwellSeconds, double TrackingGraceSeconds)
    {
        const EReachContactPhase Previous = Phase;
        const FReachHandContact Inputs[2] = {Left, Right};
        bool bMeasuredEnter = false;
        bool bHeldEnter = false;
        bool bAnyHold = false;
        for (int Hand = 0; Hand < 2; ++Hand)
        {
            FHandHistory& History = Hands[Hand];
            const bool bMeasured = Inputs[Hand].bTracked && History.bWasTracked;
            History.bWasTracked = Inputs[Hand].bTracked;
            if (bMeasured)
            {
                History.bHasMeasurement = true;
                History.LastCondition = Inputs[Hand].Condition;
                History.UnmeasuredSeconds = 0.0;
            }
            else
            {
                // Recovery frame consumes grace too (S2 D4).
                History.UnmeasuredSeconds += DeltaSeconds;
            }
            const bool bHeld = !bMeasured && History.bHasMeasurement
                && History.UnmeasuredSeconds <= TrackingGraceSeconds;
            const EReachContactCondition Result = bMeasured || bHeld
                ? History.LastCondition : EReachContactCondition::None;
            bMeasuredEnter |= bMeasured && Result == EReachContactCondition::Enter;
            bHeldEnter |= bHeld && Result == EReachContactCondition::Enter;
            bAnyHold |= Result != EReachContactCondition::None;
        }

        if (Phase == EReachContactPhase::Touching)
        {
            if (!bAnyHold)
            {
                Phase = EReachContactPhase::Idle;
                DwellElapsedSeconds = 0.0;
            }
        }
        else if (bMeasuredEnter)
        {
            DwellElapsedSeconds += DeltaSeconds;
            Phase = DwellElapsedSeconds >= DwellSeconds
                ? EReachContactPhase::Touching : EReachContactPhase::Dwelling;
        }
        else if (!bHeldEnter)
        {
            Phase = EReachContactPhase::Idle;
            DwellElapsedSeconds = 0.0;
        }
        return Phase != Previous;
    }

private:
    struct FHandHistory
    {
        bool bWasTracked = false;
        bool bHasMeasurement = false;
        double UnmeasuredSeconds = 0.0;
        EReachContactCondition LastCondition = EReachContactCondition::None;
    };
    FHandHistory Hands[2];
};
