// ============================================================================
// RaceGameMode.cpp
// notes: - create LocalPlayer #2 for split-screen
//        - spawn each player at a tagged PlayerStart (P1/P2)
//        I keep UI/HUD in PlayerController (not here).  KM
// ============================================================================
#include "RaceGameMode.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// ----------------------------------------------------------------------------
// ctor: keep minimal; let BP/ProjectSettings define default pawn etc.
// ----------------------------------------------------------------------------
ARaceGameMode::ARaceGameMode()
{
    // notes: leave defaults data-driven; we will set Default Pawn in BP GameMode. KM
}

// ----------------------------------------------------------------------------
// BeginPlay: make sure we have 2 LocalPlayers for split-screen
// ----------------------------------------------------------------------------
void ARaceGameMode::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[RaceGM] World null in BeginPlay"));
        return;
    }

    // notes: idempotent guard (don’t spawn extra players on restart). KM
    if (World->GetNumPlayerControllers() >= 2)
    {
        UE_LOG(LogTemp, Log, TEXT("[RaceGM] 2+ controllers already present"));
        return;
    }

    // notes: ControllerId 0 exists; request ControllerId 1 for second viewport. KM
    if (UGameplayStatics::CreatePlayer(World, /*ControllerId*/1, /*bSpawnPawn*/true))
    {
        UE_LOG(LogTemp, Log, TEXT("[RaceGM] Created LocalPlayer #2 (split-screen ready)"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[RaceGM] Failed to create LocalPlayer #2"));
    }
}

// ----------------------------------------------------------------------------
// Helper: accept Actor Tags and PlayerStartTag (case-insensitive)
// ----------------------------------------------------------------------------
bool ARaceGameMode::MatchesWantedTag(AActor* Start, const FName& WantedTag)
{
    if (!IsValid(Start)) return false;

    for (const FName& Tag : Start->Tags)
    {
        if (Tag.IsEqual(WantedTag, ENameCase::IgnoreCase))
        {
            return true;
        }
    }
    if (const APlayerStart* PS = Cast<APlayerStart>(Start))
    {
        if (PS->PlayerStartTag.IsEqual(WantedTag, ENameCase::IgnoreCase))
        {
            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------------------
// ChoosePlayerStart: map Controller (0/1) -> "P1"/"P2"
// ----------------------------------------------------------------------------
AActor* ARaceGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    // notes: keep parent fallback in case tags are missing
    AActor* Fallback = Super::ChoosePlayerStart_Implementation(Player);

    // --- stable per-player index (this fixed the both-as-P1 issue before) ---
    int32 Index = -1;

    // 1) Prefer PlayerState id (valid on server during spawn)
    if (APlayerState* PS = Player ? Player->GetPlayerState<APlayerState>() : nullptr)
    {
        Index = PS->GetPlayerId();
    }

    // 2) Fallback: LocalPlayer controller id (can be null during GM spawn in PIE)
    if (Index < 0)
    {
        if (const APlayerController* PC = Cast<APlayerController>(Player))
        {
            if (const ULocalPlayer* LP = PC->GetLocalPlayer())
            {
                Index = LP->GetControllerId();
            }
        }
    }

    // 3) Last resort: guess from number of player controllers
    if (Index < 0)
    {
        const int32 NumPCs = GetWorld() ? GetWorld()->GetNumPlayerControllers() : 1;
        Index = (NumPCs > 1) ? 1 : 0;
    }

    const FName WantedPrimary = (Index % 2 == 0) ? FName(TEXT("P1")) : FName(TEXT("P2"));
    const FName WantedSecondary = (Index % 2 == 0) ? FName(TEXT("P2")) : FName(TEXT("P1"));

    UE_LOG(LogTemp, Log, TEXT("[RaceGM] ChoosePlayerStart: Index=%d, want [%s] then [%s]"),
        Index, *WantedPrimary.ToString(), *WantedSecondary.ToString());

    // gather all starts
    TArray<AActor*> Starts;
    UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), Starts);
    UE_LOG(LogTemp, Log, TEXT("[RaceGM] Found %d PlayerStarts."), Starts.Num());

    for (AActor* S : Starts)
    {
        if (MatchesWantedTag(S, WantedPrimary))
        {
            UE_LOG(LogTemp, Log, TEXT("[RaceGM] PRIMARY %s -> %s"),
                *WantedPrimary.ToString(), *GetNameSafe(S));
            return S;
        }
    }

    for (AActor* S : Starts)
    {
        if (MatchesWantedTag(S, WantedSecondary))
        {
            UE_LOG(LogTemp, Warning, TEXT("[RaceGM] PRIMARY missing, using SECONDARY %s -> %s"),
                *WantedSecondary.ToString(), *GetNameSafe(S));
            return S;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[RaceGM] no tagged starts; using fallback"));
    return Fallback;
}



