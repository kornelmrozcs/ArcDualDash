#pragma once

// ============================================================================
// RacePlayerController.h
// purpose: spawn one HUD widget per LocalPlayer and attach it to that player's
//          split-screen sub-viewport. Keeps a UPROPERTY ref to avoid GC.
// used by: BP_RacePC
// ============================================================================

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "RacePlayerController.generated.h"

// Forward declarations (for cleaner includes)
class UWBP_RaceHUD;

/**
 * Per-player controller that handles local HUD spawn and restart hotkey.
 */
UCLASS()
class ARCDUALDASH_API ARacePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ARacePlayerController(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    /** Spawns and attaches WBP_RaceHUD for this local player */
    void SpawnLocalHUD();

    /** Reloads the current level (used by F12 hotkey) */
    UFUNCTION()
    void HandleRestartHotkey();

public:
    /** The widget class to use for this player's HUD (set in BP_RacePC) */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> HUDWidgetClass;

    /** Instance of the spawned HUD (kept alive via UPROPERTY) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    class UUserWidget* HUDWidgetInstance = nullptr;
};
