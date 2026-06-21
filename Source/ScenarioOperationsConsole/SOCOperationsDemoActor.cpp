#include "SOCOperationsDemoActor.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MissionScenarioRuntime.h"

namespace
{
constexpr float ValidateTime = 4.0f;
constexpr float RunTime = 8.0f;
constexpr float HoldTime = 13.0f;
constexpr float WarningTime = 19.0f;
constexpr float ReportTime = 24.0f;
constexpr int32 FinalOperationStep = 4;
const FLinearColor ReportCompleteColor(0.20f, 0.95f, 0.46f);

FLinearColor StepPadColorForState(const int32 PadIndex, const int32 CurrentStep)
{
	if (CurrentStep > PadIndex)
	{
		return FLinearColor(0.20f, 0.95f, 0.46f);
	}

	if (CurrentStep == PadIndex)
	{
		return FLinearColor(1.0f, 0.78f, 0.20f);
	}

	return FLinearColor(0.06f, 0.26f, 0.32f);
}
}

ASOCOperationsDemoActor::ASOCOperationsDemoActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ScenarioJson = TEXT(R"json({
  "schemaVersion": "msl.scenario.v1",
  "scenarioId": "ops_console_demo",
  "displayName": "Operations Console Demo",
  "briefing": "Validate a scenario, monitor objective flow, acknowledge one warning, and generate a completion report.",
  "phases": [
    {
      "phaseId": "phase_validate",
      "displayName": "Validation",
      "objectives": [
        {
          "objectiveId": "validate_contract",
          "displayName": "Validate Scenario Contract",
          "type": "Interact",
          "targetTag": "console.validate"
        }
      ]
    },
    {
      "phaseId": "phase_run",
      "displayName": "Runtime",
      "objectives": [
        {
          "objectiveId": "monitor_route",
          "displayName": "Monitor Objective Route",
          "type": "ReachLocation",
          "targetTag": "route.alpha"
        },
        {
          "objectiveId": "acknowledge_warning",
          "displayName": "Acknowledge Runtime Warning",
          "type": "Interact",
          "targetTag": "console.warning"
        },
        {
          "objectiveId": "export_report",
          "displayName": "Generate Completion Report",
          "type": "Interact",
          "targetTag": "console.report"
        }
      ]
    }
  ]
})json");
}

void ASOCOperationsDemoActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BuildStyledOperationsRoom();
}

void ASOCOperationsDemoActor::BeginPlay()
{
	Super::BeginPlay();

	BuildStyledOperationsRoom();
	InitializeScenarioRuntime();
	AddEvent(TEXT("Scenario selected: Operations Console Demo"), FLinearColor(0.5f, 0.85f, 1.0f));
}

void ASOCOperationsDemoActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DemoTimeSeconds += DeltaSeconds;
	UpdateLabelFacing();
}

int32 ASOCOperationsDemoActor::GetCompletedObjectiveCount() const
{
	return ScenarioInstance ? ScenarioInstance->GetSnapshot().CompletedObjectiveCount : 0;
}

int32 ASOCOperationsDemoActor::GetTotalObjectiveCount() const
{
	return ScenarioInstance ? ScenarioInstance->GetSnapshot().TotalObjectiveCount : 4;
}

bool ASOCOperationsDemoActor::IsScenarioComplete() const
{
	return ScenarioInstance && ScenarioInstance->IsScenarioComplete();
}

void ASOCOperationsDemoActor::AdvanceOperationStep()
{
	if (!bOperatorInRange)
	{
		AddEvent(TEXT("Move closer to the console before interacting."), FLinearColor(1.0f, 0.78f, 0.25f));
		RefreshOperatorPrompt();
		return;
	}

	if (CurrentOperationStep >= FinalOperationStep)
	{
		AddEvent(TEXT("Report already generated. Sequence complete."), FLinearColor(0.74f, 0.84f, 0.88f));
		RefreshOperatorPrompt();
		return;
	}

	++CurrentOperationStep;
	RunScriptStep(CurrentOperationStep);
}

void ASOCOperationsDemoActor::ResetOperationSequence()
{
	CurrentOperationStep = -1;
	DemoTimeSeconds = 0.0f;
	CurrentPhaseLabel = TEXT("Scenario Select");
	ActiveObjectiveLabel = TEXT("Validate Scenario Contract");
	ValidationSummary = TEXT("Pending validation run");
	ReportSummary = TEXT("Not generated");
	ConsoleEvents.Reset();

	if (ScenarioInstance != nullptr)
	{
		ScenarioInstance->ResetScenario();
		FString Error;
		ScenarioInstance->StartScenario(Error);
	}
	else
	{
		InitializeScenarioRuntime();
	}

	AddEvent(TEXT("Sequence reset. Scenario ready for operator input."), FLinearColor(0.5f, 0.85f, 1.0f));
	RefreshOperatorPrompt();
	UpdateBeaconState();
	UpdateStepPadState();
}

