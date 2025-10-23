// ============================================================================
// ArcCharacter.cp
// notes: Top-down camera, Enhanced Input binds, clean overlap for pickups.
//        I keep collision narrow (capsule) and block other pawns. KM
// ============================================================================

#include "ArcCharacter.h"

#include "Components/CapsuleComponent.h"           // notes: Capsule setup + overlap binding. KM
#include "DrawDebugHelpers.h"                      // notes: Optional capsule debug. KM
#include "Camera/CameraComponent.h"                // notes: Camera component. KM
#include "GameFramework/SpringArmComponent.h"      // notes: Camera boom. KM
#include "GameFramework/CharacterMovementComponent.h" // notes: Speed / movement flags. KM
#include "EnhancedInputComponent.h"                // notes: Bind IA_* actions. KM
#include "ArcPlayerController.h"                   // notes: I ask PC for IA_Move/IA_Fire/IA_Dash. KM

#include "Engine/EngineTypes.h"                    // notes: ECameraProjectionMode. KM

// ============================================================================
// Ctor: build camera rig and movement flagss
// ============================================================================
AArcCharacter::AArcCharacter()
{
    PrimaryActorTick.bCanEverTick = true; // notes: Free for FX/future logic. KM

    // --- camera boom ---
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm")); 
    SpringArm->SetupAttachment(RootComponent);                                   
    SpringArm->TargetArmLength = 1200.f;                                         // notes: Top-down distance. KM
    SpringArm->bDoCollisionTest = false;                                         // notes: No boom retract in maze. KM
    SpringArm->SetUsingAbsoluteRotation(true);                                   // notes: Lock top-down. KM
    SpringArm->bInheritYaw = false;
    SpringArm->bInheritPitch = false;
    SpringArm->bInheritRoll = false;
    SpringArm->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));                   // notes: Straight down. KM

    // --- camera ---
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));           
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);         
    Camera->bUsePawnControlRotation = false;                                     
    Camera->ProjectionMode = ECameraProjectionMode::Orthographic;                // notes: Flat look, no perspective. KM
    Camera->OrthoWidth = 3500.f;                                                 // notes: Tuned for split-screen. KM

    // --- movement flags ---
    bUseControllerRotationYaw = false;                                           // notes: Don’t face input. KM
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = false;                   // notes: Grid-like feel. KM
}

// =======================================================================
// BeginPlay: collision hygiene + enforce camera
// ============================================================================
void AArcCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Optional capsule debug (kept for quick visual checks). KM
    /*
    const FVector CLoc = GetCapsuleComponent()->GetComponentLocation();
    const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
    DrawDebugCapsule(GetWorld(), CLoc, HalfHeight, Radius, FQuat::Identity, FColor::Green, false, 30.f, 0, 2.f);
    */

    // --- collision: capsule only, overlap pickups, block floor/players ---
    UCapsuleComponent* Cap = GetCapsuleComponent();                              
    Cap->SetCollisionObjectType(ECC_Pawn);                                       // notes: Ensure pawn channel. KM
    Cap->SetCollisionEnabled(ECollisionEnabled::QueryOnly);                      // notes: No physics. KM
    Cap->SetGenerateOverlapEvents(true);                                         
    Cap->SetCollisionResponseToAllChannels(ECR_Ignore);                          // notes: Hard reset. KM
    Cap->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);             // notes: Walls/floor. KM
    Cap->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);           // notes: Collectables. KM
    Cap->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);             // notes: Traces if needed. KM
    Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);             // notes: Players block each other. KM
    Cap->UpdateOverlaps();                                                       // notes: Apply now. KM

    // --- enforce camera settings in case BP changed them ---
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

    // Ensure overlaps stay enabled (belt and suspenders). KM
    Cap->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);           
    Cap->SetGenerateOverlapEvents(true);                                         
    Cap->UpdateOverlaps();                                                       

    // Debug dump for quick checks. KM
    UE_LOG(LogTemp, Log, TEXT("[ArcChar] Capsule: CollEnabled=%d, ObjType=%d, WorldDynamicResp=%d, OverlapEvents=%d"),
        (int32)Cap->GetCollisionEnabled(),
        (int32)Cap->GetCollisionObjectType(),
        (int32)Cap->GetCollisionResponseToChannel(ECC_WorldDynamic),
        Cap->GetGenerateOverlapEvents());

    // Hook overlap so I can see hits in log; Collectable does its own logic on overlap. KM
    Cap->OnComponentBeginOverlap.AddDynamic(this, &AArcCharacter::OnCapsuleBeginOverlap);
}

