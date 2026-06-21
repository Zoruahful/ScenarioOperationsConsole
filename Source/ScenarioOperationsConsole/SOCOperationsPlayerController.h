#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SOCOperationsPlayerController.generated.h"

UCLASS()
class SCENARIOOPERATIONSCONSOLE_API ASOCOperationsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

private:
	void AdvanceOperationsConsole();
	void ResetOperationsConsole();
};