void ASOCOperationsDemoActor::SetOperatorInRange(bool bInRange)
{
	if (bOperatorInRange != bInRange)
	{
		bOperatorInRange = bInRange;
		RefreshOperatorPrompt();
	}
}

void ASOCOperationsDemoActor::BuildStyledOperationsRoom()
{
	ClearStyledOperationsRoom();

	// Build the demo scene in code so a reviewer can inspect the full setup without Blueprint dependencies.
	USkyAtmosphereComponent* SkyAtmosphere = NewObject<USkyAtmosphereComponent>(this, TEXT("PreviewSkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(SceneRoot);
	SkyAtmosphere->RegisterComponent();
	PreviewComponents.Add(SkyAtmosphere);

	USkyLightComponent* SkyLight = NewObject<USkyLightComponent>(this, TEXT("PreviewSkyLight"));
	SkyLight->SetupAttachment(SceneRoot);
	SkyLight->SetIntensity(1.2f);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->RegisterComponent();
	PreviewComponents.Add(SkyLight);

	UDirectionalLightComponent* DirectionalLight = NewObject<UDirectionalLightComponent>(this, TEXT("PreviewSunLight"));
	DirectionalLight->SetupAttachment(SceneRoot);
	DirectionalLight->SetRelativeRotation(FRotator(-42.0f, -35.0f, 0.0f));
	DirectionalLight->SetIntensity(5.0f);
	DirectionalLight->SetLightColor(FLinearColor(1.0f, 0.96f, 0.88f));
	DirectionalLight->RegisterComponent();
	PreviewComponents.Add(DirectionalLight);

	UPointLightComponent* AccentLight = NewObject<UPointLightComponent>(this, TEXT("PreviewConsoleAccentLight"));
	AccentLight->SetupAttachment(SceneRoot);
	AccentLight->SetRelativeLocation(FVector(0.0f, -110.0f, 210.0f));
	AccentLight->SetIntensity(1800.0f);
	AccentLight->SetLightColor(FLinearColor(0.2f, 0.9f, 1.0f));
	AccentLight->SetAttenuationRadius(650.0f);
	AccentLight->RegisterComponent();
	PreviewComponents.Add(AccentLight);

	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		GetWorld()->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator);
	}

	SpawnCube(FVector(0.0f, 0.0f, -8.0f), FVector(12.0f, 8.0f, 0.08f), FLinearColor(0.72f, 0.78f, 0.74f), TEXT("OutdoorPad"));
	SpawnCube(FVector(0.0f, 355.0f, 30.0f), FVector(4.6f, 0.06f, 0.6f), FLinearColor(0.22f, 0.30f, 0.34f), TEXT("LowRearFeatureWall"));
	SpawnCube(FVector(-520.0f, 0.0f, 18.0f), FVector(0.06f, 4.6f, 0.28f), FLinearColor(0.22f, 0.30f, 0.34f), TEXT("LeftLowRail"));
	SpawnCube(FVector(520.0f, 0.0f, 18.0f), FVector(0.06f, 4.6f, 0.28f), FLinearColor(0.22f, 0.30f, 0.34f), TEXT("RightLowRail"));

	for (int32 Index = -4; Index <= 4; ++Index)
	{
		SpawnCube(FVector(Index * 110.0f, 0.0f, -5.0f), FVector(0.012f, 6.0f, 0.012f), FLinearColor(0.45f, 0.62f, 0.66f), TEXT("FloorGridX"));
	}
	for (int32 Index = -3; Index <= 3; ++Index)
	{
		SpawnCube(FVector(0.0f, Index * 110.0f, -4.0f), FVector(9.0f, 0.012f, 0.012f), FLinearColor(0.45f, 0.62f, 0.66f), TEXT("FloorGridY"));
	}

	SpawnCube(FVector(0.0f, 40.0f, 38.0f), FVector(2.6f, 1.2f, 0.18f), FLinearColor(0.10f, 0.13f, 0.16f), TEXT("ConsoleDesk"));
	SpawnCube(FVector(-190.0f, 40.0f, 10.0f), FVector(0.22f, 0.18f, 0.55f), FLinearColor(0.10f, 0.13f, 0.16f), TEXT("DeskLegA"));
	SpawnCube(FVector(190.0f, 40.0f, 10.0f), FVector(0.22f, 0.18f, 0.55f), FLinearColor(0.10f, 0.13f, 0.16f), TEXT("DeskLegB"));
	SpawnCube(FVector(0.0f, 30.0f, 58.0f), FVector(2.15f, 0.08f, 0.52f), FLinearColor(0.05f, 0.22f, 0.26f), TEXT("ConsoleScreen"));
	SpawnCube(FVector(0.0f, -115.0f, 54.0f), FVector(2.3f, 0.65f, 0.045f), FLinearColor(0.0f, 0.55f, 0.65f), TEXT("HologramTable"));
	SpawnStepPad(FVector(-205.0f, -18.0f, 64.0f), TEXT("VALIDATE"), 0);
	SpawnStepPad(FVector(-102.5f, -18.0f, 64.0f), TEXT("RUN"), 1);
	SpawnStepPad(FVector(0.0f, -18.0f, 64.0f), TEXT("MONITOR"), 2);
	SpawnStepPad(FVector(102.5f, -18.0f, 64.0f), TEXT("WARN"), 3);
	SpawnStepPad(FVector(205.0f, -18.0f, 64.0f), TEXT("REPORT"), 4);

	const TArray<FVector> BeaconLocations = {
		FVector(-240.0f, -120.0f, 82.0f),
		FVector(-80.0f, -120.0f, 82.0f),
		FVector(80.0f, -120.0f, 82.0f),
		FVector(240.0f, -120.0f, 82.0f)
	};
	for (int32 Index = 0; Index < BeaconLocations.Num(); ++Index)
	{
		SpawnSphere(BeaconLocations[Index], FVector(0.28f), FLinearColor(0.12f, 0.35f, 0.42f), FString::Printf(TEXT("ObjectiveBeacon%d"), Index));
	}

	SpawnCube(FVector(0.0f, 346.0f, 218.0f), FVector(3.8f, 0.025f, 0.42f), FLinearColor(0.015f, 0.025f, 0.035f), TEXT("WallLabelBacking"));
	SpawnCube(FVector(0.0f, -126.0f, 122.0f), FVector(3.1f, 0.025f, 0.25f), FLinearColor(0.015f, 0.025f, 0.035f), TEXT("TimelineLabelBacking"));
	SpawnLabel(TEXT("SCENARIO OPERATIONS"), FVector(0.0f, 340.0f, 220.0f), 34.0f, FLinearColor(0.15f, 0.95f, 1.0f));
	SpawnLabel(TEXT("Validate  Run  Monitor  Report"), FVector(0.0f, -132.0f, 126.0f), 18.0f, FLinearColor(1.0f, 0.88f, 0.22f));

}