// ============================================================================
// Tick: free for future movement polish (nothing yet)
// ============================================================================
void AArcCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); 
}

// ============================================================================
// SetupPlayerInputComponent: bind Enhanced Input actions from our PC
// ============================================================================
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

// ============================================================================
// Input handlers
// ============================================================================
void AArcCharacter::HandleMove(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>(); 

    // notes: Map screen up/right to world Y/X in ortho. KM
    if (!Axis.IsNearlyZero())
    {
        AddMovementInput(FVector::XAxisVector, Axis.Y); 
        AddMovementInput(FVector::YAxisVector, Axis.X); 
    }
}

void AArcCharacter::HandleFire(const FInputActionValue& /*Value*/)
{
    UE_LOG(LogTemp, Log, TEXT("[ArcChar] Fire (placeholder)")); // notes: Will consume Ammo later. KM
}

void AArcCharacter::HandleDash(const FInputActionValue& /*Value*/)
{
    UE_LOG(LogTemp, Log, TEXT("[ArcChar] Dash (placeholder)")); // notes: Will reuse boost pattern + cooldown. KM
}

// ============================================================================
// Overlap: I only log here; Collectable applies effects on its side
// ============================================================================
void AArcCharacter::OnCapsuleBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/,
    const FHitResult& /*SweepResult*/)
{
    UE_LOG(LogTemp, Log, TEXT("[ArcChar] Capsule overlap with %s (Class=%s)"),
        *GetNameSafe(OtherActor),
        OtherActor ? *OtherActor->GetClass()->GetName() : TEXT("None")); 
}

// ============================================================================
// PowerUp: temporary speed boost then revert
// ============================================================================
void AArcCharacter::StartTimedPowerUp(float DurationSeconds)
{
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        if (BaseMaxWalkSpeed <= 0.f) { BaseMaxWalkSpeed = Move->MaxWalkSpeed; }  // notes: Cache base once. KM
        Move->MaxWalkSpeed = BoostedMaxWalkSpeed;                                 // notes: Apply boost. KM
    }

    GetWorldTimerManager().ClearTimer(PowerUpTimer);                              // notes: Refresh window. KM
    GetWorldTimerManager().SetTimer(PowerUpTimer, this, &AArcCharacter::EndTimedPowerUp, DurationSeconds, false); 
    UE_LOG(LogTemp, Log, TEXT("[PowerUp] Speed boost started for %.1fs"), DurationSeconds); 
}

void AArcCharacter::EndTimedPowerUp()
{
    if (UCharacterMovementComponent* Move = GetCharacterMovement())
    {
        Move->MaxWalkSpeed = (BaseMaxWalkSpeed > 0.f) ? BaseMaxWalkSpeed : 600.f; // notes: Safe restore. KM
    }
    UE_LOG(LogTemp, Log, TEXT("[PowerUp] Speed boost ended")); 
}

// ============================================================================
// Stats helpers (HUD + pickups)
// ============================================================================
int32 AArcCharacter::AddHealth(int32 Delta)
{
    const int32 Old = Health;                                   
    Health = FMath::Clamp(Health + Delta, 0, MaxHealth);        // notes: Clamp. KM
    UE_LOG(LogTemp, Log, TEXT("[Stats] Health: %d -> %d (Delta=%d)"), Old, Health, Delta); 
    return Health;                                              
}

int32 AArcCharacter::AddAmmo(int32 Delta)
{
    const int32 Old = Ammo;                                     
    Ammo = FMath::Clamp(Ammo + Delta, 0, MaxAmmo);             // notes: Clamp. KM
    UE_LOG(LogTemp, Log, TEXT("[Stats] Ammo: %d -> %d (Delta=%d)"), Old, Ammo, Delta); 
    return Ammo;                                                
}
