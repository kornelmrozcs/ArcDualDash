#include "Collectable.h"
#include "MyCar.h"
#include "Components/CapsuleComponent.h"
#include "Engine/EngineTypes.h" // notes: ECC_Vehicle


ACollectable::ACollectable()
{
	PrimaryActorTick.bCanEverTick = false;

	// root
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	// trigger sphere (overlap Pawn only)
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(RootComp);
	Sphere->InitSphereRadius(100.f); // tweak in BP if needed
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Sphere->SetGenerateOverlapEvents(true);

	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);     // notes: characters/bots path
	Sphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);  // notes: Chaos vehicle channel

	// visual mesh (no collision)
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComp);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Tags.Add(FName("Collectable"));
}

void ACollectable::BeginPlay()
{
	Super::BeginPlay();

	// safety
	if (!ensureAlwaysMsgf(IsValid(Sphere), TEXT("[Collectable] Sphere is null on %s"), *GetName()))
	{
		return;
	}
	if (Mesh && Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACollectable::OnSphereBeginOverlap);
	Sphere->UpdateOverlaps();
	UpdateOverlaps(false);
}

void ACollectable::OnSphereBeginOverlap(UPrimitiveComponent* /*OverlappedComp*/, AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*SweepResult*/)
{
	AMyCar* Car = Cast<AMyCar>(OtherActor);
	if (!Car)
	{
		return; // only react to our cars
	}

	// score
	if (ScoreValue != 0)
	{
		Car->AddScore(ScoreValue);
	}

	// speed boost
	if (bGivesSpeedBoost)
	{
		Car->StartSpeedBoost(BoostDuration, BoostScale);
	}

	UE_LOG(LogTemp, Log, TEXT("[Collectable] picked by %s (+%d, boost=%s, dur=%.1fs scale=%.2f)"),
		*GetNameSafe(OtherActor), ScoreValue, bGivesSpeedBoost ? TEXT("yes") : TEXT("no"), BoostDuration, BoostScale);

	ConsumePickup();
}

void ACollectable::ConsumePickup()
{
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetLifeSpan(0.1f);
}
