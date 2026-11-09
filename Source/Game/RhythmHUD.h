#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RhythmHUD.generated.h"

class ARhythmGameMode;

UCLASS()
class GAME_API ARhythmHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	void HandleClick(const FVector2D& ScreenPos);

private:
	enum class EAction : uint8
	{
		None,
		MenuPlay,
		MenuQuit,
		SongRow,
		SongPlay,
		SongBack,
		PauseResume,
		PauseRetry,
		PauseQuit,
		ResultRetry,
		ResultSongs,
		ResultMenu
	};

	struct FButton
	{
		FVector2D Pos = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
		EAction Action = EAction::None;
		int32 Index = -1;
	};

	TArray<FButton> Buttons;
	ARhythmGameMode* GM = nullptr;
	float UI = 1.0f;
	FVector2D Mouse = FVector2D::ZeroVector;
	bool bHasMouse = false;

	void AddButton(const FVector2D& Pos, const FVector2D& Size, EAction Action, int32 Index = -1);
	bool IsHovered(const FButton& B) const;

	void Tex(const FVector2D& Pos, const FVector2D& Size, UTexture2D* Texture, const FLinearColor& Tint, float Rotation = 0.0f);
	void Rect(const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Color);
	void Label(const FString& Text, const FVector2D& Pos, float Scale, const FLinearColor& Color, bool bCenterX = false, bool bCenterY = false);
	void LabelRight(const FString& Text, const FVector2D& Pos, float Scale, const FLinearColor& Color);
	void LabelShadowed(const FString& Text, const FVector2D& Pos, float Scale, const FLinearColor& Color, bool bCenterX, bool bCenterY);

	void DrawBackground(const FLinearColor& Tint);
	void DrawCover(UTexture2D* Texture, const FLinearColor& Tint);
	void DrawCoverIn(UTexture2D* Texture, const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Tint);
	void DrawHitErrorMeter();
	void DrawMenu();
	void DrawSongSelect();
	void DrawPlayScene();
	void DrawPlayingHud();
	void DrawPause();
	void DrawResults();
	void DrawButton(const FString& Text, const FVector2D& Pos, const FVector2D& Size, EAction Action, int32 Index, bool bSelected);
	FString FormatNumber(int64 Value) const;
	FString FormatTime(double Ms) const;
};
