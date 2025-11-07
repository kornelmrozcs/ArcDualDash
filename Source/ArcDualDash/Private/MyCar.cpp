#include "MyCar.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"

AMyCar::AMyCar()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMyCar::BeginPlay()
{
	Super::BeginPlay();

	// --- Input contexts (existing) ---
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
				Subsystem->AddMappingContext(DefaultMappingContext, 0);

			const int32 ControllerId = PlayerController->GetLocalPlayer()
				? PlayerController->GetLocalPlayer()->GetControllerId() : 0;
			if (ControllerId == 0 && ProxyMappingContext_P2)
				Subsystem->AddMappingContext(ProxyMappingContext_P2, 1);
		}
	}

	// Save baseline drag coefficient
	if (auto* Move = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		SavedDragCoefficient = Move->DragCoefficient;
	}

	// --- Bind crash detection (triple safety) ---
	// 1) Prefer an attached CrashTrigger box
	if (UBoxComponent* CrashTrigger = FindComponentByClass<UBoxComponent>())
	{
		CrashTrigger->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CrashTrigger->SetNotifyRigidBodyCollision(true);     // enables hit events
		CrashTrigger->SetGenerateOverlapEvents(true);        // enables overlap fallback
		CrashTrigger->OnComponentHit.AddDynamic(this, &AMyCar::OnCarHit);
		CrashTrigger->OnComponentBeginOverlap.AddDynamic(this, &AMyCar::OnCrashTriggerOverlap);
		UE_LOG(LogTemp, Warning, TEXT("[MyCar] Bound OnHit & OnOverlap to CrashTrigger."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MyCar] CrashTrigger not found; falling back to mesh/root."));
	}

	// 2) Also bind to root/mesh hit (some setups only fire here)
	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		RootPrim->SetNotifyRigidBodyCollision(true);
		RootPrim->OnComponentHit.AddDynamic(this, &AMyCar::OnCarHit);
	}

	if (GetMesh())
	{
		GetMesh()->SetNotifyRigidBodyCollision(true);
		GetMesh()->OnComponentHit.AddDynamic(this, &AMyCar::OnCarHit);
	}

	// 3) Global actor hit fallback (fires if *any* component hits)
	OnActorHit.AddDynamic(this, &AMyCar::OnAnyActorHit);
}

void AMyCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Apply boost forward force
	if (bBoostActive && GetMesh())
	{
		const FVector Fwd = GetActorForwardVector();
		GetMesh()->AddForce(Fwd * BoostForce, NAME_None, true);
	}
}

void AMyCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// --- P1 ---
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCar::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMyCar::MoveEnd);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Triggered, this, &AMyCar::OnHandbrakePressed);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AMyCar::OnHandbrakeReleased);

		// --- P2 Proxy ---
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			const int32 ControllerId = PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetControllerId() : 0;
			if (ControllerId == 0)
			{
				if (MoveAction_P2)
				{
					EnhancedInputComponent->BindAction(MoveAction_P2, ETriggerEvent::Triggered, this, &AMyCar::Move_P2);
					EnhancedInputComponent->BindAction(MoveAction_P2, ETriggerEvent::Completed, this, &AMyCar::MoveEnd_P2);
				}
				if (HandbrakeAction_P2)
				{
					EnhancedInputComponent->BindAction(HandbrakeAction_P2, ETriggerEvent::Triggered, this, &AMyCar::OnHandbrakePressed_P2);
					EnhancedInputComponent->BindAction(HandbrakeAction_P2, ETriggerEvent::Completed, this, &AMyCar::OnHandbrakeReleased_P2);
				}
			}
		}
	}
}

// ---------------- Movement ----------------

void AMyCar::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	GetVehicleMovementComponent()->SetThrottleInput(MovementVector.Y);

	if (MovementVector.Y < 0)
		GetVehicleMovementComponent()->SetBrakeInput(-MovementVector.Y);
	else
		GetVehicleMovementComponent()->SetBrakeInput(0);

	GetVehicleMovementComponent()->SetSteeringInput(MovementVector.X);
}

void AMyCar::MoveEnd()
{
	GetVehicleMovementComponent()->SetBrakeInput(0);
	GetVehicleMovementComponent()->SetThrottleInput(0);
	GetVehicleMovementComponent()->SetSteeringInput(0);
}

void AMyCar::OnHandbrakePressed()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(true);
}

void AMyCar::OnHandbrakeReleased()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(false);
}

// ---------------- P2 proxy ----------------

static AMyCar* GetP2Car(const UObject* WorldContext)
{
	if (APlayerController* PC2 = UGameplayStatics::GetPlayerController(WorldContext, 1))
		return Cast<AMyCar>(PC2->GetPawn());
	return nullptr;
}

void AMyCar::Move_P2(const FInputActionValue& Value)
{
	if (AMyCar* P2 = GetP2Car(this))
	{
		const FVector2D Axis = Value.Get<FVector2D>();
		auto* Move = P2->GetVehicleMovementComponent();

		Move->SetThrottleInput(Axis.Y);
		if (Axis.Y < 0.f) Move->SetBrakeInput(-Axis.Y);
		else Move->SetBrakeInput(0.f);
		Move->SetSteeringInput(Axis.X);
	}
}

void AMyCar::MoveEnd_P2()
{
	if (AMyCar* P2 = GetP2Car(this))
	{
		auto* Move = P2->GetVehicleMovementComponent();
		Move->SetBrakeInput(0.f);
		Move->SetThrottleInput(0.f);
		Move->SetSteeringInput(0.f);
	}
}

