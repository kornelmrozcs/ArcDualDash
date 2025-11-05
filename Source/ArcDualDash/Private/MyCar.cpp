
#include "MyCar.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"                  // notes: GetPlayerController to reach PC(1)
#include "ChaosWheeledVehicleMovementComponent.h"    // notes: explicit for SetThrottle/Steering/Handbrake
#include "TimerManager.h"                            // notes: for GetWorldTimerManager
#include "Engine/EngineTypes.h"

void AMyCar::BeginPlay()
{
	Super::BeginPlay();

	// notes: Add Input Mapping Context for THIS local player (P1 movement)
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// P1 mapping (WASD + Space)
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}

			// notes: Add P2 proxy mapping ONLY on ControllerId==0 (first local player)
			const int32 ControllerId = PlayerController->GetLocalPlayer() ? PlayerController->GetLocalPlayer()->GetControllerId() : 0;
			if (ControllerId == 0 && ProxyMappingContext_P2)
			{
				Subsystem->AddMappingContext(ProxyMappingContext_P2, 1);
			}
		}
	}
	if (USkeletalMeshComponent* CarMesh = Cast<USkeletalMeshComponent>(GetRootComponent()))
	{
		CarMesh->SetGenerateOverlapEvents(true);                       // notes: I want overlap events
		CarMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap); // notes: rings use WorldDynamic
		// notes: keep other channels as they are (driving physics etc.)
		UE_LOG(LogTemp, Log, TEXT("[Car] Enabled overlaps with WorldDynamic on %s"), *GetName());
	}
}

void AMyCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent =
		CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// --- P1 (existing) ---
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCar::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMyCar::MoveEnd);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Triggered, this, &AMyCar::OnHandbrakePressed);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AMyCar::OnHandbrakeReleased);

		// --- P2 keyboard proxy (Arrows + Right Ctrl) — bind ONLY on ControllerId==0 ---
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

void AMyCar::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();

	// notes: apply boost scale only to throttle; clamp to valid input range
	const float ScaledThrottle = FMath::Clamp(MovementVector.Y * CurrentThrottleScale, -1.0f, 1.0f);
	GetVehicleMovementComponent()->SetThrottleInput(ScaledThrottle);

	if (ScaledThrottle < 0.0f)
	{
		GetVehicleMovementComponent()->SetBrakeInput(-ScaledThrottle);
	}
	else
	{
		GetVehicleMovementComponent()->SetBrakeInput(0.0f);
	}

	GetVehicleMovementComponent()->SetSteeringInput(MovementVector.X);
}

void AMyCar::MoveEnd()
{
	GetVehicleMovementComponent()->SetBrakeInput(0.f);
	GetVehicleMovementComponent()->SetThrottleInput(0.f);
	GetVehicleMovementComponent()->SetSteeringInput(0.f);
}

void AMyCar::OnHandbrakePressed()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(true);
}

void AMyCar::OnHandbrakeReleased()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(false);
}

// notes: helpers — get Player 2's AMyCar pawn (PC index 1)
static AMyCar* GetP2Car(const UObject* WorldContext)
{
	if (APlayerController* PC2 = UGameplayStatics::GetPlayerController(WorldContext, 1))
	{
		return Cast<AMyCar>(PC2->GetPawn());
	}
	return nullptr;
}

void AMyCar::Move_P2(const FInputActionValue& Value)
{
	if (AMyCar* P2 = GetP2Car(this))
	{
		const FVector2D Axis = Value.Get<FVector2D>();
		auto* MoveComp = P2->GetVehicleMovementComponent();

		MoveComp->SetThrottleInput(Axis.Y);
		if (Axis.Y < 0.f)
		{
			MoveComp->SetBrakeInput(-Axis.Y);
		}
		else
		{
			MoveComp->SetBrakeInput(0.f);
		}
		MoveComp->SetSteeringInput(Axis.X);
	}
}

void AMyCar::MoveEnd_P2()
{
	if (AMyCar* P2 = GetP2Car(this))
	{
		auto* MoveComp = P2->GetVehicleMovementComponent();
		MoveComp->SetBrakeInput(0.f);
		MoveComp->SetThrottleInput(0.f);
		MoveComp->SetSteeringInput(0.f);
	}
}

void AMyCar::OnHandbrakePressed_P2()
{
	if (AMyCar* P2 = GetP2Car(this))
	{
		P2->GetVehicleMovementComponent()->SetHandbrakeInput(true);
	}
}

void AMyCar::OnHandbrakeReleased_P2()
{
	if (AMyCar* P2 = GetP2Car(this))
	{
		P2->GetVehicleMovementComponent()->SetHandbrakeInput(false);
	}
}

// notes: enforce sequential checkpoint order and count laps; mirrors lab logic
void AMyCar::LapCheckpoint(int32 _CheckpointNo, int32 _MaxCheckpoint, bool _bStartFinishLine)
{
	if (CurrentCheckpoint >= _MaxCheckpoint && _bStartFinishLine == true)
	{
		// notes: passed last gate and hit start/finish -> new lap
		Lap += 1;
		CurrentCheckpoint = 1;
	}
	else if (_CheckpointNo == CurrentCheckpoint + 1)
	{
		// notes: correct next checkpoint in order
		CurrentCheckpoint += 1;
	}
	else if (_CheckpointNo < CurrentCheckpoint)
	{
		// notes: allow correction if we come from a loop-shortcut back to earlier gate
		CurrentCheckpoint = _CheckpointNo;
	}

	UE_LOG(LogTemp, Warning, TEXT("Lap: %i Check Point: %i"), Lap, CurrentCheckpoint);
}

// ===========================
// Power-up: Speed Boost
// ===========================
void AMyCar::StartSpeedBoost(float DurationSeconds, float Scale)
{
	// notes: init current scale if a BP tweaked defaults earlier
	if (CurrentThrottleScale <= 0.0f)
	{
		CurrentThrottleScale = DefaultThrottleScale;
	}

	CurrentThrottleScale = (Scale > 0.0f) ? Scale : BoostThrottleScale;

	// notes: refresh timer so back-to-back pickups extend the effect
	GetWorldTimerManager().ClearTimer(BoostTimer);
	GetWorldTimerManager().SetTimer(
		BoostTimer,
		this,
		&AMyCar::EndSpeedBoost,
		(DurationSeconds > 0.0f ? DurationSeconds : DefaultBoostDuration),
		false
	);

	UE_LOG(LogTemp, Log, TEXT("[Boost] Started: Scale=%.2f, Duration=%.2fs"), CurrentThrottleScale, DurationSeconds);
}

void AMyCar::EndSpeedBoost()
{
	CurrentThrottleScale = DefaultThrottleScale;
	UE_LOG(LogTemp, Log, TEXT("[Boost] Ended"));
}

void AMyCar::AddScore(int32 Delta)
{
	const int32 Old = Score;
	Score = FMath::Max(0, Old + Delta);
	UE_LOG(LogTemp, Log, TEXT("[Score] +%d => %d"), Delta, Score);
}
