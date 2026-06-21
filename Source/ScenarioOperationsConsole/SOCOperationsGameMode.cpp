#include "SOCOperationsGameMode.h"

#include "SOCOperatorPawn.h"
#include "SOCOperationsHUD.h"
#include "SOCOperationsPlayerController.h"

ASOCOperationsGameMode::ASOCOperationsGameMode()
{
	HUDClass = ASOCOperationsHUD::StaticClass();
	DefaultPawnClass = ASOCOperatorPawn::StaticClass();
	PlayerControllerClass = ASOCOperationsPlayerController::StaticClass();
}
