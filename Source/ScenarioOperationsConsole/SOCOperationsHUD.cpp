#include "SOCOperationsHUD.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "SOCOperationsDemoActor.h"

namespace
{
const FLinearColor PanelFill(0.025f, 0.04f, 0.055f, 0.94f);
const FLinearColor PanelStroke(0.0f, 0.78f, 0.95f, 0.95f);
const FLinearColor TextPrimary(0.92f, 0.96f, 0.96f, 1.0f);
const FLinearColor TextMuted(0.74f, 0.84f, 0.88f, 1.0f);
const FLinearColor AccentYellow(1.0f, 0.82f, 0.28f, 1.0f);
}

void ASOCOperationsHUD::DrawHUD()
{
	Super::DrawHUD();

	ASOCOperationsDemoActor* DemoActor = GetDemoActor();
	if (!Canvas || !DemoActor)
	{
		return;
	}

	const float Pad = 28.0f;
	const float LeftW = 390.0f;
	const float RightW = 430.0f;
	const float BottomH = 178.0f;
	const float H = Canvas->SizeY;
	const float W = Canvas->SizeX;

	DrawPanel(Pad, Pad, LeftW, 230.0f, PanelFill, PanelStroke);
	DrawLabel(TEXT("SCENARIO OPERATIONS"), Pad + 18.0f, Pad + 16.0f, PanelStroke, 1.1f);
	DrawLabel(FString::Printf(TEXT("Phase: %s"), *DemoActor->GetCurrentPhaseLabel()), Pad + 18.0f, Pad + 58.0f, TextPrimary);
	DrawWrappedLabel(FString::Printf(TEXT("Objective: %s"), *DemoActor->GetActiveObjectiveLabel()), Pad + 18.0f, Pad + 91.0f, LeftW - 36.0f, TextPrimary);
	DrawLabel(FString::Printf(TEXT("Progress: %d / %d"), DemoActor->GetCompletedObjectiveCount(), DemoActor->GetTotalObjectiveCount()), Pad + 18.0f, Pad + 145.0f, TextPrimary);
	DrawLabel(FString::Printf(TEXT("Elapsed: %02.0fs"), DemoActor->GetDemoTimeSeconds()), Pad + 18.0f, Pad + 174.0f, TextMuted);
	DrawWrappedLabel(DemoActor->GetOperatorPrompt(), Pad + 18.0f, Pad + 199.0f, LeftW - 36.0f, DemoActor->IsOperatorInRange() ? AccentYellow : TextMuted, 0.92f);

	DrawPanel(W - RightW - Pad, Pad, RightW, 230.0f, PanelFill, PanelStroke);
	DrawLabel(TEXT("VALIDATION"), W - RightW - Pad + 18.0f, Pad + 16.0f, PanelStroke, 1.1f);
	DrawWrappedLabel(DemoActor->GetValidationSummary(), W - RightW - Pad + 18.0f, Pad + 58.0f, RightW - 36.0f, TextPrimary);
	const FString WarningText = DemoActor->GetCompletedObjectiveCount() >= 3 ? TEXT("Runtime warning: Signal delay acknowledged") : TEXT("Runtime warning: Pending operator review");
	DrawWrappedLabel(WarningText, W - RightW - Pad + 18.0f, Pad + 105.0f, RightW - 36.0f, AccentYellow);
	DrawWrappedLabel(DemoActor->GetReportSummary(), W - RightW - Pad + 18.0f, Pad + 158.0f, RightW - 36.0f, TextMuted);
	if (DemoActor->IsScenarioComplete())
	{
		DrawWrappedLabel(TEXT("REPORT GENERATED"), W - RightW - Pad + 18.0f, Pad + 196.0f, RightW - 36.0f, FLinearColor(0.20f, 0.95f, 0.46f, 1.0f), 0.9f);
	}

	DrawPanel(Pad, H - BottomH - Pad, W - (Pad * 2.0f), BottomH, PanelFill, PanelStroke);
	DrawLabel(TEXT("EVENT FEED"), Pad + 18.0f, H - BottomH - Pad + 16.0f, PanelStroke, 1.05f);

	const TArray<FSOCConsoleEvent>& Events = DemoActor->GetConsoleEvents();
	for (int32 Index = 0; Index < Events.Num(); ++Index)
	{
		const FSOCConsoleEvent& Event = Events[Index];
		DrawWrappedLabel(FString::Printf(TEXT("[%02.0fs] %s"), Event.TimeSeconds, *Event.Message), Pad + 18.0f, H - BottomH - Pad + 49.0f + (Index * 22.0f), W - (Pad * 2.0f) - 36.0f, Event.Color, 0.88f);
	}
}

ASOCOperationsDemoActor* ASOCOperationsHUD::GetDemoActor()
{
	if (CachedDemoActor.IsValid())
	{
		return CachedDemoActor.Get();
	}

	for (TActorIterator<ASOCOperationsDemoActor> It(GetWorld()); It; ++It)
	{
		CachedDemoActor = *It;
		return CachedDemoActor.Get();
	}

	return nullptr;
}

void ASOCOperationsHUD::DrawPanel(float X, float Y, float W, float H, const FLinearColor& Fill, const FLinearColor& Stroke)
{
	DrawRect(Fill, X, Y, W, H);
	DrawRect(Stroke, X, Y, 3.0f, H);
	DrawRect(Stroke, X, Y, W, 2.0f);
}

void ASOCOperationsHUD::DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale)
{
	FCanvasTextItem TextItem(FVector2D(X, Y), FText::FromString(Text), GEngine->GetSmallFont(), Color);
	TextItem.Scale = FVector2D(Scale, Scale);
	TextItem.EnableShadow(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	Canvas->DrawItem(TextItem);
}

void ASOCOperationsHUD::DrawWrappedLabel(const FString& Text, float X, float Y, float MaxWidth, const FLinearColor& Color, float Scale)
{
	// Canvas HUD text does not auto-wrap, so keep panel copy readable at common window sizes.
	const int32 MaxChars = FMath::Max(18, FMath::FloorToInt(MaxWidth / (8.0f * Scale)));
	const TArray<FString> Lines = WrapText(Text, MaxChars);
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		DrawLabel(Lines[Index], X, Y + (Index * 19.0f * Scale), Color, Scale);
	}
}

TArray<FString> ASOCOperationsHUD::WrapText(const FString& Text, int32 MaxCharsPerLine) const
{
	TArray<FString> Lines;
	TArray<FString> Words;
	Text.ParseIntoArrayWS(Words);

	FString CurrentLine;
	for (const FString& Word : Words)
	{
		const FString Candidate = CurrentLine.IsEmpty() ? Word : CurrentLine + TEXT(" ") + Word;
		if (Candidate.Len() > MaxCharsPerLine && !CurrentLine.IsEmpty())
		{
			Lines.Add(CurrentLine);
			CurrentLine = Word;
		}
		else
		{
			CurrentLine = Candidate;
		}
	}

	if (!CurrentLine.IsEmpty())
	{
		Lines.Add(CurrentLine);
	}

	return Lines;
}
