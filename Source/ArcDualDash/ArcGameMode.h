#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArcGameMode.generated.h"

UCLASS()
class ARCDUALDASH_API AArcGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AArcGameMode();
    // notes: pick per-player PlayerStart by Actor Tag (P1/P2) in local split-screen
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
protected:
    virtual void BeginPlay() override;
};