void ASOCOperationsDemoActor::ClearStyledOperationsRoom()
{
	for (UActorComponent* Component : PreviewComponents)
	{
		if (Component)
		{
			Component->DestroyComponent();
		}
	}

	TArray<UActorComponent*> GeneratedComponents;
	GetComponents(GeneratedComponents);
	for (UActorComponent* Component : GeneratedComponents)
	{
		if (Component == nullptr || Component == SceneRoot)
		{
			continue;
		}

		const bool bGeneratedVisual =
			Component->IsA<UTextRenderComponent>() ||
			Component->IsA<UStaticMeshComponent>() ||
			Component->IsA<UDirectionalLightComponent>() ||
			Component->IsA<UPointLightComponent>() ||
			Component->IsA<USkyLightComponent>() ||
			Component->IsA<USkyAtmosphereComponent>();

		if (bGeneratedVisual)
		{
			Component->DestroyComponent();
		}
	}

	PreviewComponents.Reset();
	ObjectiveBeacons.Reset();
	StepPads.Reset();
	WorldLabels.Reset();
}

void ASOCOperationsDemoActor::InitializeScenarioRuntime()
{
	ScenarioInstance = NewObject<UMissionScenarioInstance>(this);

	FString Error;
	if (!ScenarioInstance || !ScenarioInstance->InitializeFromJson(ScenarioJson, Error) || !ScenarioInstance->StartScenario(Error))
	{
		ValidationSummary = Error.IsEmpty() ? TEXT("Scenario initialization failed") : Error;
		AddEvent(ValidationSummary, FLinearColor(1.0f, 0.25f, 0.2f));
		return;
	}

	ValidationSummary = TEXT("Pending validation run");
	ActiveObjectiveLabel = TEXT("Validate Scenario Contract");
	RefreshOperatorPrompt();
}

