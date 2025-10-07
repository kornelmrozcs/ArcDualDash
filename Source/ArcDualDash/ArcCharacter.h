#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArcCharacter.generated.h"

UCLASS()
class ARCDUALDASH_API AArcCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AArcCharacter();
protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaSeconds) override;
};
