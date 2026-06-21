#include "SOCOperatorPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ASOCOperatorPawn::ASOCOperatorPawn()
{
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("OperatorCamera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	Camera->bUsePawnControlRotation = true;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = 260.0f;
	Movement->BrakingDecelerationWalking = 1600.0f;
	Movement->bOrientRotationToMovement = false;
	Movement->GravityScale = 2.0f;
	Movement->JumpZVelocity = 0.0f;
	Movement->AirControl = 0.0f;
}

void ASOCOperatorPawn::BeginPlay()
{
	Super::BeginPlay();

	if (GetActorLocation().Z < 80.0f)
	{
		SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, 96.0f), false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Walking);
	}
}

void ASOCOperatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ASOCOperatorPawn::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ASOCOperatorPawn::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ASOCOperatorPawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &ASOCOperatorPawn::AddControllerPitchInput);
}

void ASOCOperatorPawn::MoveForward(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ASOCOperatorPawn::MoveRight(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}
