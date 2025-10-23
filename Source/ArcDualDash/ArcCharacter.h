#pragma once

// ============================================================================
// ArcCharacter.h
// notes: Top-down pawn. I own camera here, bind Enhanced Input here, and expose
//        simple stats so pickups and UI can use them. Used by: ArcPlayerController
//        (input actions), Collectable (overlap → stats), HUD widgets (read stats). KM
// ============================================================================

#include "CoreMinimal.h"                 // notes: Core UE types. KM
#include "GameFramework/Character.h"     // notes: Movement/mesh/capsule baseline. KM
#include "InputActionValue.h"            // notes: Enhanced Input value wrapper. KM
#include "ArcCharacter.generated.h"

class UCameraComponent;                  // notes: Forward decl for camera. KM
class USpringArmComponent;               // notes: Forward decl for boom. KM

/**
 * notes: Top-down Character with orthographic camera and grid-like feel.
 *        I map Axis2D to world X/Y and keep camera fixed. KM
 */
UCLASS()
class ARCDUALDASH_API AArcCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AArcCharacter(); // notes: Build camera rig and disable auto-rotation. KM

protected:
    // =========================================================================
    // Lifecycle & Input
    // =========================================================================
    virtual void BeginPlay() override; // notes: Collision sanity + camera enforce. KM
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override; // notes: Bind Enhanced Input via PC getters. KM

    // =========================================================================
    // Input handlers (Enhanced Input)
    // =========================================================================
    void HandleMove(const FInputActionValue& Value); // notes: Axis2D → world X/Y. KM
    void HandleFire(const FInputActionValue& Value); // notes: Placeholder; will spend ammo later. KM
    void HandleDash(const FInputActionValue& Value); // notes: Placeholder; uses boost pattern later. KM

    // =========================================================================
    // Overlap (collectables etc.)
    // =========================================================================
    UFUNCTION()
    void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult); // notes: Debug print so I know overlaps fire; Collectable reacts on its side. KM

public:
    virtual void Tick(float DeltaSeconds) override; // notes: Free for future VFX; no logic now. KM

    // =========================================================================
    // Camera (owned in C++ so BP can’t accidentally add a second camera)
    // =========================================================================
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* SpringArm = nullptr; // notes: Fixed top-down aim; no collision test. KM

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* Camera = nullptr;       // notes: Orthographic, constant width for split-screen. KM

    // =========================================================================
    // Movement / Power-Up (used by Boost pickup and Dash)
    // =========================================================================
    friend class AArcPlayerController;        // notes: PC reads actions; not touching internals. KM

    UPROPERTY(EditAnywhere, Category = "PowerUp")
    float BaseMaxWalkSpeed = 600.f;           // notes: I cache base speed to restore after boost. KM

    UPROPERTY(EditAnywhere, Category = "PowerUp")
    float BoostedMaxWalkSpeed = 1000.f;       // notes: Used during boost/dash. KM

    FTimerHandle PowerUpTimer;                 // notes: Revert timer for boost. KM

    void StartTimedPowerUp(float DurationSeconds = 5.f); // notes: Set boosted speed, schedule revert. KM
    void EndTimedPowerUp();                                // notes: Restore base speed. KM

    // =========================================================================
    // Stats (used by HUD + pickups)
    // =========================================================================
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 MaxHealth = 100;                     // notes: Upper clamp for health. KM

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 Health = 100;                        // notes: HUD reads; pickups modify. KM

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 MaxAmmo = 100;                       // notes: Upper clamp for ammo. KM

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
    int32 Ammo = 0;                            // notes: HUD reads; Fire will consume. KM

    UFUNCTION(BlueprintCallable, Category = "Stats")
    int32 AddHealth(int32 Delta);              // notes: Clamp [0..MaxHealth]; return new. KM

    UFUNCTION(BlueprintCallable, Category = "Stats")
    int32 AddAmmo(int32 Delta);                // notes: Clamp [0..MaxAmmo]; return new. KM
};