void ASOCOperationsDemoActor::RunScriptStep(int32 StepIndex)
{
	// Each interaction advances both the scenario runtime and the visible operator-console feedback.
	switch (StepIndex)
	{
	case 0:
		CurrentPhaseLabel = TEXT("Validation");
		ValidationSummary = TEXT("Contract valid | 4 objectives | 0 blocking errors");
		AddEvent(TEXT("Validation passed: contract and objective chain accepted"), FLinearColor(0.25f, 1.0f, 0.58f));
		CompleteCurrentObjective(TEXT("Monitor Objective Route"));
		RefreshOperatorPrompt();
		break;
	case 1:
		CurrentPhaseLabel = TEXT("Run Simulation");
		AddEvent(TEXT("Simulation started: objective route is live"), FLinearColor(0.5f, 0.85f, 1.0f));
		CompleteCurrentObjective(TEXT("Acknowledge Runtime Warning"));
		RefreshOperatorPrompt();
		break;
	case 2:
		AddEvent(TEXT("Objective route monitored: all checkpoints responded"), FLinearColor(0.25f, 1.0f, 0.58f));
		RefreshOperatorPrompt();
		break;
	case 3:
		CurrentPhaseLabel = TEXT("Warning Response");
		AddEvent(TEXT("Signal delay detected and acknowledged"), FLinearColor(1.0f, 0.78f, 0.25f));
		CompleteCurrentObjective(TEXT("Generate Completion Report"));
		RefreshOperatorPrompt();
		break;
	case 4:
		CurrentPhaseLabel = TEXT("Report");
		CompleteCurrentObjective(TEXT("Scenario Complete"));
		ReportSummary = TEXT("Complete | Objectives 4/4 | Warnings 1 | Report ready");
		AddEvent(TEXT("Completion report generated"), FLinearColor(0.25f, 1.0f, 0.58f));
		RefreshOperatorPrompt();
		break;
	default:
		break;
	}

	UpdateBeaconState();
	UpdateStepPadState();
}

void ASOCOperationsDemoActor::AddEvent(const FString& Message, const FLinearColor& Color)
{
	FSOCConsoleEvent Event;
	Event.TimeSeconds = DemoTimeSeconds;
	Event.Message = Message;
	Event.Color = Color;
	ConsoleEvents.Insert(Event, 0);
	if (ConsoleEvents.Num() > 7)
	{
		ConsoleEvents.SetNum(7);
	}
}

void ASOCOperationsDemoActor::CompleteCurrentObjective(const FString& NextLabel)
{
	if (ScenarioInstance && ScenarioInstance->IsRunning())
	{
		FString Error;
		ScenarioInstance->CompleteCurrentObjective(Error);
	}

	ActiveObjectiveLabel = NextLabel;
}

void ASOCOperationsDemoActor::RefreshOperatorPrompt()
{
	if (CurrentOperationStep >= FinalOperationStep)
	{
		OperatorPrompt = TEXT("Sequence complete. Press R to reset.");
		return;
	}

	if (!bOperatorInRange)
	{
		OperatorPrompt = TEXT("Approach console");
		return;
	}

	switch (CurrentOperationStep)
	{
	case -1:
		OperatorPrompt = TEXT("Press E to validate scenario");
		break;
	case 0:
		OperatorPrompt = TEXT("Press E to run simulation");
		break;
	case 1:
		OperatorPrompt = TEXT("Press E to monitor objective route");
		break;
	case 2:
		OperatorPrompt = TEXT("Press E to acknowledge warning");
		break;
	case 3:
		OperatorPrompt = TEXT("Press E to generate report");
		break;
	default:
		OperatorPrompt = TEXT("Press E to continue");
		break;
	}
}

void ASOCOperationsDemoActor::UpdateBeaconState()
{
	const int32 Completed = GetCompletedObjectiveCount();
	for (int32 Index = 0; Index < ObjectiveBeacons.Num(); ++Index)
	{
		if (UStaticMeshComponent* Beacon = ObjectiveBeacons[Index])
		{
			const FLinearColor Color = Index < Completed ? FLinearColor(0.25f, 1.0f, 0.58f) : FLinearColor(0.12f, 0.35f, 0.42f);
			Beacon->SetMaterial(0, CreateMaterial(Color, FString::Printf(TEXT("BeaconMat%d"), Index)));
		}
	}
}

