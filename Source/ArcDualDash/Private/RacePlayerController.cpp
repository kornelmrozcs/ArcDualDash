// ============================================================================
// RacePlayerController.cpp
// notes: per-player HUD owner. I spawn WBP_RaceHUD once for my LocalPlayer and
//        attach it to the correct split-screen pane via AddViewportWidgetForPlayer.
// ============================================================================
#include "RacePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"   // notes: OpenLevel

ARacePlayerController::ARacePlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // notes: keep ctor light; HUD class is set in BP_RacePC
}
void ARacePlayerController::BeginPlay()
{
    Super::BeginPlay();
    SpawnLocalHUD();
}

void ARacePlayerController::SpawnLocalHUD()
{
    // notes: defer a frame so LocalPlayer + split panes are fully ready
    FTimerHandle H;
    GetWorldTimerManager().SetTimer(H, [this]()
        {
            ULocalPlayer* LP = GetLocalPlayer();
            UGameViewportClient* GVC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;

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

            // create + own by THIS controller
            UUserWidget* W = CreateWidget<UUserWidget>(this, HUDWidgetClass);
            if (!W)
            {
                UE_LOG(LogTemp, Warning, TEXT("[RacePC] CreateWidget failed on %s"), *GetName());
                return;
            }
            W->SetOwningPlayer(this);

            // attach to THIS player's split-screen sub-viewport (UE5.5 returns void)
            GVC->AddViewportWidgetForPlayer(LP, W->TakeWidget(), /*ZOrder=*/100);

            // keep reference so GC won’t collect
            HUDWidgetInstance = W;

            // notes: force placement per-player so each HUD is visible in its own pane
            const int32 ControllerId = LP->GetControllerId();

            if (ControllerId == 0)
            {
                // P1: top-center (works great for horizontal / top-bottom split)
                W->SetAnchorsInViewport(FAnchors(0.5f, 0.f, 0.5f, 0.f));
                W->SetAlignmentInViewport(FVector2D(0.5f, 0.f));
                W->SetPositionInViewport(FVector2D(0.f, 16.f), /*bRemoveDPIScale=*/false);
            }
            else
            {
                // P2: bottom-center
                W->SetAnchorsInViewport(FAnchors(0.5f, 1.f, 0.5f, 1.f));
                W->SetAlignmentInViewport(FVector2D(0.5f, 1.f));
                W->SetPositionInViewport(FVector2D(0.f, -16.f), /*bRemoveDPIScale=*/false);
            }

            UE_LOG(LogTemp, Log, TEXT("[RacePC] HUD added for ControllerId=%d"), ControllerId);
        },
        0.10f, /*Loop=*/false);
}

// ============================================================================
// bind F12 → RestartLevel 
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
    if (ControllerId != 0)
    {
        return; 
    }

    InputComponent->BindKey(EKeys::F12, IE_Pressed, this, &ARacePlayerController::HandleRestartHotkey);
    UE_LOG(LogTemp, Log, TEXT("[RacePC] Bound F12 → RestartLevel (ControllerId=0)"));
}

// ============================================================================
// RestartLevel: reload the current map 
// ============================================================================
void ARacePlayerController::HandleRestartHotkey()
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FName LevelName(*World->GetName());
    // notes: OpenLevel resets everything; GameMode will recreate split-screen etc.
    UGameplayStatics::OpenLevel(this, LevelName, /*bAbsolute*/true);
}
