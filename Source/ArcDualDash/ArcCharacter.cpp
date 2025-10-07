#include "ArcCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AArcCharacter::AArcCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Prosty top-down „rig” kamery (w³asny dla ka¿dego gracza)
    USpringArmComponent* SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 800.f;     // odleg³oœæ od postaci
    SpringArm->bDoCollisionTest = false;
    SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); // nachylenie z góry

    UCameraComponent* Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false; // kamera niezale¿na od kontrolera
}

void AArcCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AArcCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}
