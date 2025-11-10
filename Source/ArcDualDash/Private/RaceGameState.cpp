#include "RaceGameState.h"
#include "MyCar.h"
#include "Checkpoints.h"
#include "RacePlayerController.h"
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

    if (Found.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[RaceGameState] No checkpoints found in level!"));
        return;
    }

    // Sort by CheckPointNo (lowest to highest)
    Found.Sort([](const AActor& A, const AActor& B)
        {
            const int32 ANo = CastChecked<ACheckpoints>(&A)->CheckPointNo;
            const int32 BNo = CastChecked<ACheckpoints>(&B)->CheckPointNo;
            return ANo < BNo;
        });

    TrackCheckpoints = Found;
    NumCheckpoints = TrackCheckpoints.Num();

    // --- Validate checkpoint numbering ---
    TSet<int32> Seen;
    for (int32 i = 0; i < TrackCheckpoints.Num(); i++)
    {
        if (ACheckpoints* CP = Cast<ACheckpoints>(TrackCheckpoints[i]))
        {
            if (Seen.Contains(CP->CheckPointNo))
            {
                UE_LOG(LogTemp, Warning, TEXT("[RaceGameState] Duplicate CheckPointNo found: %d (%s)"),
                    CP->CheckPointNo, *CP->GetName());
            }
            Seen.Add(CP->CheckPointNo);

            UE_LOG(LogTemp, Log, TEXT("[RaceGameState] Checkpoint order %02d: %s (No=%d, StartFinish=%s)"),
                i, *CP->GetName(), CP->CheckPointNo,
                CP->bStartFinishLine ? TEXT("True") : TEXT("False"));
        }
    }

    // --- Start periodic leaderboard updates ---
    GetWorldTimerManager().SetTimer(
        LeaderboardTimerHandle,
        this,
        &ARaceGameState::UpdateLeaderboard,
        0.5f,
        true
    );

    UE_LOG(LogTemp, Log, TEXT("[RaceGameState] Loaded %d checkpoints for leaderboard tracking."), NumCheckpoints);
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

// ============================================================================
// Leaderboard logic (Lap → Checkpoint → Distance)
// ============================================================================
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

            // --- Identify player ---
            if (Car->PlayerID > 0)
            {
                Data.PlayerName = FString::Printf(TEXT("Player %d"), Car->PlayerID);
            }
            else if (APlayerController* PC = Cast<APlayerController>(Car->GetController()))
            {
                if (ARacePlayerController* RPC = Cast<ARacePlayerController>(PC))
                    Data.PlayerName = FString::Printf(TEXT("Player %d"), RPC->PlayerIndex);
                else
                    Data.PlayerName = TEXT("Player ?");
            }
            else
            {
                Data.PlayerName = TEXT("AI");
            }

            // --- Distance to next checkpoint ---
            float DistanceToNext = 999999.f;

            if (TrackCheckpoints.IsValidIndex(Data.Checkpoint))
            {
                const int32 CurrIdx = Data.Checkpoint % NumCheckpoints;
                const int32 NextIdx = (CurrIdx + 1) % NumCheckpoints;

                const FVector CarLoc = Car->GetActorLocation();
                const FVector NextLoc = TrackCheckpoints[NextIdx]->GetActorLocation();

                DistanceToNext = FVector::Dist(CarLoc, NextLoc);
            }

            Data.DistanceToNext = DistanceToNext;

            Leaderboard.Add(Data);

            UE_LOG(LogTemp, Log, TEXT("[LeaderboardDebug] %s -> Lap=%d | CP=%d | DistToNext=%.1f"),
                *Data.PlayerName, Data.Lap, Data.Checkpoint, Data.DistanceToNext);
        }
    }

    // --- Sort by Lap, then Checkpoint, then Distance ---
    Leaderboard.Sort([](const FPlayerRaceData& A, const FPlayerRaceData& B)
        {
            // Higher lap wins
            if (A.Lap != B.Lap)
                return A.Lap > B.Lap;

            // Same lap: higher checkpoint wins
            if (A.Checkpoint != B.Checkpoint)
                return A.Checkpoint > B.Checkpoint;

            // Same lap and checkpoint: closer to next checkpoint wins
            return A.DistanceToNext < B.DistanceToNext;
        });

    OnLeaderboardUpdated.Broadcast();
}
