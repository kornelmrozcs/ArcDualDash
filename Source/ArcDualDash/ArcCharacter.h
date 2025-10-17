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
    UFUNCTION()
    void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);




public:
    virtual void Tick(float DeltaSeconds) override;

    // notes: camera & boom are UPROPERTY so child BP sees them as (Inherited) and won't add duplicates
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* SpringArm = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* Camera = nullptr;

    friend class AArcPlayerController;
    // notes: timed speed boost after picking a ring
    UPROPERTY(EditAnywhere, Category = "PowerUp")
    float BaseMaxWalkSpeed = 600.f;

    UPROPERTY(EditAnywhere, Category = "PowerUp")
    float BoostedMaxWalkSpeed = 1000.f;

    FTimerHandle PowerUpTimer;

    // notes: start boost for N seconds (default 5)
    void StartTimedPowerUp(float DurationSeconds = 5.f);

    // notes: revert to base speed
    void EndTimedPowerUp();
};
