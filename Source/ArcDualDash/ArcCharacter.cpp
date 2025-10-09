#include "ArcCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "ArcPlayerController.h"
#include "Engine/EngineTypes.h" // ECameraProjectionMode

AArcCharacter::AArcCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // --- camera boom (UPROPERTY) ---
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 1200.f;
    SpringArm->bDoCollisionTest = false;
    SpringArm->SetUsingAbsoluteRotation(true);
    SpringArm->bInheritYaw = false;
    SpringArm->bInheritPitch = false;
    SpringArm->bInheritRoll = false;
    SpringArm->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f)); // straight top-down

    // --- camera (UPROPERTY) ---
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
    Camera->OrthoWidth = 3500.f; // tweak to taste (2500–5000)

    // top-down hygiene: no auto-rotation
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AArcCharacter::BeginPlay()
{
    Super::BeginPlay();

    // notes: enforce camera in case BP had overrides previously
    if (Camera && Camera->ProjectionMode != ECameraProjectionMode::Orthographic)
    {
        Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
        Camera->OrthoWidth = 3500.f;
    }
    if (SpringArm && !SpringArm->IsUsingAbsoluteRotation())
    {
        SpringArm->SetUsingAbsoluteRotation(true);
        SpringArm->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
    }
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

    // notes: flip signs to match screen 'up'/'right' in ortho
    if (!Axis.IsNearlyZero())
    {
        AddMovementInput(FVector::XAxisVector, Axis.Y);
        AddMovementInput(FVector::YAxisVector, Axis.X);
    }
}

void AArcCharacter::HandleFire(const FInputActionValue& /*Value*/)
{
    UE_LOG(LogTemp, Log, TEXT("[ArcChar] Fire (placeholder)"));
}

void AArcCharacter::HandleDash(const FInputActionValue& /*Value*/)
{
    UE_LOG(LogTemp, Log, TEXT("[ArcChar] Dash (placeholder)"));
}
