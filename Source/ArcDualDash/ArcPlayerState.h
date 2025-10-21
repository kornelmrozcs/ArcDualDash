#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ArcPlayerState.generated.h"

/**
 * Per-player score holder
 * notes: simple local score for split-screen; can be extended to replicate if needed
 */
UCLASS()
class ARCDUALDASH_API AArcPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    // notes: returns current score (uses built-in APlayerState::GetScore())
    UFUNCTION(BlueprintCallable, Category = "Score")
    int32 GetScoreInt() const { return static_cast<int32>(GetScore()); }

    // notes: add points (can be negative)
    UFUNCTION(BlueprintCallable, Category = "Score")
    void AddScoreInt(int32 Delta)
    {
        const float NewScore = GetScore() + static_cast<float>(Delta);
        SetScore(NewScore);
        UE_LOG(LogTemp, Log, TEXT("[Score] %s += %d => %d"),
            *GetPlayerName(),
            Delta,
            static_cast<int32>(NewScore));
    }
};
