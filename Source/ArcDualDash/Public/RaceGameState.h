// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "RaceGameState.generated.h"

// --- Delegates for HUD binding ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeUpdated, float, NewTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLapChanged, int32, NewLap, int32, TotalLaps);

UCLASS()
class ARCDUALDASH_API ARaceGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ARaceGameState();

    virtual void Tick(float DeltaSeconds) override;

    // --- Delegates for UI ---
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnTimeUpdated OnTimeUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnLapChanged OnLapChanged;

    // --- Race data ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race")
    int32 TotalLaps = 3;

    UPROPERTY(BlueprintReadOnly, Category = "Race")
    int32 CurrentLap = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Race")
    float ElapsedTime = 0.f;

    // --- State control ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Race")
    bool bTimerRunning = true;

    // --- Methods for lap and time handling ---
    UFUNCTION(BlueprintCallable, Category = "Race")
    void IncrementLapAndBroadcast();

    UFUNCTION(BlueprintCallable, Category = "Race")
    void ResetTimer();

    UFUNCTION(BlueprintCallable, Category = "Race")
    void StopTimer();
};


