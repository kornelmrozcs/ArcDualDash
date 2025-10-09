#pragma once

// Engine
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

// Enhanced Input types used in this header
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"

// MUST BE LAST
#include "ArcPlayerController.generated.h"

/**
 * ArcPlayerController
 * notes: adds IMC per LocalPlayer (P1 vs P2)
 * notes: on ControllerId==0 also binds a keyboard proxy for Player #1 (P2 on arrows)
 * notes: exposes getters for IA_* so pawn can bind without touching internals
 */
UCLASS()
class ARCDUALDASH_API AArcPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    // notes: load IMC/IA assets once here (centralize input data)
    AArcPlayerController(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void BeginPlay() override;

    // --- Base Mapping Contexts (added per LocalPlayer) ---
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

    // --- Handlers for EnhancedInput::BindAction (no lambdas; UE5.4-friendly) ---
    UFUNCTION() void OnP2Move(const FInputActionValue& Value);
    UFUNCTION() void OnP2Fire(const FInputActionValue& Value);
    UFUNCTION() void OnP2Dash(const FInputActionValue& Value);

public:
    // --- Read-only accessors for pawn binding ---
    const UInputAction* GetIA_Move() const { return IA_Move; }
    const UInputAction* GetIA_Fire() const { return IA_Fire; }
    const UInputAction* GetIA_Dash() const { return IA_Dash; }

    // --- Proxy helpers (called by OnP2*; kept public intentionally) ---
    void Proxy_P2_Move(const FVector2D& Axis);
    void Proxy_P2_Fire();
    void Proxy_P2_Dash();
};
