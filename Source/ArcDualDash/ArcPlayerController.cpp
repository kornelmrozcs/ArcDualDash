#include "ArcPlayerController.h"

#include "EnhancedInputSubsystems.h"   // UEnhancedInputLocalPlayerSubsystem
#include "EnhancedInputComponent.h"    // UEnhancedInputComponent
#include "UObject/ConstructorHelpers.h"

#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CanvasPanelSlot.h"

#include "GameFramework/Character.h"
#include "ArcCharacter.h"

// notes: load assets in ctor (paths copied from Content Browser References)
AArcPlayerController::AArcPlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // --- Base IMCs (P1/P2) ---
    {
        static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC1Ref(
            TEXT("/Game/Input/Contexts/IMC_Player1.IMC_Player1"));
        if (IMC1Ref.Succeeded()) { IMC_Player1 = IMC1Ref.Object; }

        static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC2Ref(
            TEXT("/Game/Input/Contexts/IMC_Player2.IMC_Player2"));
        if (IMC2Ref.Succeeded()) { IMC_Player2 = IMC2Ref.Object; }
    }

    // --- Base Actions ---
    {
        static ConstructorHelpers::FObjectFinder<UInputAction> MoveRef(
            TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
        if (MoveRef.Succeeded()) { IA_Move = MoveRef.Object; }

        static ConstructorHelpers::FObjectFinder<UInputAction> FireRef(
            TEXT("/Game/Input/Actions/IA_Fire.IA_Fire"));
        if (FireRef.Succeeded()) { IA_Fire = FireRef.Object; }

        static ConstructorHelpers::FObjectFinder<UInputAction> DashRef(
            TEXT("/Game/Input/Actions/IA_Dash.IA_Dash"));
        if (DashRef.Succeeded()) { IA_Dash = DashRef.Object; }
    }

    // --- Keyboard proxy for P2 (used only on ControllerId==0) ---
    {
        static ConstructorHelpers::FObjectFinder<UInputMappingContext> ProxyIMCRef(
            TEXT("/Game/Input/Contexts/IMC_KeyboardProxy_P2.IMC_KeyboardProxy_P2"));
        if (ProxyIMCRef.Succeeded()) { IMC_KeyboardProxy_P2 = ProxyIMCRef.Object; }

        static ConstructorHelpers::FObjectFinder<UInputAction> P2MoveRef(
            TEXT("/Game/Input/Actions/IA_Move_P2.IA_Move_P2"));
        if (P2MoveRef.Succeeded()) { IA_Move_P2 = P2MoveRef.Object; }

        static ConstructorHelpers::FObjectFinder<UInputAction> P2FireRef(
            TEXT("/Game/Input/Actions/IA_Fire_P2.IA_Fire_P2"));
        if (P2FireRef.Succeeded()) { IA_Fire_P2 = P2FireRef.Object; }

        static ConstructorHelpers::FObjectFinder<UInputAction> P2DashRef(
            TEXT("/Game/Input/Actions/IA_Dash_P2.IA_Dash_P2"));
        if (P2DashRef.Succeeded()) { IA_Dash_P2 = P2DashRef.Object; }
    }
}

void AArcPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // notes: attach the right IMC to the right LocalPlayer; add proxy on ControllerId==0
    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
        {
            const int32 ControllerId = LP->GetControllerId();

            // notes: clear stale contexts (PIE restarts can duplicate)
            Subsystem->ClearAllMappings();

            // --- Base IMC per LocalPlayer ---
            if (ControllerId == 0 && IMC_Player1)
            {
                Subsystem->AddMappingContext(IMC_Player1, /*Priority*/0);
                UE_LOG(LogTemp, Log, TEXT("[ArcPC] Added IMC_Player1 on ControllerId=0"));
            }
            else if (ControllerId == 1 && IMC_Player2)
            {
                Subsystem->AddMappingContext(IMC_Player2, /*Priority*/0);
                UE_LOG(LogTemp, Log, TEXT("[ArcPC] Added IMC_Player2 on ControllerId=1"));
            }

            // --- Keyboard proxy for P2 (only on ControllerId==0) ---
            if (ControllerId == 0 && IMC_KeyboardProxy_P2)
            {
                Subsystem->AddMappingContext(IMC_KeyboardProxy_P2, /*Priority*/1);

                if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
                {
                    if (IA_Move_P2)
                        EIC->BindAction(IA_Move_P2, ETriggerEvent::Triggered, this, &AArcPlayerController::OnP2Move);
                    if (IA_Fire_P2)
                        EIC->BindAction(IA_Fire_P2, ETriggerEvent::Started, this, &AArcPlayerController::OnP2Fire);
                    if (IA_Dash_P2)
                        EIC->BindAction(IA_Dash_P2, ETriggerEvent::Started, this, &AArcPlayerController::OnP2Dash);

                    UE_LOG(LogTemp, Log, TEXT("[ArcPC] Proxy bindings for P2 added on ControllerId=0"));
                }
            }
        }
    }

    // notes: robust split-screen spawn: attach timer HUD to THIS local player's viewport
    {
        FTimerHandle H;
        FTimerDelegate D;
        D.BindLambda([this]()
            {
                if (!TimerWidgetClass)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[PC] TimerWidgetClass is NULL on %s"), *GetName());
                    return;
                }

                UUserWidget* W = CreateWidget<UUserWidget>(GetWorld(), TimerWidgetClass);
                if (!W)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[PC] CreateWidget failed on %s"), *GetName());
                    return;
                }

                W->SetOwningPlayer(this);

                if (UGameViewportClient* GVC = GetWorld()->GetGameViewport())
                {
                    if (ULocalPlayer* LP2 = GetLocalPlayer())
                    {
                        // UE 5.4: returns void
                        GVC->AddViewportWidgetForPlayer(LP2, W->TakeWidget(), /*ZOrder=*/100);
                        UE_LOG(LogTemp, Log, TEXT("[PC] AddViewportWidgetForPlayer done for %s"), *GetName());
                    }
                    else
                    {
                        // fallback (shouldn't happen in split-screen)
                        W->AddToPlayerScreen(100);
                        UE_LOG(LogTemp, Log, TEXT("[PC] Fallback AddToPlayerScreen for %s"), *GetName());
                    }
                }

                // notes: keep reference to prevent GC
                TimerWidgetInstance = W;

                // notes: force top-center placement
                W->SetAnchorsInViewport(FAnchors(0.5f, 0.f, 0.5f, 0.f));
                W->SetAlignmentInViewport(FVector2D(0.5f, 0.f));
                W->SetPositionInViewport(FVector2D(0.f, 12.f), false);

                UE_LOG(LogTemp, Log, TEXT("[PC] Timer HUD ready for %s"), *GetName());
            });

        GetWorldTimerManager().SetTimer(H, D, /*Rate=*/0.10f, /*bLoop=*/false);
    }
    // --- Score HUD per player (mirrors timer pattern) ---
