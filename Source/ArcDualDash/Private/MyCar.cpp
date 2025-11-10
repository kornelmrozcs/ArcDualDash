#include "MyCar.h"
#include "RaceGameState.h"
#include "RacePlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"

AMyCar::AMyCar()
{
	PrimaryActorTick.bCanEverTick = true;

	// Optional crash trigger component
	CrashTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("CrashTrigger"));
	if (CrashTrigger)
	{
		CrashTrigger->SetupAttachment(GetMesh());
		CrashTrigger->SetBoxExtent(FVector(120.f, 80.f, 50.f));
		CrashTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CrashTrigger->SetCollisionResponseToAllChannels(ECR_Overlap);
	}
}

void AMyCar::BeginPlay()
{
	Super::BeginPlay();

	// --- Assign PlayerID ---
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ARacePlayerController* RPC = Cast<ARacePlayerController>(PC))
		{
			PlayerID = RPC->PlayerIndex;
			UE_LOG(LogTemp, Log, TEXT("[MyCar] %s assigned to Player %d"), *GetName(), PlayerID);
		}
	}

	// --- Remember initial spawn ---
	InitialSpawnLocation = GetActorLocation();
	InitialSpawnRotation = GetActorRotation();

	// --- Enable hit events ---
	if (USkeletalMeshComponent* CarMesh = GetMesh())
	{
		CarMesh->SetNotifyRigidBodyCollision(true);
		CarMesh->BodyInstance.SetUseCCD(true);
		CarMesh->SetGenerateOverlapEvents(true);
	}

	// --- Bind hit and overlap events ---
	if (USkeletalMeshComponent* CarMesh = GetMesh())
	{
		CarMesh->OnComponentHit.AddDynamic(this, &AMyCar::OnCarHit);
	}
	OnActorHit.AddDynamic(this, &AMyCar::OnAnyActorHit);

	if (CrashTrigger)
	{
		CrashTrigger->OnComponentBeginOverlap.AddDynamic(this, &AMyCar::OnCrashTriggerOverlap);
	}

	UE_LOG(LogTemp, Log, TEXT("[MyCar] Bound collision + overlap events for crash detection."));

	// --- Input mapping setup ---
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

	// --- Cache checkpoints ---
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACheckpoints::StaticClass(), FoundActors);

	AllCheckpoints.Empty();
	for (AActor* Actor : FoundActors)
	{
		if (ACheckpoints* CP = Cast<ACheckpoints>(Actor))
		{
			AllCheckpoints.Add(CP);
		}
	}

	AllCheckpoints.Sort([](const ACheckpoints& A, const ACheckpoints& B)
		{
			return A.CheckPointNo < B.CheckPointNo;
		});

	UE_LOG(LogTemp, Log, TEXT("[MyCar] Found %d checkpoints for tracking."), AllCheckpoints.Num());
}

// ---------------------------------------------------------
// Tick
// ---------------------------------------------------------
void AMyCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bBoostActive && GetMesh())
	{
		const FVector Fwd = GetActorForwardVector();
		GetMesh()->AddForce(Fwd * BoostForce, NAME_None, true);
	}

	// --- Update distance to next checkpoint ---
	if (AllCheckpoints.Num() > 0)
	{
		int32 NextIndex = (CurrentCheckpointIndex + 1) % AllCheckpoints.Num();
		if (AllCheckpoints.IsValidIndex(NextIndex))
		{
			if (ACheckpoints* NextCheckpoint = AllCheckpoints[NextIndex])
			{
				DistanceToNextCheckpoint = FVector::Dist(GetActorLocation(), NextCheckpoint->GetActorLocation());
			}
		}
	}

	if (IsPlayerControlled())
	{
		LocalElapsedTime += DeltaSeconds;
		OnTimeUpdatedLocal.Broadcast(LocalElapsedTime);
	}
}

// ---------------------------------------------------------
// Input setup
// ---------------------------------------------------------
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

// ---------------------------------------------------------
// Movement
// ---------------------------------------------------------
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

// ---------------------------------------------------------
// Lap + checkpoint logic
// ---------------------------------------------------------
void AMyCar::LapCheckpoint(int32 CheckpointNo, int32 MaxCheckpoint, bool bStartFinishLine)
{
	bool bNewLap = false;

	if (CurrentCheckpointIndex >= MaxCheckpoint && bStartFinishLine)
	{
		Lap += 1;
		CurrentCheckpointIndex = 1;
		bNewLap = true;
	}
	else if (CheckpointNo == CurrentCheckpointIndex + 1)
	{
		CurrentCheckpointIndex += 1;
	}
	else if (CheckpointNo < CurrentCheckpointIndex)
	{
		CurrentCheckpointIndex = CheckpointNo;
	}

	// ✅ Sync log output and leaderboard behavior
	UE_LOG(LogTemp, Warning, TEXT("[MyCar] Player %d -> Lap %d | Checkpoint %d"), PlayerID, Lap, CurrentCheckpointIndex);

	// --- Global GameState broadcast ---
	if (ARaceGameState* GS = GetWorld()->GetGameState<ARaceGameState>())
	{
		if (bStartFinishLine)
		{
			if (Lap == 1 && CurrentCheckpointIndex == 1 && !GS->bTimerRunning)
			{
				GS->ResetTimer();
				GS->bTimerRunning = true;
			}
			if (bNewLap)
			{
				GS->IncrementLapAndBroadcast();
			}
		}
	}

	// --- Local HUD update ---
	const int32 TotalLaps = 3;
	OnLapChangedLocal.Broadcast(Lap, TotalLaps);
}


