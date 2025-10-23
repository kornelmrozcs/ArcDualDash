#pragma once

// ============================================================================
// ArcGameMode.h
// notes: I handle split-screen setup and choose spawn points for each player.
//        Reason: I want both players auto-created and spawned on tagged starts.
//        Used by: level (PlayerStarts with tags P1/P2), PlayerController (HUD). KM
// ============================================================================

#include "CoreMinimal.h"                    // notes: Core UE types. KM
#include "GameFramework/GameModeBase.h"     // notes: Base for game rules and spawn logic. KM
#include "ArcGameMode.generated.h"

UCLASS()
class ARCDUALDASH_API AArcGameMode : public AGameModeBases
{
    GENERATED_BODY()

public:
    AArcGameMode(); // notes: Bind my custom PlayerState/GameState so UI and score work. KM

    // notes: Pick PlayerStart by tag (P1/P2) for split-screen. I added this because
    //        both players were spawning on the same start. Used by: level PlayerStarts. KM
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
    // notes: Create LocalPlayer #2 once. I do it here so packaging behaves like PIE{Play In Editor]. KM
    virtual void BeginPlay() override;
};
