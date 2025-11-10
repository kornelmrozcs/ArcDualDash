#pragma once

// ============================================================================
// RacePlayerController.h
// purpose: spawn one HUD widget per LocalPlayer and attach it to that player's
//          split-screen sub-viewport. I keep a UPROPERTY ref to avoid GC.
// used by: BP_RacePC 
// ============================================================================
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "RacePlayerController.generated.h"

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
    // notes: create and attach WBP_RaceHUD for THIS LocalPlayer
    void SpawnLocalHUD();

public:
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<class UUserWidget> HUDWidgetClass;

    UPROPERTY()
    class UUserWidget* HUDWidgetInstance = nullptr;

    // notes: reload current level (used by F12)
    UFUNCTION()
    void HandleRestartHotkey();
};