void AMyCar::OnHandbrakePressed_P2()
{
	if (AMyCar* P2 = GetP2Car(this))
		P2->GetVehicleMovementComponent()->SetHandbrakeInput(true);
}

void AMyCar::OnHandbrakeReleased_P2()
{
	if (AMyCar* P2 = GetP2Car(this))
		P2->GetVehicleMovementComponent()->SetHandbrakeInput(false);
}

// ---------------- Laps ----------------

void AMyCar::LapCheckpoint(int32 _CheckpointNo, int32 _MaxCheckpoint, bool _bStartFinishLine)
{
	if (CurrentCheckpoint >= _MaxCheckpoint && _bStartFinishLine)
	{
		Lap += 1;
		CurrentCheckpoint = 1;
	}
	else if (_CheckpointNo == CurrentCheckpoint + 1)
	{
		CurrentCheckpoint += 1;
	}
	else if (_CheckpointNo < CurrentCheckpoint)
	{
		CurrentCheckpoint = _CheckpointNo;
	}

	UE_LOG(LogTemp, Warning, TEXT("Lap: %i Check Point: %i"), Lap, CurrentCheckpoint);
}

// ---------------- PowerUps ----------------

void AMyCar::StartSpeedBoost(float DurationSeconds, float Force)
{
	if (auto* Move = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		if (!bBoostActive)
		{
			SavedDragCoefficient = Move->DragCoefficient;
			Move->DragCoefficient = SavedDragCoefficient * BoostDragScale;
		}
	}

	BoostForce = (Force > 0.f) ? Force : BoostForce;
	bBoostActive = true;

	GetWorldTimerManager().ClearTimer(BoostTimer);
	GetWorldTimerManager().SetTimer(BoostTimer, this, &AMyCar::EndSpeedBoost,
		(DurationSeconds > 0.f ? DurationSeconds : BoostDurationDefault), false);

	UE_LOG(LogTemp, Log, TEXT("[MyCar] BOOST ON for %.2fs, Force=%.0f"), DurationSeconds, BoostForce);
}

void AMyCar::EndSpeedBoost()
{
	if (auto* Move = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		Move->DragCoefficient = SavedDragCoefficient;
	}
	bBoostActive = false;
	UE_LOG(LogTemp, Log, TEXT("[MyCar] BOOST OFF"));
}

int32 AMyCar::AddScore(int32 Delta)
{
	Score = FMath::Max(0, Score + Delta);
	UE_LOG(LogTemp, Log, TEXT("[Score] %s += %d => %d"), *GetName(), Delta, Score);
	return Score;
}

// ---------------- Crash + Respawn ----------------

void AMyCar::OnCarHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const float ImpactForce = NormalImpulse.Size();
	if (!bIsCrashed && ImpactForce > CrashForceThreshold && OtherActor && OtherActor != this)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Crash] HIT by %s | Force=%.1f"), *OtherActor->GetName(), ImpactForce);
		HandleCarCrash();
	}
}

void AMyCar::OnAnyActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	const float ImpactForce = NormalImpulse.Size();
	if (!bIsCrashed && ImpactForce > CrashForceThreshold && OtherActor && OtherActor != this)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Crash] ACTOR HIT by %s | Force=%.1f"), *OtherActor->GetName(), ImpactForce);
		HandleCarCrash();
	}
}

void AMyCar::OnCrashTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 /*OtherBodyIndex*/, bool /*bFromSweep*/, const FHitResult& /*Sweep*/)
{
	if (bIsCrashed || !OtherActor || OtherActor == this) return;

	float Speed = 0.f;
	if (const auto* Move = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		Speed = FMath::Abs(Move->GetForwardSpeed()); // cm/s (can be negative in reverse)
	}
	else if (USkeletalMeshComponent* CarMesh = GetMesh()) // ✅ renamed + non-const
	{
		Speed = CarMesh->GetPhysicsLinearVelocity().Size();
	}

	if (Speed > CrashSpeedThreshold)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Crash] OVERLAP with %s | Speed=%.1f cm/s -> CRASH"), *OtherActor->GetName(), Speed);
		HandleCarCrash();
	}
}


void AMyCar::HandleCarCrash()
{
	if (bIsCrashed) return;
	bIsCrashed = true;

	// Explosion FX
	if (ExplosionFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionFX, GetActorLocation(), GetActorRotation());
	}

	// Disable car temporarily
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	// Stop movement immediately
	if (auto* Move = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		Move->StopMovementImmediately();
	}

	// Timer for respawn
	GetWorldTimerManager().SetTimerForNextTick([this]()
		{
			FTimerHandle RespawnHandle;
			GetWorldTimerManager().SetTimer(RespawnHandle, this, &AMyCar::RespawnCar, RespawnDelay, false);
		});
}

void AMyCar::RespawnCar()
{
	FVector RespawnLocation = FVector(0, 0, 200);
	FRotator RespawnRotation = FRotator::ZeroRotator;

	if (LastCheckpoint)
	{
		RespawnLocation = LastCheckpoint->GetActorLocation();
		RespawnRotation = LastCheckpoint->GetActorRotation();
	}

	// Teleport instead of normal move (avoids warnings)
	SetActorLocationAndRotation(RespawnLocation, RespawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	// Reset physics
	if (USkeletalMeshComponent* CarMesh = GetMesh())
	{
		CarMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		CarMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		CarMesh->WakeAllRigidBodies();
	}

	bIsCrashed = false;
	UE_LOG(LogTemp, Warning, TEXT("[Crash] Respawned successfully."));
}

