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

    // notes: simple health/ammo store so pickups can modify them
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 MaxHealth = 100;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 Health = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 MaxAmmo = 100;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 Ammo = 0;

    // notes: adjust with clamping; returns new value
    UFUNCTION(BlueprintCallable, Category = "Stats")
    int32 AddHealth(int32 Delta);

    UFUNCTION(BlueprintCallable, Category = "Stats")
    int32 AddAmmo(int32 Delta);

};
