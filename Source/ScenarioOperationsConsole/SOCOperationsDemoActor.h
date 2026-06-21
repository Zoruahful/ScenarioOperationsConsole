#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissionScenarioInstance.h"
#include "SOCOperationsDemoActor.generated.h"

class UMaterialInterface;
class UStaticMesh;
class USceneComponent;
class UTextRenderComponent;

USTRUCT(BlueprintType)
struct FSOCConsoleEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float TimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	FString Message;

	UPROPERTY(BlueprintReadOnly)
	FLinearColor Color = FLinearColor::White;
};

UCLASS()
class SCENARIOOPERATIONSCONSOLE_API ASOCOperationsDemoActor : public AActor
{
	GENERATED_BODY()

public:
	ASOCOperationsDemoActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	float GetDemoTimeSeconds() const { return DemoTimeSeconds; }

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	FString GetCurrentPhaseLabel() const { return CurrentPhaseLabel; }

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	FString GetActiveObjectiveLabel() const { return ActiveObjectiveLabel; }

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	FString GetValidationSummary() const { return ValidationSummary; }

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	FString GetReportSummary() const { return ReportSummary; }

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	FString GetOperatorPrompt() const { return OperatorPrompt; }

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	bool IsOperatorInRange() const { return bOperatorInRange; }

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	int32 GetCompletedObjectiveCount() const;

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	int32 GetTotalObjectiveCount() const;

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	bool IsScenarioComplete() const;

	UFUNCTION(BlueprintPure, Category = "Scenario Operations Console")
	const TArray<FSOCConsoleEvent>& GetConsoleEvents() const { return ConsoleEvents; }

	UFUNCTION(BlueprintCallable, Category = "Scenario Operations Console")
	void AdvanceOperationStep();

	UFUNCTION(BlueprintCallable, Category = "Scenario Operations Console")
	void ResetOperationSequence();

	UFUNCTION(BlueprintCallable, Category = "Scenario Operations Console")
	void SetOperatorInRange(bool bInRange);

private:
	UPROPERTY(VisibleAnywhere, Category = "Scenario Operations Console")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UActorComponent>> PreviewComponents;

	UPROPERTY(Transient)
	TObjectPtr<UMissionScenarioInstance> ScenarioInstance;

	UPROPERTY(Transient)
	TArray<FSOCConsoleEvent> ConsoleEvents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> ObjectiveBeacons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> StepPads;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> WorldLabels;

	UPROPERTY(Transient)
	float DemoTimeSeconds = 0.0f;

	UPROPERTY(Transient)
	int32 LastScriptStep = INDEX_NONE;

	UPROPERTY(Transient)
	int32 CurrentOperationStep = -1;

	UPROPERTY(Transient)
	FString CurrentPhaseLabel = TEXT("Scenario Select");

	UPROPERTY(Transient)
	FString ActiveObjectiveLabel = TEXT("Standby");

	UPROPERTY(Transient)
	FString ValidationSummary = TEXT("Pending");

	UPROPERTY(Transient)
	FString ReportSummary = TEXT("Not generated");

	UPROPERTY(Transient)
	FString OperatorPrompt = TEXT("Press E to validate scenario");

	UPROPERTY(Transient)
	bool bOperatorInRange = false;

	FString ScenarioJson;

	void BuildStyledOperationsRoom();
	void ClearStyledOperationsRoom();
	void InitializeScenarioRuntime();
	void RunScriptStep(int32 StepIndex);
	void AddEvent(const FString& Message, const FLinearColor& Color);
	void CompleteCurrentObjective(const FString& NextLabel);
	void RefreshOperatorPrompt();
	void UpdateBeaconState();
	void UpdateStepPadState();
	void UpdateLabelFacing();
	void SpawnCube(const FVector& Location, const FVector& Scale, const FLinearColor& Color, const FString& Name);
	void SpawnSphere(const FVector& Location, const FVector& Scale, const FLinearColor& Color, const FString& Name);
	UStaticMeshComponent* SpawnStepPad(const FVector& Location, const FString& Label, int32 StepIndex);
	void SpawnLabel(const FString& Text, const FVector& Location, float WorldSize, const FLinearColor& Color);
	UMaterialInterface* CreateMaterial(const FLinearColor& Color, const FString& Name);
	UStaticMesh* LoadBasicMesh(const TCHAR* Path) const;
};