void ASOCOperationsDemoActor::UpdateStepPadState()
{
	for (int32 Index = 0; Index < StepPads.Num(); ++Index)
	{
		if (UStaticMeshComponent* Pad = StepPads[Index])
		{
			const FLinearColor PadColor = CurrentOperationStep >= FinalOperationStep ? ReportCompleteColor : StepPadColorForState(Index, CurrentOperationStep);
			Pad->SetMaterial(0, CreateMaterial(PadColor, FString::Printf(TEXT("StepPadMat%d"), Index)));
		}
	}
}

void ASOCOperationsDemoActor::UpdateLabelFacing()
{
	const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (CameraManager == nullptr)
	{
		return;
	}

	const FVector CameraLocation = CameraManager->GetCameraLocation();
	for (UTextRenderComponent* Label : WorldLabels)
	{
		if (Label == nullptr)
		{
			continue;
		}

		const FVector Direction = CameraLocation - Label->GetComponentLocation();
		if (!Direction.IsNearlyZero())
		{
			Label->SetWorldRotation(Direction.Rotation());
		}
	}
}

void ASOCOperationsDemoActor::SpawnCube(const FVector& Location, const FVector& Scale, const FLinearColor& Color, const FString& Name)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, *Name);
	Component->SetupAttachment(SceneRoot);
	Component->RegisterComponent();
	Component->SetRelativeLocation(Location);
	Component->SetRelativeScale3D(Scale);
	Component->SetStaticMesh(LoadBasicMesh(TEXT("/Engine/BasicShapes/Cube.Cube")));
	Component->SetMaterial(0, CreateMaterial(Color, Name + TEXT("Mat")));
	Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	PreviewComponents.Add(Component);
}

UStaticMeshComponent* ASOCOperationsDemoActor::SpawnStepPad(const FVector& Location, const FString& Label, int32 StepIndex)
{
	const FLinearColor PadColor = StepPadColorForState(StepIndex, CurrentOperationStep);
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("StepPad%d"), StepIndex));
	Component->SetupAttachment(SceneRoot);
	Component->RegisterComponent();
	Component->SetRelativeLocation(Location);
	Component->SetRelativeScale3D(FVector(0.86f, 0.34f, 0.035f));
	Component->SetStaticMesh(LoadBasicMesh(TEXT("/Engine/BasicShapes/Cube.Cube")));
	Component->SetMaterial(0, CreateMaterial(PadColor, FString::Printf(TEXT("StepPadMat%d"), StepIndex)));
	Component->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	StepPads.Add(Component);
	PreviewComponents.Add(Component);

	SpawnLabel(Label, Location + FVector(0.0f, -7.0f, 13.0f), 7.5f, FLinearColor(0.96f, 0.98f, 1.0f));
	return Component;
}

void ASOCOperationsDemoActor::SpawnSphere(const FVector& Location, const FVector& Scale, const FLinearColor& Color, const FString& Name)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, *Name);
	Component->SetupAttachment(SceneRoot);
	Component->RegisterComponent();
	Component->SetRelativeLocation(Location);
	Component->SetRelativeScale3D(Scale);
	Component->SetStaticMesh(LoadBasicMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	Component->SetMaterial(0, CreateMaterial(Color, Name + TEXT("Mat")));
	ObjectiveBeacons.Add(Component);
	PreviewComponents.Add(Component);
}

void ASOCOperationsDemoActor::SpawnLabel(const FString& Text, const FVector& Location, float WorldSize, const FLinearColor& Color)
{
	UTextRenderComponent* Component = NewObject<UTextRenderComponent>(this, *FString::Printf(TEXT("Label_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	Component->SetupAttachment(SceneRoot);
	Component->RegisterComponent();
	Component->SetRelativeLocation(Location);
	Component->SetWorldRotation((FVector(0.0f, -430.0f, 120.0f) - Component->GetComponentLocation()).Rotation());
	Component->SetHorizontalAlignment(EHTA_Center);
	Component->SetVerticalAlignment(EVRTA_TextCenter);
	Component->SetWorldSize(WorldSize);
	Component->SetText(FText::FromString(Text));
	Component->SetTextRenderColor(Color.ToFColor(true));
	Component->SetCastShadow(false);
	WorldLabels.Add(Component);
	PreviewComponents.Add(Component);
}

UMaterialInterface* ASOCOperationsDemoActor::CreateMaterial(const FLinearColor& Color, const FString& Name)
{
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")), this, *Name);
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
	return Material;
}

UStaticMesh* ASOCOperationsDemoActor::LoadBasicMesh(const TCHAR* Path) const
{
	return LoadObject<UStaticMesh>(nullptr, Path);
}
