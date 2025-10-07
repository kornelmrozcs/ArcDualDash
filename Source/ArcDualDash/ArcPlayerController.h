#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ArcPlayerController.generated.h"

UCLASS()
class ARCDUALDASH_API AArcPlayerController : public APlayerController
{
    GENERATED_BODY()
protected:
    virtual void BeginPlay() override;
};
