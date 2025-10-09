#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ArcCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * ArcCharacter (top-down)
 * notes: binds Enhanced Input, maps Axis2D to world X/Y, owns camera in C++
 */
UCLASS()
class ARCDUALDASH_API AArcCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AArcCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void HandleMove(const FInputActionValue& Value);
    void HandleFire(const FInputActionValue& Value);
    void HandleDash(const FInputActionValue& Value);

public:
    virtual void Tick(float DeltaSeconds) override;

    // notes: camera & boom are UPROPERTY so child BP sees them as (Inherited) and won't add duplicates
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* SpringArm = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* Camera = nullptr;

    friend class AArcPlayerController;
};
