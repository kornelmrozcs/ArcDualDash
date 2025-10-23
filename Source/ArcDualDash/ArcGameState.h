#pragma once

// ============================================================================
// ArcGameState.h
// note:    Authoritative round timer (count-up). I keep time here and expose
//        a read-only accessor for UI. Widgets should only READ.
//        Cross-links:
//        - AArcPlayerController: spawns W_RoundTimer per LocalPlayer.
//        - W_RoundTimer (UMG): reads GetElapsedSeconds() every ~0.1s and formats MM:SS.
//        - It do NOT own any UI logic; just the timer source of truth.
// ============================================================================

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ArcGameState.generated.h"

/**
 * Round timer that counts UP from 00:00.
 * notes: simple 1s tick; UI will read ElapsedSeconds and format as MM:SS.
 */
UCLASS()
class ARCDUALDASH_API AArcGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    // =========================================================================
    // Public API
    // notes: expose the elapsed whole seconds. Widgets format this as MM:SS.
    // =========================================================================
    UFUNCTION(BlueprintCallable, Category = "Timer")
    int32 GetElapsedSeconds() const { return ElapsedSeconds; }

protected:
    // =========================================================================
    // Lifecycle
    // =========================================================================
    virtual void BeginPlay() override;

private:
    // =========================================================================
    // Internals
    // notes: store time as whole seconds; UI handles presentation.
    // =========================================================================
    int32 ElapsedSeconds = 0;          // notes: authoritative counter (seconds)
    FTimerHandle TimerHandle_CountUp;  // notes: 1s repeating handle

    // notes: called every 1 second to increment the counter and log
    void TickOneSecond();
};
