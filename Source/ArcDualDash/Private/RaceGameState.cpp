// Fill out your copyright notice in the Description page of Project Settings.


#include "RaceGameState.h"

ARaceGameState::ARaceGameState()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARaceGameState::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bTimerRunning)
    {
        ElapsedTime += DeltaSeconds;
        OnTimeUpdated.Broadcast(ElapsedTime);
    }
}

void ARaceGameState::IncrementLapAndBroadcast()
{
    CurrentLap++;

    // Loop back if race finished
    if (CurrentLap > TotalLaps)
    {
        CurrentLap = TotalLaps;
        bTimerRunning = false;
    }

    // Notify any listeners (HUD)
    OnLapChanged.Broadcast(CurrentLap, TotalLaps);
}

void ARaceGameState::ResetTimer()
{
    ElapsedTime = 0.f;
    OnTimeUpdated.Broadcast(ElapsedTime);
}

void ARaceGameState::StopTimer()
{
    bTimerRunning = false;
}



