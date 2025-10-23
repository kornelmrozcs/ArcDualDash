#pragma once

// ============================================================================
// ArcPlayerState.h
// notes: I keep per-player score here, because each local player needs its own
//        counter in split-screen. UI reads from me; collectables add to me. KM
//        Used by: W_PlayerScore (reads GetScoreInt), ACollectable (calls AddScoreInt). KM
// ============================================================================

#include "CoreMinimal.h"                 // notes: Basic UE types; required for UCLASS/UFUNCTION. KM
#include "GameFramework/PlayerState.h"   // notes: I extend PlayerState to reuse built-in float Score. KM
#include "ArcPlayerState.generated.h"    // notes: UHT reflection for BP access. KM

/**
 * notes: Simple local score holder for split-screen. If we need networking later,
 *        I will replicate the built-in Score field. KM
 */
UCLASS()
class ARCDUALDASH_API AArcPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    // =========================================================================
    // Public API
    // =========================================================================

    UFUNCTION(BlueprintCallable, Category = "Score")
    int32 GetScoreInt() const
    {
        // notes: I expose score as int because UI and pickups use ints; built-in Score is float. KM
        return static_cast<int32>(GetScore());
    }

    UFUNCTION(BlueprintCallable, Category = "Score")
    void AddScoreInt(int32 Delta)
    {
        // notes: This adds/subtracts points. I use built-in float Score, then cast to int for logs/UI. KM
        //        I did it this way to stay compatible with APlayerState while keeping UI simple. KM
        const float NewScore = GetScore() + static_cast<float>(Delta);
        SetScore(NewScore);

        UE_LOG(LogTemp, Log, TEXT("[Score] %s += %d => %d"),
            *GetPlayerName(),
            Delta,
            static_cast<int32>(NewScore)); // notes: Helps me verify pickups update score during tests. KM
    }
};