// notes: spawn after a tiny delay so LocalPlayer/GVC are ready (same as timer)
    {
        FTimerHandle H;
        FTimerDelegate D;
        D.BindLambda([this]()
            {
                if (!ScoreWidgetClass)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[PC] ScoreWidgetClass is NULL on %s"), *GetName());
                    return;
                }

                UUserWidget* W = CreateWidget<UUserWidget>(GetWorld(), ScoreWidgetClass);
                if (!W)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[PC] CreateWidget(Score) failed on %s"), *GetName());
                    return;
                }

                W->SetOwningPlayer(this);

                if (UGameViewportClient* GVC = GetWorld()->GetGameViewport())
                {
                    if (ULocalPlayer* LP2 = GetLocalPlayer())
                    {
                        // UE 5.4: returns void
                        GVC->AddViewportWidgetForPlayer(LP2, W->TakeWidget(), /*ZOrder=*/90);
                        UE_LOG(LogTemp, Log, TEXT("[PC] Score AddViewportWidgetForPlayer done for %s"), *GetName());
                    }
                    else
                    {
                        // notes: fallback (shouldn't happen in split-screen)
                        W->AddToPlayerScreen(90);
                        UE_LOG(LogTemp, Log, TEXT("[PC] Score Fallback AddToPlayerScreen for %s"), *GetName());
                    }
                }

                // notes: keep reference so GC won't collect the widget
                ScoreWidgetInstance = W;

                // notes: force per-player placement in viewport
                int32 ControllerIdForLayout = 0;
                if (ULocalPlayer* LPx = GetLocalPlayer())
                {
                    ControllerIdForLayout = LPx->GetControllerId();
                }

                if (ControllerIdForLayout == 0)
                {
                    // P1: top-left
                    W->SetAnchorsInViewport(FAnchors(0.f, 0.f, 0.f, 0.f));
                    W->SetAlignmentInViewport(FVector2D(0.f, 0.f));
                    W->SetPositionInViewport(FVector2D(16.f, 16.f), false);
                }
                else
                {
                    // P2: top-right
                    W->SetAnchorsInViewport(FAnchors(1.f, 0.f, 1.f, 0.f));
                    W->SetAlignmentInViewport(FVector2D(1.f, 0.f));
                    W->SetPositionInViewport(FVector2D(-16.f, 16.f), false);
                }

                UE_LOG(LogTemp, Log, TEXT("[PC] Score HUD ready for %s (ControllerId=%d)"),
                    *GetName(), ControllerIdForLayout);
            });

        GetWorldTimerManager().SetTimer(H, D, /*Rate=*/0.10f, /*bLoop=*/false);
    }


    // notes: game-only input, hide cursor
    FInputModeGameOnly Mode;
    SetInputMode(Mode);
    bShowMouseCursor = false;
}


// --- Proxy: handlers bound to IA_*_P2, then routed to Player #1 pawn ---

void AArcPlayerController::OnP2Move(const FInputActionValue& Value)
{
    Proxy_P2_Move(Value.Get<FVector2D>());
}

void AArcPlayerController::OnP2Fire(const FInputActionValue& /*Value*/)
{
    Proxy_P2_Fire();
}

void AArcPlayerController::OnP2Dash(const FInputActionValue& /*Value*/)
{
    Proxy_P2_Dash();
}

void AArcPlayerController::Proxy_P2_Move(const FVector2D& Axis)
{
    if (APlayerController* PC2 = UGameplayStatics::GetPlayerController(this, 1))
    {
        if (AArcCharacter* P2Pawn = Cast<AArcCharacter>(PC2->GetPawn()))
        {
            if (!Axis.IsNearlyZero())
            {
                // notes: same mapping as character handler: Y -> +X, X -> +Y (signs handled in pawn)
                P2Pawn->AddMovementInput(FVector::XAxisVector, Axis.Y);
                P2Pawn->AddMovementInput(FVector::YAxisVector, Axis.X);
            }
        }
    }
}

void AArcPlayerController::Proxy_P2_Fire()
{
    if (APlayerController* PC2 = UGameplayStatics::GetPlayerController(this, 1))
        if (AArcCharacter* P2Pawn = Cast<AArcCharacter>(PC2->GetPawn()))
            P2Pawn->HandleFire(FInputActionValue()); // notes: reuse placeholder
}

void AArcPlayerController::Proxy_P2_Dash()
{
    if (APlayerController* PC2 = UGameplayStatics::GetPlayerController(this, 1))
        if (AArcCharacter* P2Pawn = Cast<AArcCharacter>(PC2->GetPawn()))
            P2Pawn->HandleDash(FInputActionValue()); // notes: reuse placeholder
}
