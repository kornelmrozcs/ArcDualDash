#pragma once

// Engine
#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// We use FInputActionValue in the header
#include "InputActionValue.h"

// MUST BE LAST
#include "ArcCharacter.generated.h"

/**
 * ArcCharacter (top-down)
 * - Binds Enhanced Input (via actions provided by owning AArcPlayerController).
 * - Maps Axis2D to world X/Y (top-down, camera fixed).
 */
UCLASS()
class ARCDUALDASH_API AArcCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AArcCharacter();

protected:
    virtual void BeginPlay() override;

    // Bind Enhanced Input on this pawn (pawn owns movement)
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // Handlers — tiny, readable
    void HandleMove(const FInputActionValue& Value);  // Axis2D -> AddMovementInput
    void HandleFire(const FInputActionValue& Value);  // Bool   -> placeholder
    void HandleDash(const FInputActionValue& Value);  // Bool   -> placeholder

public:
    virtual void Tick(float DeltaSeconds) override;

    // Allow controller to call placeholders directly for proxy routing
    friend class AArcPlayerController;
};
