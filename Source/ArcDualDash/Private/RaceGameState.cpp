#include "RaceGameState.h"
#include "MyCar.h"
#include "Checkpoints.h" // your checkpoint actor class
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerState.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

ARaceGameState::ARaceGameState()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARaceGameState::BeginPlay()
{
    Super::BeginPlay();

    // --- Cache all checkpoint actors in order ---
    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACheckpoints::StaticClass(), Found);

    Found.Sort([](const AActor& A, const AActor& B)
        {
            const int32 ANo = CastChecked<ACheckpoints>(&A)->CheckPointNo;
            const int32 BNo = CastChecked<ACheckpoints>(&B)->CheckPointNo;
            return ANo < BNo;
        });

    TrackCheckpoints = Found;
    NumCheckpoints = TrackCheckpoints.Num();

    // --- Start periodic leaderboard updates ---
    GetWorldTimerManager().SetTimer(
        LeaderboardTimerHandle,
        this,
        &ARaceGameState::UpdateLeaderboard,
        0.5f, // update 4x per second for smoother results
        true
    );
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

    if (CurrentLap > TotalLaps)
    {
        CurrentLap = TotalLaps;
        bTimerRunning = false;
    }

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

// --- Leaderboard logic ---
void ARaceGameState::UpdateLeaderboard()
{
    Leaderboard.Empty();

    if (NumCheckpoints == 0)
        return;

    TArray<AActor*> FoundCars;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyCar::StaticClass(), FoundCars);

    for (AActor* Actor : FoundCars)
    {
        if (AMyCar* Car = Cast<AMyCar>(Actor))
        {
            FPlayerRaceData Data;
            Data.Car = Car;
            Data.Lap = Car->Lap;
            Data.Checkpoint = Car->CurrentCheckpointIndex;

            // --- Name players 1,2,3 properly for splitscreen ---
            if (APlayerController* PC = Cast<APlayerController>(Car->GetController()))
            {
                if (ULocalPlayer* LP = PC->GetLocalPlayer())
                {
                    const int32 LocalIdx = LP->GetControllerId();
                    Data.PlayerName = FString::Printf(TEXT("Player %d"), LocalIdx + 1);
                }
                else
                {
                    Data.PlayerName = TEXT("Player ?");
                }
            }
            else
            {
                Data.PlayerName = TEXT("AI");
            }

            // --- Compute progress value ---
            float SegFrac = 0.f;
            if (TrackCheckpoints.IsValidIndex(Data.Checkpoint))
            {
                const int32 CurrIdx = Data.Checkpoint % NumCheckpoints;
                const int32 NextIdx = (CurrIdx + 1) % NumCheckpoints;

                const FVector A = TrackCheckpoints[CurrIdx]->GetActorLocation();
                const FVector B = TrackCheckpoints[NextIdx]->GetActorLocation();
                SegFrac = CalculateSegmentT(A, B, Car->GetActorLocation());
            }

            // Final continuous progress key
            Data.ProgressKey = (float)Data.Lap * (float)NumCheckpoints + (float)Data.Checkpoint + SegFrac;

            Leaderboard.Add(Data);
        }
    }

    // --- Sort descending by progress ---
    Leaderboard.Sort([](const FPlayerRaceData& A, const FPlayerRaceData& B)
        {
            return A.ProgressKey > B.ProgressKey;
        });

    OnLeaderboardUpdated.Broadcast();
}
