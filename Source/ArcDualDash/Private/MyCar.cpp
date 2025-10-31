// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCar.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
void AMyCar::BeginPlay()
{
	Super::BeginPlay();
	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController -> GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}
void AMyCar::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent =
		CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyCar::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMyCar::MoveEnd);
		//Handbrake
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Triggered, this,
			&AMyCar::OnHandbrakePressed);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this,
			&AMyCar::OnHandbrakeReleased);
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
