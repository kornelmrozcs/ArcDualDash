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
protected:
    virtual void BeginPlay() override;
};
