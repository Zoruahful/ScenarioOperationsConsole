#include "SOCOperationsPlayerController.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/Pawn.h"
#include "SOCOperationsDemoActor.h"

namespace
{
constexpr float ConsoleInteractionRange = 360.0f;
const FVector ConsoleInteractionPoint(0.0f, 40.0f, 38.0f);
}

void ASOCOperationsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	if (APawn* ControlledPawn = GetPawn())
	{
		SetViewTarget(ControlledPawn);
		SetControlRotation(FRotator(-4.0f, 90.0f, 0.0f));
		ControlledPawn->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));
	}
}

void ASOCOperationsPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	for (TActorIterator<ASOCOperationsDemoActor> It(GetWorld()); It; ++It)
	{
		const APawn* ControlledPawn = GetPawn();
		// Use horizontal distance so small camera/pawn height changes do not break console interaction.
		const bool bInRange = ControlledPawn && FVector::Dist2D(ControlledPawn->GetActorLocation(), ConsoleInteractionPoint) <= ConsoleInteractionRange;
		It->SetOperatorInRange(bInRange);
		return;
	}
}

void ASOCOperationsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ASOCOperationsPlayerController::AdvanceOperationsConsole);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ASOCOperationsPlayerController::ResetOperationsConsole);
}

void ASOCOperationsPlayerController::AdvanceOperationsConsole()
{
	for (TActorIterator<ASOCOperationsDemoActor> It(GetWorld()); It; ++It)
	{
		It->AdvanceOperationStep();
		return;
	}
}

void ASOCOperationsPlayerController::ResetOperationsConsole()
{
	for (TActorIterator<ASOCOperationsDemoActor> It(GetWorld()); It; ++It)
	{
		It->ResetOperationSequence();
		return;
	}
}
