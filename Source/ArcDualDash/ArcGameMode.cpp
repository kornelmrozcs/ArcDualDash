// ============================================================================
// ArcGameMode.cpp
// notes: Minimal, data-driven GameMode. I:
//        - create LocalPlayer #2 for split-screen,
//        - pick PlayerStart per player by tag,
//        - leave HUD to PlayerControllers. KM
// ===========================================================================

#include "ArcGameMode.h"

// --- Project types I coordinate with ---
#include "ArcPlayerState.h"   // notes: I set PlayerStateClass so score lives on our type. KM
#include "ArcGameState.h"     // notes: I set GameStateClass so UI reads one timer source. KM

// --- Engine / helpers ---
#include "GameFramework/PlayerController.h" // notes: Needed for controller checks. KM
#include "GameFramework/PlayerStart.h"      // notes: I search these when spawning. KM
#include "GameFramework/PlayerState.h"      // notes: I read GetPlayerId() for stable index. KM
#include "Kismet/GameplayStatics.h"         // notes: CreatePlayer + GetAllActorsOfClass. KM
#include "Engine/World.h"
#include "Engine/Engine.h"

// ============================================================================
// 1) Constructor
// notes: Bind our custom state classes; keep other defaults in Project Settings. KM
// ============================================================================
AArcGameMode::AArcGameMode()
{
    PlayerStateClass = AArcPlayerState::StaticClass(); // notes: So pickups/UI talk to our PS. KM
    GameStateClass = AArcGameState::StaticClass();   // notes: So widgets read the shared timer. KM
}

// ============================================================================
// 2) BeginPlay
// notes: Create LocalPlayer #2 once. I did this because editor “Number of Players”
//        does not carry to packaged build. UI stays in PlayerController. KM
// ============================================================================
void AArcGameMode::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArcGameMode] World is null at BeginPlay() – skip LP#2."));
        return;
    }

    const int32 ExistingPCs = World->GetNumPlayerControllers();
    if (ExistingPCs >= 2)
    {
        UE_LOG(LogTemp, Log, TEXT("[ArcGameMode] %d PC(s) present – skip LP#2."), ExistingPCs);
        return;
    }

    // notes: ControllerId 0 = first view, I request ControllerId 1 for split-screen. KM
    if (UGameplayStatics::CreatePlayer(World, /*ControllerId*/ 1, /*bSpawnPawn*/ true))
    {
        UE_LOG(LogTemp, Log, TEXT("[ArcGameMode] LocalPlayer#2 created."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArcGameMode] Failed to create LocalPlayer#2."));
    }
}

// ============================================================================
// 3) Helpers
// notes: Check both Actor Tags and PlayerStartTag (case-insensitive). I did this
//        because now we can use either field. Used by: spawn selection. KM
// ============================================================================
static bool MatchesWantedTag(AActor* Start, const FName& WantedTag)
{
    if (!IsValid(Start)) return false;

    // Actor Tags path
    for (const FName& Tag : Start->Tags)
    {
        if (Tag.IsEqual(WantedTag, ENameCase::IgnoreCase))
        {
            return true;
        }
    }

    // PlayerStartTag property path
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
// 4) PlayerStart selection
// notes: Map a stable per-player index to tags: 0→P1, 1→P2. I switched to
//        PlayerState->GetPlayerId() because LocalPlayer->ControllerId was 0 for both
//        during spawn. Used by: engine when spawning players. KM
// ============================================================================
AActor* AArcGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    AActor* Fallback = Super::ChoosePlayerStart_Implementation(Player); // notes: Safety if tags missing. KM

    // --- stable index (prefer PlayerState id, then LocalPlayer id, then guess) ---
    int32 Index = -1;

    if (APlayerState* PS = Player ? Player->GetPlayerState<APlayerState>() : nullptr)
    {
        Index = PS->GetPlayerId(); // notes: Works on server during spawn; fixed both-as-P1 bug. KM
    }

    if (Index < 0)
    {
        if (const APlayerController* PC = Cast<APlayerController>(Player))
        {
            if (const ULocalPlayer* LP = PC->GetLocalPlayer())
            {
                Index = LP->GetControllerId(); // notes: Fallback; sometimes null in GM. KM
            }
        }
    }

    if (Index < 0)
    {
        const int32 NumPCs = GetWorld() ? GetWorld()->GetNumPlayerControllers() : 1;
        Index = (NumPCs > 1) ? 1 : 0; // notes: Last resort guess. KM
    }

    const FName WantedPrimary = (Index % 2 == 0) ? FName(TEXT("P1")) : FName(TEXT("P2"));
    const FName WantedSecondary = (Index % 2 == 0) ? FName(TEXT("P2")) : FName(TEXT("P1"));

    UE_LOG(LogTemp, Log, TEXT("[GM] ChoosePlayerStart: Index=%d, want [%s] then [%s]"),
        Index, *WantedPrimary.ToString(), *WantedSecondary.ToString());

    // --- scan starts and log what we have (helps level setup) ---
    TArray<AActor*> Starts;
    UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), Starts);

    UE_LOG(LogTemp, Log, TEXT("[GM] ChoosePlayerStart: Found %d PlayerStarts."), Starts.Num());
    for (AActor* S : Starts)
    {
        const APlayerStart* PS = Cast<APlayerStart>(S);
        FString ActorTags;
        for (const FName& Tag : S->Tags) { ActorTags += Tag.ToString() + TEXT(" "); }
        const FString PSTag = PS ? PS->PlayerStartTag.ToString() : TEXT("<none>");
        UE_LOG(LogTemp, Log, TEXT("  - %s | ActorTags=[%s] | PlayerStartTag=%s"),
            *GetNameSafe(S), *ActorTags, *PSTag);
    }

    // --- choose PRIMARY tag, then SECONDARY, else fallback ---
    for (AActor* Start : Starts)
    {
        if (MatchesWantedTag(Start, WantedPrimary))
        {
            UE_LOG(LogTemp, Log, TEXT("[GM] ChoosePlayerStart: PRIMARY %s -> %s"),
                *WantedPrimary.ToString(), *GetNameSafe(Start));
            return Start;
        }
    }

    for (AActor* Start : Starts)
    {
        if (MatchesWantedTag(Start, WantedSecondary))
        {
            UE_LOG(LogTemp, Warning, TEXT("[GM] ChoosePlayerStart: PRIMARY missing, using SECONDARY %s -> %s"),
                *WantedSecondary.ToString(), *GetNameSafe(Start));
            return Start;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[GM] ChoosePlayerStart: no tagged starts, using fallback"));
    return Fallback;
}
