#include "ArcGameMode.h"
#include "ArcPlayerState.h"   // notes: for PlayerStateClass = AArcPlayerState::StaticClass()

#include "Kismet/GameplayStatics.h" // UGameplayStatics::CreatePlayer
#include "Engine/World.h"

AArcGameMode::AArcGameMode()
{
    // Intentionally left empty. We bind defaults via Project Settings to keep BP flexibility.

    PlayerStateClass = AArcPlayerState::StaticClass();
}

void AArcGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Purpose: Ensure local split-screen by creating a second LocalPlayer at runtime.
    // Rationale: This avoids editor-only "Number of Players" settings and works in a packaged build.
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArcGameMode] World is null at BeginPlay() – aborting LocalPlayer creation."));
        return;
    }

    // If we already have two or more PlayerControllers, do nothing (idempotent behavior).
    const int32 ExistingPCs = World->GetNumPlayerControllers();
    if (ExistingPCs >= 2)
    {
        UE_LOG(LogTemp, Log, TEXT("[ArcGameMode] %d PC(s) present – skipping LocalPlayer#2 creation."), ExistingPCs);
        return;
    }

    // Index 0 is the primary LocalPlayer; we request index 1 for the second split-screen view.
    if (UGameplayStatics::CreatePlayer(World, /*ControllerId*/ 1, /*bSpawnPawn*/ true))
    {
        UE_LOG(LogTemp, Log, TEXT("[ArcGameMode] LocalPlayer#2 created successfully for split-screen."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArcGameMode] Failed to create LocalPlayer#2."));
    }
}