// ---------------------------------------------------------
// PowerUps
// ---------------------------------------------------------
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

// ---------------------------------------------------------
// Crash + Respawn
// ---------------------------------------------------------
void AMyCar::OnCarHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsGhost) return;

	const float ImpactForce = NormalImpulse.Size();
	if (!bIsCrashed && ImpactForce > CrashForceThreshold && OtherActor && OtherActor != this)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Crash] HIT by %s | Force=%.1f"), *OtherActor->GetName(), ImpactForce);
		HandleCarCrash();
	}
}

void AMyCar::OnAnyActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsGhost) return;

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
	if (bIsGhost || bIsCrashed || !OtherActor || OtherActor == this) return;

	float Speed = 0.f;
	if (const auto* Move = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		Speed = FMath::Abs(Move->GetForwardSpeed());
	}
	else if (USkeletalMeshComponent* CarMesh = GetMesh())
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

	if (ExplosionFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionFX, GetActorLocation(), GetActorRotation());
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	if (auto* Move = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		Move->StopMovementImmediately();
	}

	FTimerHandle RespawnHandle;
	GetWorldTimerManager().SetTimer(RespawnHandle, this, &AMyCar::RespawnCar, RespawnDelay, false);
}

void AMyCar::RespawnCar()
{
	FVector BaseLoc = InitialSpawnLocation;
	FRotator BaseRot = InitialSpawnRotation;

	if (LastCheckpoint)
	{
		BaseLoc = LastCheckpoint->GetActorLocation();
		BaseRot = LastCheckpoint->GetActorRotation();
	}

	BaseLoc.Z += 100.f;

	const int32 Slot = GetRespawnSlot();
	const FVector Right = BaseRot.RotateVector(FVector::RightVector);
	BaseLoc += Right * ((Slot - 0.5f) * LaneOffset);

	SetActorLocationAndRotation(BaseLoc, BaseRot, false, nullptr, ETeleportType::TeleportPhysics);

	if (USkeletalMeshComponent* CarMesh = GetMesh())
	{
		CarMesh->SetPhysicsLinearVelocity(BaseRot.Vector() * ForwardNudgeOnRespawn);
		CarMesh->WakeAllRigidBodies();
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	BeginGhost();
	bIsCrashed = false;

	UE_LOG(LogTemp, Warning, TEXT("[Crash] Respawned safely at %s"), *BaseLoc.ToString());
}

// ---------------------------------------------------------
// Respawn helpers
// ---------------------------------------------------------
int32 AMyCar::GetRespawnSlot() const
{
	if (const APawn* P = Cast<APawn>(this))
	{
		if (const APlayerController* PC = Cast<APlayerController>(P->GetController()))
		{
			if (const ULocalPlayer* LP = PC->GetLocalPlayer())
				return FMath::Clamp(LP->GetControllerId(), 0, 3);
		}
	}
	return (GetUniqueID() % 4);
}

bool AMyCar::FindSafeRespawnSpot(const FVector& BaseLoc, const FRotator& BaseRot, FVector& OutLoc) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OutLoc = BaseLoc;
		return false;
	}

	TArray<FVector> Offsets;
	const FVector Fwd = BaseRot.RotateVector(FVector::ForwardVector);
	const FVector Right = BaseRot.RotateVector(FVector::RightVector);

	// Check forward steps
	for (int32 i = 0; i < MaxRespawnSearchSteps; i++)
	{
		Offsets.Add(Fwd * (i * 200.f));
	}

	// Check lane offsets left/right
	for (int32 i = 1; i <= MaxRespawnSearchSteps; i++)
	{
		Offsets.Add(Right * (i * LaneOffset));
		Offsets.Add(Right * -(i * LaneOffset));
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(RespawnOverlap), false, this);
	FCollisionShape Sphere = FCollisionShape::MakeSphere(RespawnClearRadius);

	for (const FVector& O : Offsets)
	{
		const FVector Test = BaseLoc + O + FVector(0, 0, 10);
		bool bHit = World->OverlapBlockingTestByChannel(
			Test,
			FQuat::Identity,
			ECC_Pawn,
			Sphere,
			Params
		);

		if (!bHit)
		{
			OutLoc = Test;
			return true;
		}
	}

	OutLoc = BaseLoc;
	return false;
}


// ---------------------------------------------------------
// Ghost mode
// ---------------------------------------------------------
void AMyCar::BeginGhost()
{
	if (bIsGhost) return;
	bIsGhost = true;

	if (USkeletalMeshComponent* CarMesh = GetMesh())
	{
		CarMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	TArray<AActor*> Cars;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyCar::StaticClass(), Cars);
	for (AActor* A : Cars)
	{
		if (A && A != this && GetMesh())
			GetMesh()->IgnoreActorWhenMoving(A, true);
	}

	FTimerHandle GhostHandle;
	GetWorldTimerManager().SetTimer(GhostHandle, this, &AMyCar::EndGhost, GhostTimeAfterRespawn, false);
}

void AMyCar::EndGhost()
{
	if (!bIsGhost) return;
	bIsGhost = false;

	if (USkeletalMeshComponent* CarMesh = GetMesh())
	{
		CarMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

		TArray<AActor*> Cars;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyCar::StaticClass(), Cars);
		for (AActor* A : Cars)
		{
			if (A && A != this)
				CarMesh->IgnoreActorWhenMoving(A, false);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Respawn] Ghost ended."));
}