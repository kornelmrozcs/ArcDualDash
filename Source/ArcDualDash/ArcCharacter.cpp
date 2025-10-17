#include "ArcCharacter.h"

#include "Components/CapsuleComponent.h" // notes: capsule collision tweak for pickups
#include "DrawDebugHelpers.h" // notes: debug visualize capsule

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

    /*
    // notes: draw my capsule so 
    // I SEE it (lifetime 30s)
    const FVector CLoc = GetCapsuleComponent()->GetComponentLocation();
    const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    DrawDebugCapsule(GetWorld(), CLoc, HalfHeight, Radius, FQuat::Identity, FColor::Green, false, 30.f, 0, 2.f);
    */
    GetCharacterMovement()->MaxWalkSpeed;

    // notes: force capsule to be the only collidable thing and to overlap WorldDynamic (collectables)
    UCapsuleComponent* Cap = GetCapsuleComponent();
    Cap->SetCollisionObjectType(ECC_Pawn);             // ensure it's really Pawn
    Cap->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Cap->SetGenerateOverlapEvents(true);
    Cap->SetCollisionResponseToAllChannels(ECR_Ignore); // hard reset
    Cap->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);   // floor etc.
    Cap->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap); // collectable sphere/box
    Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);   // traces if needed
    Cap->UpdateOverlaps(); // recalc now


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

    // notes: ensure my capsule overlaps WorldDynamic (collectable sphere) and that overlaps are evaluated now
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
    GetCapsuleComponent()->SetGenerateOverlapEvents(true);
    GetCapsuleComponent()->UpdateOverlaps(); //


    // notes: debug dump of my capsule settings
    UE_LOG(LogTemp, Log, TEXT("[ArcChar] Capsule: CollEnabled=%d, ObjType=%d, WorldDynamicResp=%d, OverlapEvents=%d"),
        (int32)GetCapsuleComponent()->GetCollisionEnabled(),
        (int32)GetCapsuleComponent()->GetCollisionObjectType(),
        (int32)GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_WorldDynamic),
        GetCapsuleComponent()->GetGenerateOverlapEvents());

    // notes: bind overlap on capsule to confirm I see overlaps at all
    GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AArcCharacter::OnCapsuleBeginOverlap);

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

void AArcCharacter::OnCapsuleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
    const FHitResult& /*SweepResult*/)
{
    UE_LOG(LogTemp, Log, TEXT("[ArcChar] Capsule overlap with %s (Class=%s)"),
        *GetNameSafe(OtherActor),
        OtherActor ? *OtherActor->GetClass()->GetName() : TEXT("None"));
}

void AArcCharacter::StartTimedPowerUp(float DurationSeconds)
{
    // notes: set boosted speed and schedule revert
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        // first time: remember base (in case edited in BP)
        if (BaseMaxWalkSpeed <= 0.f)
        {
            BaseMaxWalkSpeed = Move->MaxWalkSpeed;
        }

        Move->MaxWalkSpeed = BoostedMaxWalkSpeed;
    }

    // refresh timer (stackable feel)
    GetWorldTimerManager().ClearTimer(PowerUpTimer);
    GetWorldTimerManager().SetTimer(PowerUpTimer, this, &AArcCharacter::EndTimedPowerUp, DurationSeconds, false);

    UE_LOG(LogTemp, Log, TEXT("[PowerUp] Speed boost started for %.1fs"), DurationSeconds);
}

void AArcCharacter::EndTimedPowerUp()
{
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = (BaseMaxWalkSpeed > 0.f) ? BaseMaxWalkSpeed : 600.f;
    }
    UE_LOG(LogTemp, Log, TEXT("[PowerUp] Speed boost ended"));
}
