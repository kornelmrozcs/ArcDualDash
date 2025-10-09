#pragma once

// Engine
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

// Enhanced Input types we reference in this header
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"

// MUST BE LAST
#include "ArcPlayerController.generated.h"

/**
 * ArcPlayerController
 * - Adds per-local-player Input Mapping Context (P1 vs P2).
 * - ControllerId==0 also binds a "keyboard proxy" for Player#2 (because keyboard routes to LP#0).
 * - Exposes const getters for IA_* so pawn can bind without poking internals.
 */
UCLASS()
class ARCDUALDASH_API AArcPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    // ctor: we load IMC/IA assets once here (centralize input data)
    AArcPlayerController(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void BeginPlay() override;

    // --- Base IMCs (added per LocalPlayer) ---
    UPROPERTY() TObjectPtr<UInputMappingContext> IMC_Player1 = nullptr;  // P1 (WASD)
    UPROPERTY() TObjectPtr<UInputMappingContext> IMC_Player2 = nullptr;  // P2 (Arrows)

    // --- Base Actions (shared) ---
    UPROPERTY() TObjectPtr<UInputAction> IA_Move = nullptr;
    UPROPERTY() TObjectPtr<UInputAction> IA_Fire = nullptr;
    UPROPERTY() TObjectPtr<UInputAction> IA_Dash = nullptr;

    // --- Keyboard proxy for P2 (added ONLY on ControllerId==0) ---
    UPROPERTY() TObjectPtr<UInputMappingContext> IMC_KeyboardProxy_P2 = nullptr;
    UPROPERTY() TObjectPtr<UInputAction>        IA_Move_P2 = nullptr;
    UPROPERTY() TObjectPtr<UInputAction>        IA_Fire_P2 = nullptr;
    UPROPERTY() TObjectPtr<UInputAction>        IA_Dash_P2 = nullptr;

    // NEW: methods for EnhancedInput::BindAction (no lambdas; keeps 5.4 happy)
    UFUNCTION() void OnP2Move(const FInputActionValue& Value);
    UFUNCTION() void OnP2Fire(const FInputActionValue& Value);
    UFUNCTION() void OnP2Dash(const FInputActionValue& Value);

public:
    // Small, explicit accessors (reason: keep IA_* protected; pawn binds via getters)
    const UInputAction* GetIA_Move() const { return IA_Move; }
    const UInputAction* GetIA_Fire() const { return IA_Fire; }
    const UInputAction* GetIA_Dash() const { return IA_Dash; }

    // Proxy helpers (kept public intentionally): called by our OnP2* methods
    void Proxy_P2_Move(const FVector2D& Axis);
    void Proxy_P2_Fire();
    void Proxy_P2_Dash();
};
