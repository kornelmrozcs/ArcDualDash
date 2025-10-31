#pragma once

// ============================================================================
// RaceGameMode.h
// purpose: split-screen + spawn points for P1/P2.
// why: packaged build won't remember PIE player count; I create LocalPlayer #2.
// used by: level startup; PlayerStart tags ("P1", "P2").  KM
// ============================================================================
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RaceGameMode.generated.h"

UCLASS()
class ARCDUALDASH_API ARaceGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARaceGameMode(); // notes: keep ctor light; defaults via Project Settings. KM

protected:
    virtual void BeginPlay() override; // notes: create LocalPlayer#2 once. KM
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override; // notes: pick by tag. KM

private:
    // notes: helper to compare tags on AActor/APlayerStart. KM
    static bool MatchesWantedTag(AActor* Start, const FName& WantedTag);
};
