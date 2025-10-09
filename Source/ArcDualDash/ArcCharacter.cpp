#include "ArcCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "ArcPlayerController.h"


AArcCharacter::AArcCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Simple top-down camera rig (independent per player)
    USpringArmComponent* SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 800.f;
    SpringArm->bDoCollisionTest = false;
    SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));

    UCameraComponent* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
}

void AArcCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AArcCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void AArcCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (const AArcPlayerController* PC = Cast<AArcPlayerController>(Controller))
        {
            if (const UInputAction* IA_Move = PC->GetIA_Move())
            {
                EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AArcCharacter::HandleMove);
            }
            if (const UInputAction* IA_Fire = PC->GetIA_Fire())
            {
                EIC->BindAction(IA_Fire, ETriggerEvent::Started, this, &AArcCharacter::HandleFire);
            }
            if (const UInputAction* IA_Dash = PC->GetIA_Dash())
            {
                EIC->BindAction(IA_Dash, ETriggerEvent::Started, this, &AArcCharacter::HandleDash);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[ArcChar] Missing AArcPlayerController -> no input binds."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[ArcChar] Expected EnhancedInputComponent."));
    }
}

void AArcCharacter::HandleMove(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();

    // Top-down mapping:
    // - In IMC, W/Up produce (0, +1) after Swizzle.
    // - Map Axis.Y to world +X, Axis.X to world +Y.
    if (!Axis.IsNearlyZero())
    {
        AddMovementInput(FVector::XAxisVector, Axis.Y);
        AddMovementInput(FVector::YAxisVector, Axis.X);
    }
}

void AArcCharacter::HandleFire(const FInputActionValue& /*Value*/)
{
    // Placeholder: hitscan later.
    UE_LOG(LogTemp, Log, TEXT("[ArcChar] Fire (placeholder)"));
}

void AArcCharacter::HandleDash(const FInputActionValue& /*Value*/)
{
    // Placeholder: sprint/impulse later.
    UE_LOG(LogTemp, Log, TEXT("[ArcChar] Dash (placeholder)"));
}
