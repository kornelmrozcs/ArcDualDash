#include "ArcPlayerController.h"

#include "EnhancedInputSubsystems.h"   // UEnhancedInputLocalPlayerSubsystem
#include "EnhancedInputComponent.h"    // UEnhancedInputComponent
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
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
                    // notes: bind to member methods (no lambdas; UE5.4-friendly)
                    if (IA_Move_P2)
                    {
                        EIC->BindAction(IA_Move_P2, ETriggerEvent::Triggered, this, &AArcPlayerController::OnP2Move);
                    }
                    if (IA_Fire_P2)
                    {
                        EIC->BindAction(IA_Fire_P2, ETriggerEvent::Started, this, &AArcPlayerController::OnP2Fire);
                    }
                    if (IA_Dash_P2)
                    {
                        EIC->BindAction(IA_Dash_P2, ETriggerEvent::Started, this, &AArcPlayerController::OnP2Dash);
                    }

                    UE_LOG(LogTemp, Log, TEXT("[ArcPC] Proxy bindings for P2 added on ControllerId=0"));
                }
            }
        }
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
