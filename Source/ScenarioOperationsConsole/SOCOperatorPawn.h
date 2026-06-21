#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SOCOperatorPawn.generated.h"

class UCameraComponent;

UCLASS()
class SCENARIOOPERATIONSCONSOLE_API ASOCOperatorPawn : public ACharacter
{
	GENERATED_BODY()

public:
	ASOCOperatorPawn();

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Operator")
	TObjectPtr<UCameraComponent> Camera;

	void MoveForward(float Value);
	void MoveRight(float Value);
};
