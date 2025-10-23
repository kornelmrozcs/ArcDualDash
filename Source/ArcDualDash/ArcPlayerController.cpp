// ============================================================================
// ArcPlayerController.cpp
// notes: Per-player input setup and per-viewport HUD spawn. I also proxy arrow
//        input to Player 2 so two players can share one keyboard. KM
// ============================================================================

#include "ArcPlayerController.h"

#include "EnhancedInputSubsystems.h"   // notes: UEnhancedInputLocalPlayerSubsystem for IMC attach. KM
#include "EnhancedInputComponent.h"    // notes: Bind Enhanced Input actions. KM
#include "UObject/ConstructorHelpers.h"// notes: Load IMC/IA assets by path once. KM

#include "Blueprint/UserWidget.h"      // notes: CreateWidget + widget types. KM
#include "Engine/GameViewportClient.h" // notes: AddViewportWidgetForPlayer. KM
#include "Engine/LocalPlayer.h"        // notes: Get LocalPlayer/ControllerId. KM
#include "Kismet/GameplayStatics.h"    // notes: GetPlayerController for proxy. KM
#include "Components/CanvasPanelSlot.h"// notes: Anchor/position helpers if needed. KM

#include "GameFramework/Character.h"   // notes: Pawn base for forwarding. KM
#include "ArcCharacter.h"              // notes: We call pawn handlers in proxy. KM

// ============================================================================
// Ctor: load IMC/IA assets once so BeginPlay can attach them by ControllerId
// ============================================================================
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

    // --- Keyboard proxy for P2 (only used on ControllerId==0) ---
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

// ============================================================================
// BeginPlay: attach mapping for this LocalPlayer and spawn per-viewport HUD
// ============================================================================
void AArcPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Add correct IMC for this LocalPlayer; add proxy on ControllerId==0 because one keyboard. KM
    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
        {
            const int32 ControllerId = LP->GetControllerId();

            // Clear stale contexts (PIE restarts can duplicate). KM
            Subsystem->ClearAllMappings();

            // Base IMC per LocalPlayer. KM
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

            // Keyboard proxy for P2 (only on ControllerId==0). KM
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

    // Timer HUD per LocalPlayer viewport. I add via GameViewport to respect split-screen. KM
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
                        GVC->AddViewportWidgetForPlayer(LP2, W->TakeWidget(), /*ZOrder=*/100); // UE 5.4 returns void. KM
                        UE_LOG(LogTemp, Log, TEXT("[PC] AddViewportWidgetForPlayer done for %s"), *GetName());
                    }
                    else
                    {
                        W->AddToPlayerScreen(100); // Fallback; not expected for split-screen. KM
                        UE_LOG(LogTemp, Log, TEXT("[PC] Fallback AddToPlayerScreen for %s"), *GetName());
                    }
                }

                TimerWidgetInstance = W; // Keep ref so GC won’t collect. KM

                // Force top-center placement for visibility in both sub-views. KM
                W->SetAnchorsInViewport(FAnchors(0.5f, 0.f, 0.5f, 0.f));
                W->SetAlignmentInViewport(FVector2D(0.5f, 0.f));
                W->SetPositionInViewport(FVector2D(0.f, 12.f), false);

                UE_LOG(LogTemp, Log, TEXT("[PC] Timer HUD ready for %s"), *GetName());
            });

        GetWorldTimerManager().SetTimer(H, D, /*Rate=*/0.10f, /*bLoop=*/false);
    }

    // Score HUD per LocalPlayer viewport (same pattern). KM
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
                        GVC->AddViewportWidgetForPlayer(LP2, W->TakeWidget(), /*ZOrder=*/90);
                        UE_LOG(LogTemp, Log, TEXT("[PC] Score AddViewportWidgetForPlayer done for %s"), *GetName());
                    }
                    else
                    {
                        W->AddToPlayerScreen(90);
                        UE_LOG(LogTemp, Log, TEXT("[PC] Score Fallback AddToPlayerScreen for %s"), *GetName());
                    }
                }

                ScoreWidgetInstance = W; // Prevent GC. KM

                // Place per player: P1 left, P2 right. I use anchors to be split-screen safe. KM
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

    // Game-only input, hide cursor. KM
    FInputModeGameOnly Mode;
    SetInputMode(Mode);
    bShowMouseCursor = false;
}

// ============================================================================
// Proxy: handlers bound to IA_*_P2; route to Player 2 pawn
// ============================================================================
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
                // notes: Same mapping as pawn handler: Y → +X, X → +Y (ortho screen → world). KM
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
            P2Pawn->HandleFire(FInputActionValue()); // Reuse pawn placeholder. KM
}

void AArcPlayerController::Proxy_P2_Dash()
{
    if (APlayerController* PC2 = UGameplayStatics::GetPlayerController(this, 1))
        if (AArcCharacter* P2Pawn = Cast<AArcCharacter>(PC2->GetPawn()))
            P2Pawn->HandleDash(FInputActionValue()); // Reuse pawn placeholder. KM
}
