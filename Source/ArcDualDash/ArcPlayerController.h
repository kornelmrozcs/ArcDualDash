#pragma once

// ============================================================================
// ArcPlayerControler.h
// notes: I attach the right input mapping per LocalPlayer, expose IA_* getters
//        so the pawn binds cleanly, and spawn per-player HUD widgets. I also
//        proxy arrow-key input to Player 2 when only keyboard is available. KM
// ============================================================================

// Engine
#include "CoreMinimal.h"                    // notes: Core UE types. KM
#include "GameFramework/PlayerController.h" // notes: Base for input/view/HUD owner. KM
#include "Blueprint/UserWidget.h"           // notes: I spawn widgets here. KM

// Enhanced Input used in this header
#include "InputActionValue.h"               // notes: Wrapper for action values. KM
#include "InputMappingContext.h"            // notes: IMC assets for mappings. KM
#include "InputAction.h"                    // notes: IA assets for actions. KM

// MUST BE LAST
#include "ArcPlayerController.generated.h"

/**
 * notes: Adds IMC per LocalPlayer (P1 WASD / P2 Arrows), exposes IA getters for the pawn,
 *        and spawns timer/score HUD per viewport. I added the keyboard proxy because
 *        we play local split-screen on one keyboard. Used by: ArcCharacter (binds input),
 *        W_RoundTimer & W_PlayerScore (HUD). KM
 */
UCLASS()
class ARCDUALDASH_API AArcPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    // notes: Load IMC/IA assets once so all instances share the same data. KM
    AArcPlayerController(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void BeginPlay() override; // notes: Add mapping contexts + spawn HUD for this LocalPlayer. KM

    // --- Base Mapping Contexts (added per LocalPlayer) ---
    UPROPERTY() TObjectPtr<UInputMappingContext> IMC_Player1 = nullptr;  // notes: P1 WASD layout. KM
    UPROPERTY() TObjectPtr<UInputMappingContext> IMC_Player2 = nullptr;  // notes: P2 Arrow layout. KM

    // --- Base Actions (shared) ---
    UPROPERTY() TObjectPtr<UInputAction> IA_Move = nullptr;  
    UPROPERTY() TObjectPtr<UInputAction> IA_Fire = nullptr;  
    UPROPERTY() TObjectPtr<UInputAction> IA_Dash = nullptr;  

    // --- Keyboard proxy for P2 (added only on ControllerId==0) ---
    UPROPERTY() TObjectPtr<UInputMappingContext> IMC_KeyboardProxy_P2 = nullptr; // notes: Arrow keys routed to Player 2. KM
    UPROPERTY() TObjectPtr<UInputAction>        IA_Move_P2 = nullptr;            
    UPROPERTY() TObjectPtr<UInputAction>        IA_Fire_P2 = nullptr;            
    UPROPERTY() TObjectPtr<UInputAction>        IA_Dash_P2 = nullptr;            

    // --- Handlers for EnhancedInput::BindAction  ---
    UFUNCTION() void OnP2Move(const FInputActionValue& Value); // notes: Reads arrow axis and forwards to Player 2 pawn. KM
    UFUNCTION() void OnP2Fire(const FInputActionValue& Value); 
    UFUNCTION() void OnP2Dash(const FInputActionValue& Value); 

public:
    // --- Read-only accessors for pawn binding   ---
    const UInputAction* GetIA_Move() const { return IA_Move; } // notes: ArcCharacter uses these in SetupPlayerInputComponent. KM
    const UInputAction* GetIA_Fire() const { return IA_Fire; } 
    const UInputAction* GetIA_Dash() const { return IA_Dash; } 

    // --- Proxy helpers (called by OnP2*; public by design so tests can call them) ---
    void Proxy_P2_Move(const FVector2D& Axis); // notes: Forwards movement to Player 2 pawn. KM
    void Proxy_P2_Fire();                      
    void Proxy_P2_Dash();                      

    // --- HUD: Timer ---
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> TimerWidgetClass; // notes: Set in BP_ArcPlayerController per our pattern. KM

    UPROPERTY()
    class UUserWidget* TimerWidgetInstance = nullptr; // notes: Keep ref to avoid GC. KM

    // --- HUD: Score ---
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> ScoreWidgetClass; // notes: Same pattern as timer. KM

    UPROPERTY()
    class UUserWidget* ScoreWidgetInstance = nullptr; // notes: Keep ref to avoid GC. KM
};
