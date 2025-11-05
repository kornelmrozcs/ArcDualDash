// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCar.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"  // notes: GetPlayerController to reach PC(1)
#include "ChaosWheeledVehicleMovementComponent.h" // notes: explicit for SetThrottle/Steering/Handbrake
#include "Components/SkeletalMeshComponent.h" // notes: AddForce on vehicle mesh

AMyCar::AMyCar()
{
	// notes: boost u¿ywa Tick do AddForce
	PrimaryActorTick.bCanEverTick = true;
}

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

	// notes: zapisz bazowy drag, ¿eby po boost wróciæ do wartoœci wyjœciowej
	if (auto* Move = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		SavedDragCoefficient = Move->DragCoefficient;
	}
}

void AMyCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// notes: podczas boost dodajê sta³¹ si³ê do przodu — niezale¿nie od gazu
	if (bBoostActive && GetMesh())
	{
		const FVector Fwd = GetActorForwardVector();
		GetMesh()->AddForce(Fwd * BoostForce, NAME_None, true);
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
	FVector2D MovementVector = Value.Get<FVector2D>();
	GetVehicleMovementComponent()->SetThrottleInput(MovementVector.Y);
	if (MovementVector.Y < 0)
	{
		GetVehicleMovementComponent()->SetBrakeInput(MovementVector.Y * -1);
	}
	else
	{
		GetVehicleMovementComponent()->SetBrakeInput(0);
	}
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
		auto* Move = P2->GetVehicleMovementComponent();

		Move->SetThrottleInput(Axis.Y);
		if (Axis.Y < 0.f)
		{
			Move->SetBrakeInput(-Axis.Y);
		}
		else
		{
			Move->SetBrakeInput(0.f);
		}
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

// --- PowerUps / Score --------------------------------------------------------

void AMyCar::StartSpeedBoost(float DurationSeconds, float Force)
{
	if (auto* Move = Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent()))
	{
		// zapamiêtaj i zredukuj drag przy pierwszym wejœciu
		if (!bBoostActive)
		{
			SavedDragCoefficient = Move->DragCoefficient;
			Move->DragCoefficient = SavedDragCoefficient * BoostDragScale;
		}
	}

	BoostForce = (Force > 0.f) ? Force : BoostForce;
	bBoostActive = true;

	GetWorldTimerManager().ClearTimer(BoostTimer);
	GetWorldTimerManager().SetTimer(
		BoostTimer, this, &AMyCar::EndSpeedBoost,
		(DurationSeconds > 0.f ? DurationSeconds : BoostDurationDefault),
		false
	);

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
