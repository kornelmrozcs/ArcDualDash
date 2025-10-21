#pragma once

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
    // notes: total elapsed time in whole seconds
    UFUNCTION(BlueprintCallable, Category = "Timer")
    int32 GetElapsedSeconds() const { return ElapsedSeconds; }

protected:
    virtual void BeginPlay() override;

private:
    // notes: internal counter
    int32 ElapsedSeconds = 0;

    FTimerHandle TimerHandle_CountUp;

    // notes: called every 1 second
    void TickOneSecond();
};
