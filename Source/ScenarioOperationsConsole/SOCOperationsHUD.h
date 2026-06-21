#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SOCOperationsHUD.generated.h"

class ASOCOperationsDemoActor;

UCLASS()
class SCENARIOOPERATIONSCONSOLE_API ASOCOperationsHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	TWeakObjectPtr<ASOCOperationsDemoActor> CachedDemoActor;

	ASOCOperationsDemoActor* GetDemoActor();
	void DrawPanel(float X, float Y, float W, float H, const FLinearColor& Fill, const FLinearColor& Stroke);
	void DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.0f);
	void DrawWrappedLabel(const FString& Text, float X, float Y, float MaxWidth, const FLinearColor& Color, float Scale = 1.0f);
	TArray<FString> WrapText(const FString& Text, int32 MaxCharsPerLine) const;
};
