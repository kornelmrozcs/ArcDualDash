#include "Collectable.h"
#include "MyCar.h"

ACollectable::ACollectable()
{
	PrimaryActorTick.bCanEverTick = false;

	// --- root ---
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	// --- trigger sphere (overlap Vehicle / Pickup) ---
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(RootComp);
	Sphere->InitSphereRadius(100.f);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	Sphere->SetCollisionProfileName(TEXT("Pickup"));


	Sphere->SetGenerateOverlapEvents(true);

	// --- visual mesh (no collision) ---
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	Mesh->SetupAttachment(RootComp);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Tags.Add(FName("Collectable"));
}

void ACollectable::BeginPlay()
{
	Super::BeginPlay();
	if (ensure(Sphere))
	{
		Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACollectable::OnSphereBeginOverlap);
		Sphere->UpdateOverlaps();
	}
}

void ACollectable::OnSphereBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	AMyCar* Car = Cast<AMyCar>(OtherActor);
	if (!Car) return;

	if (ScoreValue != 0)
	{
		Car->AddScore(ScoreValue);
	}

	if (bGivesSpeedBoost)
	{
		Car->StartSpeedBoost(BoostDuration, BoostForce);
	}

	UE_LOG(LogTemp, Log, TEXT("[Collectable] picked by %s (+%d, boost=%s %.1fs %.0f)"),
		*GetNameSafe(Car), ScoreValue, bGivesSpeedBoost ? TEXT("yes") : TEXT("no"), BoostDuration, BoostForce);

	ConsumePickup();
}

void ACollectable::ConsumePickup()
{
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetLifeSpan(0.1f);
}
