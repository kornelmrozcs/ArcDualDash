// ============================================================================
// RacePlayerController.cpp
// notes: per-player HUD owner. Spawns WBP_RaceHUD once for this LocalPlayer and
//        attaches it to the correct split-screen pane. Automatically sets
//        OwningCar reference inside the HUD so Blueprints don't need to.
// ============================================================================

#include "RacePlayerController.h"
#include "MyCar.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// ============================================================================
// Constructor / BeginPlay
// ============================================================================
ARacePlayerController::ARacePlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // HUDWidgetClass is set in BP_RacePC
}

void ARacePlayerController::BeginPlay()
{
    Super::BeginPlay();

    // --- Assign player index for splitscreen / leaderboard ---
    if (PlayerIndex == 0)
    {
        if (ULocalPlayer* LP = GetLocalPlayer())
        {
            PlayerIndex = LP->GetControllerId() + 1; // 1-based for display
            UE_LOG(LogTemp, Log, TEXT("[RacePC] Assigned PlayerIndex = %d for %s"), PlayerIndex, *GetName());
        }
    }

    SpawnLocalHUD();
}

// ============================================================================
// SpawnLocalHUD
// ============================================================================
void ARacePlayerController::SpawnLocalHUD()
{
    FTimerHandle H;
    GetWorldTimerManager().SetTimer(H, [this]()
        {
            ULocalPlayer* LP = GetLocalPlayer();
            UGameViewportClient* GVC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;

            // --- Ensure PlayerIndex is valid (late correction) ---
            if (PlayerIndex == 0 && LP)
            {
                PlayerIndex = LP->GetControllerId() + 1;
                UE_LOG(LogTemp, Log, TEXT("[RacePC] Late PlayerIndex correction = %d for %s"), PlayerIndex, *GetName());
            }

            if (!HUDWidgetClass)
            {
                UE_LOG(LogTemp, Warning, TEXT("[RacePC] HUDWidgetClass is null on %s"), *GetName());
                return;
            }
            if (!LP || !GVC)
            {
                UE_LOG(LogTemp, Warning, TEXT("[RacePC] Missing LocalPlayer/GameViewport on %s"), *GetName());
                return;
            }

            // --- Create widget instance ---
            UUserWidget* W = CreateWidget<UUserWidget>(this, HUDWidgetClass);
            if (!W)
            {
                UE_LOG(LogTemp, Warning, TEXT("[RacePC] CreateWidget failed on %s"), *GetName());
                return;
            }
            W->SetOwningPlayer(this);

            // --- Automatically assign OwningCar variable in the BP widget ---
            if (AMyCar* MyCar = Cast<AMyCar>(GetPawn()))
            {
                if (UWidgetBlueprintGeneratedClass* WidgetBPClass = Cast<UWidgetBlueprintGeneratedClass>(W->GetClass()))
                {
                    if (FObjectPropertyBase* Prop = FindFProperty<FObjectPropertyBase>(WidgetBPClass, FName(TEXT("OwningCar"))))
                    {
                        Prop->SetObjectPropertyValue_InContainer(W, MyCar);
                        UE_LOG(LogTemp, Log, TEXT("[RacePC] Assigned OwningCar = %s for %s"), *MyCar->GetName(), *GetName());
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[RacePC] Could not find 'OwningCar' variable in %s"), *W->GetName());
                    }
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[RacePC] No MyCar pawn found for %s"), *GetName());
            }

            // --- Add to correct split-screen region ---
            GVC->AddViewportWidgetForPlayer(LP, W->TakeWidget(), /*ZOrder=*/100);
            HUDWidgetInstance = W;

            // --- Adjust screen position depending on player ---
            const int32 ControllerId = LP->GetControllerId();
            if (ControllerId == 0)
            {
                W->SetAnchorsInViewport(FAnchors(0.5f, 0.f, 0.5f, 0.f));
                W->SetAlignmentInViewport(FVector2D(0.5f, 0.f));
                W->SetPositionInViewport(FVector2D(0.f, 16.f), false);
            }
            else
            {
                W->SetAnchorsInViewport(FAnchors(0.5f, 1.f, 0.5f, 1.f));
                W->SetAlignmentInViewport(FVector2D(0.5f, 1.f));
                W->SetPositionInViewport(FVector2D(0.f, -16.f), false);
            }

            UE_LOG(LogTemp, Log, TEXT("[RacePC] HUD added for ControllerId=%d"), ControllerId);
        },
        0.10f, false);
}

// ============================================================================
// Bind F12 → RestartLevel (only for Player 1)
// ============================================================================
void ARacePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (!InputComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("[RacePC] No InputComponent on %s"), *GetName());
        return;
    }

    const ULocalPlayer* LP = GetLocalPlayer();
    const int32 ControllerId = LP ? LP->GetControllerId() : 0;

    // Only player 1 gets the restart hotkey
    if (ControllerId == 0)
    {
        InputComponent->BindKey(EKeys::F12, IE_Pressed, this, &ARacePlayerController::HandleRestartHotkey);
        UE_LOG(LogTemp, Log, TEXT("[RacePC] Bound F12 → RestartLevel (ControllerId=0)"));
    }
}

// ============================================================================
// RestartLevel (reload current map)
// ============================================================================
void ARacePlayerController::HandleRestartHotkey()
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FName LevelName(*World->GetName());
    UGameplayStatics::OpenLevel(this, LevelName, true);
}
