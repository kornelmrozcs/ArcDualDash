// ============================================================================
// ArcGameMode.cpp
// notes: GameMode for ArcDualDash. I keep it minimal and data-driven.
//        - I own LocalPlayer #2 creation for split-screen.
//        - I choose PlayerStart per player using map tags (P1/P2).
//        - I don’t spawn HUD here; PlayerControllers own UI (per our pattern).
// ============================================================================

#include "ArcGameMode.h"

// --- Project types I coordinate with ---
#include "ArcPlayerState.h"   // notes: I set PlayerStateClass = AArcPlayerState::StaticClass()
#include "ArcGameState.h"     // notes: authoritative round timer (count-up) lives here

// --- Engine / Gameplay helpers ---
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h" // notes: for GetPlayerId()
#include "Kismet/GameplayStatics.h"    // notes: CreatePlayer, GetAllActorsOfClass
#include "Engine/World.h"
#include "Engine/Engine.h"

// ============================================================================
// 1) Constructor
// notes: I bind core classes. I keep the rest in Project Settings for BP flexibility.
// ============================================================================
AArcGameMode::AArcGameMode()
{
    // notes: use my custom state classes; other defaults remain data-driven via Project Settings
    PlayerStateClass = AArcPlayerState::StaticClass();
    GameStateClass = AArcGameState::StaticClass();  // notes: read-only timer authority for UI
}

// ============================================================================
// 2) BeginPlay
// notes: I enforce local split-screen by creating a second LocalPlayer here.
//        - I do this once, idempotent.
//        - UI remains in PlayerController (CreateWidget + AddViewportWidgetForPlayer).
//        - This keeps packaging behavior identical to PIE.
// ============================================================================
void AArcGameMode::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArcGameMode] World is null at BeginPlay() – aborting LocalPlayer creation."));
        return;
    }

    const int32 ExistingPCs = World->GetNumPlayerControllers();
    if (ExistingPCs >= 2)
    {
        UE_LOG(LogTemp, Log, TEXT("[ArcGameMode] %d PC(s) present – skipping LocalPlayer#2 creation."), ExistingPCs);
        return;
    }

    // notes: ControllerId 0 is primary; I request ControllerId 1 for the second split viewport.
    if (UGameplayStatics::CreatePlayer(World, /*ControllerId*/ 1, /*bSpawnPawn*/ true))
    {
        UE_LOG(LogTemp, Log, TEXT("[ArcGameMode] LocalPlayer#2 created successfully for split-screen."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArcGameMode] Failed to create LocalPlayer#2."));
    }
}

// ============================================================================
// 3) Helpers (internal)
// notes: I support both Actor Tags and PlayerStartTag (case-insensitive).
//        Level setup: place two PlayerStarts; tag them as P1 and P2.
//        - Actor → Details → Tags: add "P1" or "P2"
//        - or PlayerStart → Player Start Tag: set "P1" or "P2"
// ============================================================================
static bool MatchesWantedTag(AActor* Start, const FName& WantedTag)
{
    if (!IsValid(Start)) return false;

    // notes: 3a) Actor Tags
    for (const FName& Tag : Start->Tags)
    {
        if (Tag.IsEqual(WantedTag, ENameCase::IgnoreCase))
        {
            return true;
        }
    }

    // notes: 3b) PlayerStartTag property
    if (const APlayerStart* PS = Cast<APlayerStart>(Start))
    {
        if (PS->PlayerStartTag.IsEqual(WantedTag, ENameCase::IgnoreCase))
        {
            return true;
        }
    }

    return false;
}

// ============================================================================
// 4) PlayerStart selection (per-player)
// notes: I map player → start by tag with a stable per-player index:
//        - Index 0 → "P1", Index 1 → "P2"
//        I log what I find for quick debugging.
//        Cross-links:
//        - AArcPlayerController: owns HUD per LocalPlayer (score/timer per viewport)
//        - AArcGameState: timer the HUD reads (I don’t touch UI here)
// ============================================================================
AActor* AArcGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    // notes: keep parent fallback; I only override when I find tagged starts
    AActor* Fallback = Super::ChoosePlayerStart_Implementation(Player);

    // notes: derive stable per-player index for split-screen
    int32 Index = -1;

    // 1) Prefer PlayerState->GetPlayerId() (valid on server during spawn)
    if (APlayerState* PS = Player ? Player->GetPlayerState<APlayerState>() : nullptr)
    {
        Index = PS->GetPlayerId();
    }

    // 2) Fallback: LocalPlayer->GetControllerId() (can be null in GameMode)
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

    // 3) Last resort: guess from number of PlayerControllers
    if (Index < 0)
    {
        const int32 NumPCs = GetWorld() ? GetWorld()->GetNumPlayerControllers() : 1;
        Index = (NumPCs > 1) ? 1 : 0;
    }

    const FName WantedPrimary = (Index % 2 == 0) ? FName(TEXT("P1")) : FName(TEXT("P2"));
    const FName WantedSecondary = (Index % 2 == 0) ? FName(TEXT("P2")) : FName(TEXT("P1"));

    UE_LOG(LogTemp, Log, TEXT("[GM] ChoosePlayerStart: Index=%d (PS/LP), want [%s] then [%s]"),
        Index, *WantedPrimary.ToString(), *WantedSecondary.ToString());

    // notes: gather and log all PlayerStarts so I can see tags immediately
    TArray<AActor*> Starts;
    UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), Starts);

    UE_LOG(LogTemp, Log, TEXT("[GM] ChoosePlayerStart: Found %d PlayerStarts."), Starts.Num());
    for (AActor* S : Starts)
    {
        const APlayerStart* PS = Cast<APlayerStart>(S);
        FString ActorTags;
        for (const FName& Tag : S->Tags)
        {
            ActorTags += Tag.ToString() + TEXT(" ");
        }
        const FString PSTag = PS ? PS->PlayerStartTag.ToString() : TEXT("<none>");
        UE_LOG(LogTemp, Log, TEXT("  - %s | ActorTags=[%s] | PlayerStartTag=%s"),
            *GetNameSafe(S), *ActorTags, *PSTag);
    }

    // notes: try PRIMARY tag first
    for (AActor* Start : Starts)
    {
        if (MatchesWantedTag(Start, WantedPrimary))
        {
            UE_LOG(LogTemp, Log, TEXT("[GM] ChoosePlayerStart: matched PRIMARY %s -> %s"),
                *WantedPrimary.ToString(), *GetNameSafe(Start));
            return Start;
        }
    }

    // notes: graceful fallback to SECONDARY tag (lets me keep playing even if one tag is missing)
    for (AActor* Start : Starts)
    {
        if (MatchesWantedTag(Start, WantedSecondary))
        {
            UE_LOG(LogTemp, Warning, TEXT("[GM] ChoosePlayerStart: PRIMARY %s not found, using SECONDARY %s -> %s"),
                *WantedPrimary.ToString(), *WantedSecondary.ToString(), *GetNameSafe(Start));
            return Start;
        }
    }

    // notes: last resort — parent logic
    UE_LOG(LogTemp, Warning, TEXT("[GM] ChoosePlayerStart: no start with %s nor %s, using fallback"),
        *WantedPrimary.ToString(), *WantedSecondary.ToString());
    return Fallback;
}
