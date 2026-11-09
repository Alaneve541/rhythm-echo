#include "RhythmHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "RhythmGameMode.h"
#include "RhythmTypes.h"

namespace
{
	const FLinearColor Pink(1.0f, 0.31f, 0.62f, 1.0f);
	const FLinearColor Cyan(0.36f, 0.86f, 1.0f, 1.0f);
	const FLinearColor White(1.0f, 1.0f, 1.0f, 1.0f);
	const FLinearColor Dim(0.72f, 0.76f, 0.86f, 1.0f);
	const FLinearColor Dark(0.025f, 0.025f, 0.045f, 1.0f);
	const FLinearColor Panel(0.06f, 0.06f, 0.10f, 0.94f);
}

void ARhythmHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}
	GM = Cast<ARhythmGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM)
	{
		return;
	}

	UI = Canvas->SizeY / 720.0f;
	Buttons.Reset();
	bHasMouse = false;
	if (PlayerOwner)
	{
		float X = 0.0f, Y = 0.0f;
		bHasMouse = PlayerOwner->GetMousePosition(X, Y);
		Mouse = FVector2D(X, Y);
		PlayerOwner->CurrentMouseCursor = (GM->Screen == ERhythmScreen::Menu || GM->Screen == ERhythmScreen::SongSelect
			|| GM->Screen == ERhythmScreen::Paused || GM->Screen == ERhythmScreen::Results)
			? EMouseCursor::Default : EMouseCursor::None;
	}

	switch (GM->Screen)
	{
	case ERhythmScreen::Menu:
		DrawMenu();
		break;
	case ERhythmScreen::SongSelect:
		DrawSongSelect();
		break;
	case ERhythmScreen::Playing:
		DrawPlayScene();
		DrawPlayingHud();
		break;
	case ERhythmScreen::Paused:
		DrawPlayScene();
		DrawPlayingHud();
		DrawPause();
		break;
	case ERhythmScreen::Results:
		DrawResults();
		break;
	}

	if (!GM->LaunchError.IsEmpty())
	{
		Label(GM->LaunchError, FVector2D(Canvas->SizeX * 0.5f, 60.0f * UI), 1.0f, FLinearColor(1.0f, 0.4f, 0.4f, 1.0f), true, false);
	}

	GM->OnHudDrawn();
}

void ARhythmHUD::AddButton(const FVector2D& Pos, const FVector2D& Size, EAction Action, int32 Index)
{
	FButton B;
	B.Pos = Pos;
	B.Size = Size;
	B.Action = Action;
	B.Index = Index;
	Buttons.Add(B);
}

bool ARhythmHUD::IsHovered(const FButton& B) const
{
	return bHasMouse && Mouse.X >= B.Pos.X && Mouse.X <= B.Pos.X + B.Size.X && Mouse.Y >= B.Pos.Y && Mouse.Y <= B.Pos.Y + B.Size.Y;
}

void ARhythmHUD::HandleClick(const FVector2D& ScreenPos)
{
	for (int32 i = Buttons.Num() - 1; i >= 0; --i)
	{
		const FButton& B = Buttons[i];
		if (ScreenPos.X >= B.Pos.X && ScreenPos.X <= B.Pos.X + B.Size.X && ScreenPos.Y >= B.Pos.Y && ScreenPos.Y <= B.Pos.Y + B.Size.Y)
		{
			switch (B.Action)
			{
			case EAction::MenuPlay: GM->OpenSongSelect(); break;
			case EAction::MenuQuit: GM->RequestQuitGame(); break;
			case EAction::SongRow: GM->SelectSong(B.Index); break;
			case EAction::SongPlay: GM->StartSelectedSong(); break;
			case EAction::SongBack: GM->OpenMenu(); break;
			case EAction::PauseResume: GM->Resume(); break;
			case EAction::PauseRetry: GM->RestartSong(); break;
			case EAction::PauseQuit: GM->QuitToMenu(); break;
			case EAction::ResultRetry: GM->RestartSong(); break;
			case EAction::ResultSongs: GM->OpenSongSelect(); break;
			case EAction::ResultMenu: GM->OpenMenu(); break;
			default: break;
			}
			return;
		}
	}
}

void ARhythmHUD::Tex(const FVector2D& Pos, const FVector2D& Size, UTexture2D* Texture, const FLinearColor& Tint, float Rotation)
{
	Canvas->K2_DrawTexture(Texture, Pos, Size, FVector2D(0, 0), FVector2D(1, 1), Tint, BLEND_Translucent, Rotation, FVector2D(0.5f, 0.5f));
}

void ARhythmHUD::Rect(const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Color)
{
	Canvas->K2_DrawTexture(nullptr, Pos, Size, FVector2D(0, 0), FVector2D(1, 1), Color, BLEND_Translucent);
}

void ARhythmHUD::Label(const FString& Text, const FVector2D& Pos, float Scale, const FLinearColor& Color, bool bCenterX, bool bCenterY)
{
	if (!GEngine || !GEngine->GetMediumFont())
	{
		return;
	}
	Canvas->K2_DrawText(GEngine->GetMediumFont(), Text, Pos, FVector2D(Scale, Scale), Color, 0.0f,
		FLinearColor(0.0f, 0.0f, 0.0f, 0.8f), FVector2D(1.0f, 1.0f), bCenterX, bCenterY, false, FLinearColor::Black);
}

void ARhythmHUD::LabelRight(const FString& Text, const FVector2D& Pos, float Scale, const FLinearColor& Color)
{
	if (!GEngine || !GEngine->GetMediumFont())
	{
		return;
	}
	const FVector2D Size = Canvas->K2_TextSize(GEngine->GetMediumFont(), Text, FVector2D(Scale, Scale));
	Label(Text, FVector2D(Pos.X - Size.X, Pos.Y), Scale, Color, false, false);
}

void ARhythmHUD::LabelShadowed(const FString& Text, const FVector2D& Pos, float Scale, const FLinearColor& Color, bool bCenterX, bool bCenterY)
{
	Label(Text, Pos, Scale, Color, bCenterX, bCenterY);
}

void ARhythmHUD::DrawBackground(const FLinearColor& Tint)
{
	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;
	Rect(FVector2D(0, 0), FVector2D(W, H), Dark);
	if (UTexture2D* BG = GM->GetBackgroundTexture())
	{
		DrawCover(BG, Tint);
	}
	Rect(FVector2D(0, 0), FVector2D(W, H * 0.24f), FLinearColor(0.08f, 0.04f, 0.13f, 0.55f));
	Rect(FVector2D(0, H * 0.72f), FVector2D(W, H * 0.28f), FLinearColor(0.01f, 0.01f, 0.03f, 0.82f));
}

void ARhythmHUD::DrawCover(UTexture2D* Texture, const FLinearColor& Tint)
{
	if (!Texture)
	{
		return;
	}
	const float TW = static_cast<float>(Texture->GetSizeX());
	const float TH = static_cast<float>(Texture->GetSizeY());
	if (TW <= 1.0f || TH <= 1.0f)
	{
		return;
	}
	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;
	const float Scale = FMath::Max(W / TW, H / TH);
	const FVector2D Size(TW * Scale, TH * Scale);
	Tex(FVector2D((W - Size.X) * 0.5f, (H - Size.Y) * 0.5f), Size, Texture, Tint);
}

void ARhythmHUD::DrawCoverIn(UTexture2D* Texture, const FVector2D& Pos, const FVector2D& Size, const FLinearColor& Tint)
{
	if (!Texture)
	{
		Rect(Pos, Size, FLinearColor(0.08f, 0.08f, 0.12f, 1.0f));
		return;
	}
	const float TW = static_cast<float>(Texture->GetSizeX());
	const float TH = static_cast<float>(Texture->GetSizeY());
	if (TW <= 1.0f || TH <= 1.0f)
	{
		return;
	}
	const float Scale = FMath::Min(Size.X / TW, Size.Y / TH);
	const FVector2D DrawSize(TW * Scale, TH * Scale);
	Tex(Pos + (Size - DrawSize) * 0.5f, DrawSize, Texture, Tint);
}

void ARhythmHUD::DrawMenu()
{
	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;
	DrawBackground(FLinearColor(0.26f, 0.24f, 0.34f, 1.0f));

	if (UTexture2D* Star = GM->GetTexture(TEXT("star")))
	{
		Tex(FVector2D(W * 0.74f, H * 0.12f), FVector2D(130.0f * UI, 130.0f * UI), Star, FLinearColor(0.4f, 0.9f, 1.0f, 0.18f));
		Tex(FVector2D(W * 0.15f, H * 0.60f), FVector2D(240.0f * UI, 240.0f * UI), Star, FLinearColor(1.0f, 0.4f, 0.7f, 0.12f));
	}

	Label(TEXT("RHYTHM ECHO"), FVector2D(W * 0.5f, H * 0.22f), 4.4f, White, true, true);
	Label(TEXT("4-KEY MANIA"), FVector2D(W * 0.5f, H * 0.32f), 1.8f, Cyan, true, true);
	Label(TEXT("falling notes  ·  hold the long ones  ·  chase the combo"), FVector2D(W * 0.5f, H * 0.385f), 1.0f, Dim, true, true);

	const FVector2D BtnSize(360.0f * UI, 64.0f * UI);
	DrawButton(TEXT("Play"), FVector2D(W * 0.5f - BtnSize.X * 0.5f, H * 0.50f), BtnSize, EAction::MenuPlay, -1, GM->SelectedMenuButton == 0);
	DrawButton(TEXT("Quit"), FVector2D(W * 0.5f - BtnSize.X * 0.5f, H * 0.50f + 86.0f * UI), BtnSize, EAction::MenuQuit, -1, GM->SelectedMenuButton == 1);

	Label(TEXT("A   S   ;   '      hit the four lanes        Esc pauses"), FVector2D(W * 0.5f, H - 62.0f * UI), 0.95f, Dim, true, true);
	Label(FString::Printf(TEXT("%d charts  ·  osu!mania 4K library"), GM->Songs.Num()), FVector2D(24.0f * UI, H - 32.0f * UI), 0.72f, FLinearColor(0.55f, 0.58f, 0.68f, 1.0f));
	LabelRight(TEXT("RHYTHM ECHO  /  private build"), FVector2D(W - 24.0f * UI, H - 32.0f * UI), 0.66f, FLinearColor(0.45f, 0.48f, 0.58f, 1.0f));
}

void ARhythmHUD::DrawButton(const FString& Text, const FVector2D& Pos, const FVector2D& Size, EAction Action, int32 Index, bool bSelected)
{
	FButton B;
	B.Pos = Pos;
	B.Size = Size;
	B.Action = Action;
	B.Index = Index;
	const bool bHover = IsHovered(B);
	Buttons.Add(B);

	if (UTexture2D* Bg = GM->GetTexture(TEXT("menu-button-background")))
	{
		const FLinearColor Tint = bSelected || bHover ? FLinearColor(0.55f, 0.85f, 1.0f, 0.95f) : FLinearColor(0.20f, 0.20f, 0.28f, 0.92f);
		Tex(Pos, Size, Bg, Tint);
	}
	else
	{
		Rect(Pos, Size, bSelected || bHover ? FLinearColor(0.2f, 0.6f, 0.8f, 0.95f) : FLinearColor(0.12f, 0.12f, 0.18f, 0.95f));
	}
	Rect(Pos, FVector2D(5.0f * UI, Size.Y), bSelected || bHover ? Cyan : FLinearColor(0.3f, 0.32f, 0.42f, 1.0f));
	Label(Text, Pos + Size * 0.5f, 1.5f, White, true, true);
}

void ARhythmHUD::DrawSongSelect()
{
	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;
	DrawBackground(FLinearColor(0.22f, 0.22f, 0.30f, 1.0f));

	Label(TEXT("Song Select"), FVector2D(40.0f * UI, 28.0f * UI), 2.2f, White);
	LabelRight(TEXT("Up / Down choose    Enter play    Esc back"), FVector2D(W - 40.0f * UI, 40.0f * UI), 0.78f, Dim);

	const float RowX = 40.0f * UI;
	const float RowW = W * 0.55f;
	const float RowH = 70.0f * UI;
	float Y = 104.0f * UI;

	for (int32 i = 0; i < GM->Songs.Num(); ++i)
	{
		const FRhythmChart& Song = GM->Songs[i];
		FButton Row;
		Row.Pos = FVector2D(RowX, Y);
		Row.Size = FVector2D(RowW, RowH - 8.0f * UI);
		Row.Action = EAction::SongRow;
		Row.Index = i;
		const bool bSelected = i == GM->SelectedSong;
		const bool bHover = IsHovered(Row);

		Rect(Row.Pos, Row.Size, bSelected ? FLinearColor(0.10f, 0.40f, 0.55f, 0.90f) : (bHover ? FLinearColor(0.13f, 0.13f, 0.20f, 0.9f) : FLinearColor(0.05f, 0.05f, 0.09f, 0.78f)));
		if (bSelected)
		{
			Rect(Row.Pos, FVector2D(5.0f * UI, Row.Size.Y), Cyan);
		}
		Buttons.Add(Row);

		Label(Song.Title, FVector2D(RowX + 18.0f * UI, Y + 7.0f * UI), 1.18f, White);
		Label(FString::Printf(TEXT("%s   //   [%s]"), *Song.Artist, *Song.Version), FVector2D(RowX + 18.0f * UI, Y + 34.0f * UI), 0.76f, Dim);
		LabelRight(FString::Printf(TEXT("4K   %.0f BPM   %s"), Song.BPM, *FormatTime(Song.LastTime)), FVector2D(RowX + RowW - 16.0f * UI, Y + 11.0f * UI), 0.7f, Dim);
		LabelRight(FString::Printf(TEXT("%d notes   %d LN   OD %.1f"), Song.ObjectCount(), Song.HoldCount, Song.OD), FVector2D(RowX + RowW - 16.0f * UI, Y + 36.0f * UI), 0.7f, Dim);

		Y += RowH;
	}

	const float PX = W * 0.62f;
	const float PW = W * 0.35f;
	Rect(FVector2D(PX, 104.0f * UI), FVector2D(PW, H - 224.0f * UI), FLinearColor(0.04f, 0.04f, 0.08f, 0.86f));

	if (GM->Songs.IsValidIndex(GM->SelectedSong))
	{
		const FRhythmChart& Song = GM->Songs[GM->SelectedSong];
		const FVector2D ThumbPos(PX + 16.0f * UI, 120.0f * UI);
		const FVector2D ThumbSize(PW - 32.0f * UI, (PW - 32.0f * UI) * 9.0f / 16.0f);
		DrawCoverIn(GM->GetSongBackgroundTexture(GM->SelectedSong), ThumbPos, ThumbSize, FLinearColor(0.9f, 0.9f, 0.95f, 1.0f));
		Rect(ThumbPos, ThumbSize, FLinearColor(0.0f, 0.0f, 0.0f, 0.08f));

		float SY = ThumbPos.Y + ThumbSize.Y + 18.0f * UI;
		Label(Song.Title, FVector2D(PX + 20.0f * UI, SY), 1.5f, White);
		SY += 34.0f * UI;
		Label(Song.Artist, FVector2D(PX + 20.0f * UI, SY), 0.92f, Dim);
		SY += 26.0f * UI;
		Label(FString::Printf(TEXT("[%s]  by %s"), *Song.Version, *Song.Creator), FVector2D(PX + 20.0f * UI, SY), 0.78f, Cyan);
		SY += 34.0f * UI;

		Label(FString::Printf(TEXT("Keys            %d"), Song.Keys), FVector2D(PX + 20.0f * UI, SY), 0.86f, Dim); SY += 25.0f * UI;
		Label(FString::Printf(TEXT("Overall diff    %.1f"), Song.OD), FVector2D(PX + 20.0f * UI, SY), 0.86f, Dim); SY += 25.0f * UI;
		Label(FString::Printf(TEXT("HP drain        %.1f"), Song.HP), FVector2D(PX + 20.0f * UI, SY), 0.86f, Dim); SY += 25.0f * UI;
		Label(FString::Printf(TEXT("BPM             %.1f"), Song.BPM), FVector2D(PX + 20.0f * UI, SY), 0.86f, Dim); SY += 25.0f * UI;
		Label(FString::Printf(TEXT("Notes           %d  (%d hold)"), Song.ObjectCount(), Song.HoldCount), FVector2D(PX + 20.0f * UI, SY), 0.86f, Dim); SY += 25.0f * UI;
		Label(FString::Printf(TEXT("Length          %s"), *FormatTime(Song.LastTime)), FVector2D(PX + 20.0f * UI, SY), 0.86f, Dim); SY += 30.0f * UI;
		Label(TEXT("A  S  ;  '      also D F J K"), FVector2D(PX + 20.0f * UI, SY), 0.82f, FLinearColor(0.95f, 0.78f, 0.36f, 1.0f));
	}

	const FVector2D BtnSize(250.0f * UI, 58.0f * UI);
	DrawButton(TEXT("Play"), FVector2D(PX, H - 96.0f * UI), BtnSize, EAction::SongPlay, -1, true);
	FButton Back;
	Back.Pos = FVector2D(PX + 266.0f * UI, H - 96.0f * UI);
	Back.Size = FVector2D(150.0f * UI, 58.0f * UI);
	Back.Action = EAction::SongBack;
	const bool bHover = IsHovered(Back);
	Rect(Back.Pos, Back.Size, bHover ? FLinearColor(0.2f, 0.2f, 0.3f, 1.0f) : FLinearColor(0.09f, 0.09f, 0.15f, 1.0f));
	Label(TEXT("Back"), Back.Pos + Back.Size * 0.5f, 1.0f, Dim, true, true);
	Buttons.Add(Back);
}

void ARhythmHUD::DrawPlayScene()
{
	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;
	const float StageX = GM->StageX;
	const float StageW = GM->StageW;
	const float ColumnW = GM->ColumnW;
	const float HitY = GM->HitY;
	const float NoteH = GM->NoteH;

	Rect(FVector2D(0, 0), FVector2D(W, H), FLinearColor(0.012f, 0.012f, 0.02f, 1.0f));
	if (UTexture2D* BG = GM->GetBackgroundTexture())
	{
		// osu!'s DimLevel 80 leaves the picture at 20% brightness behind the stage.
		DrawCover(BG, FLinearColor(0.16f, 0.16f, 0.20f, 1.0f));
	}

	// decorative side panels from the player's skin
	if (UTexture2D* Left = GM->GetTexture(TEXT("mania-stage-left")))
	{
		const float SideW = 275.0f * (H / 768.0f) * 0.9f;
		Tex(FVector2D(StageX - SideW, 0), FVector2D(SideW, H), Left, FLinearColor(1.0f, 1.0f, 1.0f, 0.62f));
	}
	if (UTexture2D* Right = GM->GetTexture(TEXT("mania-stage-right")))
	{
		const float SideW = 275.0f * (H / 768.0f) * 0.9f;
		Tex(FVector2D(StageX + StageW, 0), FVector2D(SideW, H), Right, FLinearColor(1.0f, 1.0f, 1.0f, 0.62f));
	}

	// dark playfield so the pastel notes read clearly
	Rect(FVector2D(StageX, 0), FVector2D(StageW, HitY + 24.0f * UI), FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
	for (int32 Lane = 0; Lane < 4; ++Lane)
	{
		const float LX = StageX + Lane * ColumnW;
		const bool bOuter = (Lane == 0 || Lane == 3);
		Rect(FVector2D(LX, 0), FVector2D(ColumnW, HitY), bOuter
			? FLinearColor(0.06f, 0.09f, 0.14f, 0.72f)
			: FLinearColor(0.11f, 0.07f, 0.12f, 0.72f));
		Rect(FVector2D(LX, 0), FVector2D(1.0f * UI, HitY), FLinearColor(1.0f, 1.0f, 1.0f, 0.055f));
	}

	// key area
	Rect(FVector2D(StageX, HitY), FVector2D(StageW, H - HitY), FLinearColor(0.015f, 0.015f, 0.03f, 0.94f));

	for (int32 Lane = 0; Lane < 4; ++Lane)
	{
		const float LX = StageX + Lane * ColumnW;
		const bool bOuter = (Lane == 0 || Lane == 3);
		const bool bDown = GM->bLaneDown[Lane];
		UTexture2D* KeyTex = GM->GetTexture(bOuter ? (bDown ? TEXT("mania-key1D") : TEXT("mania-key1"))
			: (bDown ? TEXT("mania-key2D") : TEXT("mania-key2")));
		if (KeyTex)
		{
			Tex(FVector2D(LX + 1.0f * UI, HitY), FVector2D(ColumnW - 2.0f * UI, H - HitY), KeyTex, FLinearColor(1.0f, 1.0f, 1.0f, bDown ? 1.0f : 0.92f));
		}
		if (bDown)
		{
			const FLinearColor Glow = bOuter ? FLinearColor(0.30f, 0.75f, 1.0f, 0.20f) : FLinearColor(1.0f, 0.35f, 0.65f, 0.20f);
			Rect(FVector2D(LX, HitY), FVector2D(ColumnW, H - HitY), Glow);
			for (int32 Band = 0; Band < 8; ++Band)
			{
				const float Alpha = 0.16f * (1.0f - Band / 8.0f);
				Rect(FVector2D(LX, HitY - (Band + 1) * 26.0f * UI), FVector2D(ColumnW, 26.0f * UI),
					bOuter ? FLinearColor(0.30f, 0.75f, 1.0f, Alpha) : FLinearColor(1.0f, 0.35f, 0.65f, Alpha));
			}
		}
	}

	if (UTexture2D* Hint = GM->GetTexture(TEXT("mania-stage-hint")))
	{
		Tex(FVector2D(StageX, HitY - 13.0f * UI), FVector2D(StageW, 26.0f * UI), Hint, FLinearColor(1.0f, 1.0f, 1.0f, 0.95f));
	}
	else
	{
		Rect(FVector2D(StageX, HitY - 2.0f * UI), FVector2D(StageW, 4.0f * UI), FLinearColor(0.85f, 0.95f, 1.0f, 0.85f));
	}

	// hold bodies first
	for (const FManiaNote& N : GM->CurrentChart.Notes)
	{
		if (!N.bHold || N.bJudged)
		{
			continue;
		}
		const float LX = StageX + N.Lane * ColumnW;
		const float HeadY = GM->NoteY(N.Time);
		const float TailY = GM->NoteY(N.EndTime);
		const float Top = FMath::Min(HeadY, TailY);
		const float Bottom = FMath::Max(HeadY, TailY);
		if (Bottom < -NoteH || Top > H + NoteH)
		{
			continue;
		}
		UTexture2D* Body = GM->GetTexture((N.Lane == 0 || N.Lane == 3) ? TEXT("mania-note1L") : TEXT("mania-note2L"));
		if (Body && Bottom > Top)
		{
			const float BW = ColumnW * 0.62f;
			Tex(FVector2D(LX + (ColumnW - BW) * 0.5f, Top), FVector2D(BW, Bottom - Top), Body,
				N.bHeld ? FLinearColor(1.0f, 0.95f, 0.55f, 0.95f) : FLinearColor(0.9f, 0.9f, 1.0f, 0.78f));
		}
	}

	// note heads and tails
	for (const FManiaNote& N : GM->CurrentChart.Notes)
	{
		if (N.bJudged)
		{
			continue;
		}
		const float LX = StageX + N.Lane * ColumnW;
		const float HeadY = GM->NoteY(N.Time);
		if (HeadY < -NoteH * 2.0f || HeadY - NoteH > H + NoteH)
		{
			continue;
		}
		const bool bOuter = (N.Lane == 0 || N.Lane == 3);
		if (N.bHold)
		{
			UTexture2D* Tail = GM->GetTexture(bOuter ? TEXT("mania-note1T") : TEXT("mania-note2T"));
			UTexture2D* Head = GM->GetTexture(bOuter ? TEXT("mania-note1H") : TEXT("mania-note2H"));
			const float TailY = GM->NoteY(N.EndTime);
			const FLinearColor Tint = N.bHeld ? FLinearColor(1.0f, 0.98f, 0.75f, 1.0f) : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
			if (Tail)
			{
				Tex(FVector2D(LX, TailY - NoteH), FVector2D(ColumnW, NoteH), Tail, Tint, PI);
			}
			if (Head)
			{
				Tex(FVector2D(LX, HeadY - NoteH), FVector2D(ColumnW, NoteH), Head, Tint);
			}
		}
		else
		{
			UTexture2D* NoteTex = GM->GetTexture(bOuter ? TEXT("mania-note1") : TEXT("mania-note2"));
			if (NoteTex)
			{
				Tex(FVector2D(LX, HeadY - NoteH), FVector2D(ColumnW, NoteH), NoteTex, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
			}
		}
	}

	// judgement burst
	if (GM->SongTime - GM->LastJudgementTime < 650.0)
	{
		const float Age = static_cast<float>(GM->SongTime - GM->LastJudgementTime);
		const float Alpha = FMath::Clamp(1.0f - Age / 650.0f, 0.0f, 1.0f);
		FString Name = TEXT("mania-hit0");
		if (GM->LastJudgement == 320) Name = TEXT("mania-hit300g");
		else if (GM->LastJudgement == 300) Name = TEXT("mania-hit300");
		else if (GM->LastJudgement == 200) Name = TEXT("mania-hit200");
		else if (GM->LastJudgement == 100) Name = TEXT("mania-hit100");
		else if (GM->LastJudgement == 50) Name = TEXT("mania-hit50");
		if (UTexture2D* HitTex = GM->GetTexture(Name))
		{
			const FVector2D Size(250.0f * 1.45f * UI, 128.0f * 1.45f * UI);
			Tex(FVector2D(W * 0.5f - Size.X * 0.5f, H * 0.34f - Size.Y * 0.5f), Size, HitTex, FLinearColor(1.0f, 1.0f, 1.0f, Alpha));
			GM->bDrewJudgement = true;
		}
	}

	// key labels
	const TCHAR* Primary[4] = { TEXT("A"), TEXT("S"), TEXT(";"), TEXT("'") };
	const TCHAR* Secondary[4] = { TEXT("D"), TEXT("F"), TEXT("J"), TEXT("K") };
	for (int32 Lane = 0; Lane < 4; ++Lane)
	{
		const float CX = StageX + (Lane + 0.5f) * ColumnW;
		Rect(FVector2D(StageX + Lane * ColumnW + ColumnW * 0.18f, H - 54.0f * UI),
			FVector2D(ColumnW * 0.64f, 38.0f * UI), FLinearColor(0.0f, 0.0f, 0.0f, 0.35f));
		Label(Primary[Lane], FVector2D(CX, H - 34.0f * UI), 1.5f, FLinearColor(1.0f, 1.0f, 1.0f, 0.98f), true, true);
		Label(Secondary[Lane], FVector2D(CX, H - 12.0f * UI), 0.62f, FLinearColor(0.65f, 0.70f, 0.82f, 0.85f), true, true);
	}
}

void ARhythmHUD::DrawPlayingHud()
{
	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;

	Rect(FVector2D(0, 0), FVector2D(W, 112.0f * UI), FLinearColor(0.0f, 0.0f, 0.0f, 0.35f));

	// health bar, skin scorebar
	const FVector2D HB(24.0f * UI, 20.0f * UI);
	const FVector2D HBSize(310.0f * UI, 22.0f * UI);
	if (UTexture2D* HBBg = GM->GetTexture(TEXT("scorebar-bg")))
	{
		Tex(HB, HBSize, HBBg, FLinearColor(0.45f, 0.45f, 0.5f, 0.95f));
	}
	else
	{
		Rect(HB, HBSize, FLinearColor(0.12f, 0.12f, 0.16f, 1.0f));
	}
	const float Hp = FMath::Clamp(GM->Health / 100.0f, 0.0f, 1.0f);
	if (UTexture2D* HBCol = GM->GetTexture(TEXT("scorebar-colour")))
	{
		Tex(HB, FVector2D(HBSize.X * Hp, HBSize.Y), HBCol, Hp < 0.25f ? FLinearColor(1.0f, 0.35f, 0.35f, 1.0f) : FLinearColor(0.55f, 0.95f, 1.0f, 1.0f));
	}
	else
	{
		Rect(HB, FVector2D(HBSize.X * Hp, HBSize.Y), Hp < 0.25f ? FLinearColor(1.0f, 0.35f, 0.35f, 1.0f) : Cyan);
	}
	Rect(HB + FVector2D(-2.0f, -2.0f) * UI, FVector2D(HBSize.X + 4.0f * UI, 1.0f * UI), FLinearColor(1.0f, 1.0f, 1.0f, 0.35f));
	GM->bDrewHealth = true;

	Label(GM->CurrentChart.Title, FVector2D(24.0f * UI, 52.0f * UI), 0.92f, White);
	Label(FString::Printf(TEXT("%s  //  [%s]"), *GM->CurrentChart.Artist, *GM->CurrentChart.Version),
		FVector2D(24.0f * UI, 76.0f * UI), 0.68f, Dim);

	const FString ScoreText = FormatNumber(GM->Score);
	LabelRight(ScoreText, FVector2D(W - 28.0f * UI, 16.0f * UI), 2.05f, White);
	GM->bDrewScore = true;

	const FString AccText = FString::Printf(TEXT("%.2f%%"), GM->Accuracy());
	LabelRight(AccText, FVector2D(W - 28.0f * UI, 62.0f * UI), 1.08f, Cyan);
	GM->bDrewAccuracy = true;

	const FString ComboText = FString::Printf(TEXT("%dx"), GM->Combo);
	Label(ComboText, FVector2D(W * 0.5f, H * 0.13f), 2.6f, White, true, true);
	Label(TEXT("COMBO"), FVector2D(W * 0.5f, H * 0.13f + 36.0f * UI), 0.66f, Dim, true, true);
	GM->bDrewCombo = true;

	if (GM->bAutoPlay)
	{
		LabelRight(TEXT("AUTO"), FVector2D(W - 28.0f * UI, 104.0f * UI), 0.9f, FLinearColor(0.4f, 0.9f, 1.0f, 1.0f));
	}

	// hit error meter under the song title
	if (GM->HitErrors.Num() > 0)
	{
		const float CX = W * 0.5f;
		const float EY = 18.0f * UI;
		Rect(FVector2D(CX - 62.0f * UI, EY), FVector2D(124.0f * UI, 2.0f * UI), FLinearColor(1.0f, 1.0f, 1.0f, 0.25f));
		Rect(FVector2D(CX - 1.0f * UI, EY - 5.0f * UI), FVector2D(2.0f * UI, 12.0f * UI), FLinearColor(1.0f, 1.0f, 1.0f, 0.6f));
		for (const ARhythmGameMode::FHitError& E : GM->HitErrors)
		{
			const float EX = CX + FMath::Clamp(static_cast<float>(E.ErrorMs) * 2.0f, -60.0f, 60.0f) * UI;
			const float AE = static_cast<float>(FMath::Abs(E.ErrorMs));
			FLinearColor C = FLinearColor(0.95f, 0.85f, 0.3f, 0.9f);
			if (AE <= 16.0f) C = FLinearColor(0.35f, 0.95f, 1.0f, 0.95f);
			else if (AE <= 45.0f) C = FLinearColor(0.45f, 0.95f, 0.55f, 0.9f);
			else if (AE > 100.0f) C = FLinearColor(1.0f, 0.4f, 0.4f, 0.9f);
			Rect(FVector2D(EX, EY - 6.0f * UI), FVector2D(2.0f * UI, 14.0f * UI), C);
		}
	}

	// song progress
	const float Progress = GM->ChartProgress();
	Rect(FVector2D(0, H - 6.0f * UI), FVector2D(W, 6.0f * UI), FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
	Rect(FVector2D(0, H - 6.0f * UI), FVector2D(W * Progress, 6.0f * UI), FLinearColor(0.45f, 0.85f, 1.0f, 0.95f));

	const FString TimeText = FString::Printf(TEXT("%s / %s"), *FormatTime(GM->SongTime), *FormatTime(GM->CurrentChart.LastTime));
	LabelRight(TimeText, FVector2D(W - 24.0f * UI, H - 34.0f * UI), 0.72f, Dim);
	Label(FString::Printf(TEXT("OD %.1f   %.1f BPM   x%d"), GM->CurrentChart.OD, GM->CurrentChart.BPM, GM->ScrollSpeed),
		FVector2D(24.0f * UI, H - 34.0f * UI), 0.7f, Dim);
}

void ARhythmHUD::DrawPause()
{
	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;
	Rect(FVector2D(0, 0), FVector2D(W, H), FLinearColor(0.0f, 0.0f, 0.0f, 0.62f));
	Rect(FVector2D(W * 0.5f - 230.0f * UI, H * 0.30f), FVector2D(460.0f * UI, 300.0f * UI), Panel);

	Label(TEXT("PAUSED"), FVector2D(W * 0.5f, H * 0.36f), 2.6f, White, true, true);
	const FVector2D BtnSize(300.0f * UI, 54.0f * UI);
	DrawButton(TEXT("Resume"), FVector2D(W * 0.5f - BtnSize.X * 0.5f, H * 0.45f), BtnSize, EAction::PauseResume, -1, true);
	DrawButton(TEXT("Retry"), FVector2D(W * 0.5f - BtnSize.X * 0.5f, H * 0.45f + 68.0f * UI), BtnSize, EAction::PauseRetry, -1, false);
	DrawButton(TEXT("Quit to menu"), FVector2D(W * 0.5f - BtnSize.X * 0.5f, H * 0.45f + 136.0f * UI), BtnSize, EAction::PauseQuit, -1, false);
	Label(TEXT("Esc resumes"), FVector2D(W * 0.5f, H * 0.45f + 206.0f * UI), 0.8f, Dim, true, true);
}

void ARhythmHUD::DrawResults()
{
	const float W = Canvas->SizeX;
	const float H = Canvas->SizeY;
	DrawBackground(FLinearColor(0.20f, 0.20f, 0.28f, 1.0f));
	Rect(FVector2D(0, 0), FVector2D(W, H), FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));

	const float PW = FMath::Min(W * 0.78f, 980.0f * UI);
	const float PH = FMath::Min(H * 0.74f, 540.0f * UI);
	const FVector2D P((W - PW) * 0.5f, (H - PH) * 0.5f);
	Rect(P, FVector2D(PW, PH), FLinearColor(0.045f, 0.045f, 0.075f, 0.95f));
	Rect(P, FVector2D(PW, 5.0f * UI), GM->bFailed ? FLinearColor(1.0f, 0.32f, 0.35f, 1.0f) : Cyan);

	const FString Grade = GM->GradeText();
	Label(Grade, P + FVector2D(50.0f * UI, 100.0f * UI), 6.0f, GM->bFailed ? FLinearColor(1.0f, 0.35f, 0.4f, 1.0f) : White, false, true);
	Label(GM->bFailed ? TEXT("FAILED") : TEXT("CLEARED"), P + FVector2D(52.0f * UI, 176.0f * UI), 1.0f,
		GM->bFailed ? FLinearColor(1.0f, 0.4f, 0.45f, 1.0f) : FLinearColor(0.5f, 0.95f, 1.0f, 1.0f));
	Label(GM->CurrentChart.Title, P + FVector2D(50.0f * UI, 42.0f * UI), 1.3f, White);
	Label(FString::Printf(TEXT("%s  //  [%s]"), *GM->CurrentChart.Artist, *GM->CurrentChart.Version),
		P + FVector2D(50.0f * UI, 210.0f * UI), 0.72f, Dim);

	const float RX = P.X + PW * 0.46f;
	float RY = P.Y + 50.0f * UI;
	LabelRight(FormatNumber(GM->Score), FVector2D(RX + PW * 0.48f, RY), 1.9f, White);
	RY += 50.0f * UI;
	Label(FString::Printf(TEXT("%.2f%%"), GM->Accuracy()), FVector2D(RX, RY), 1.15f, Cyan);
	RY += 36.0f * UI;
	Label(FString::Printf(TEXT("Max combo   %dx"), GM->MaxCombo), FVector2D(RX, RY), 0.9f, Dim);
	RY += 28.0f * UI;
	Label(FString::Printf(TEXT("300g %d    300 %d    200 %d"), GM->Count320, GM->Count300, GM->Count200), FVector2D(RX, RY), 0.78f, Dim);
	RY += 24.0f * UI;
	Label(FString::Printf(TEXT("100  %d    50  %d    miss %d"), GM->Count100, GM->Count50, GM->CountMiss), FVector2D(RX, RY), 0.78f, Dim);
	RY += 36.0f * UI;
	Label(FString::Printf(TEXT("OD %.1f    4K    %s"), GM->CurrentChart.OD, *FormatTime(GM->CurrentChart.LastTime)), FVector2D(RX, RY), 0.72f, Dim);

	const FVector2D BtnSize(240.0f * UI, 52.0f * UI);
	const float BY = P.Y + PH - 78.0f * UI;
	DrawButton(TEXT("Retry"), FVector2D(P.X + 50.0f * UI, BY), BtnSize, EAction::ResultRetry, -1, true);
	DrawButton(TEXT("Song select"), FVector2D(P.X + 50.0f * UI + 258.0f * UI, BY), BtnSize, EAction::ResultSongs, -1, false);
	DrawButton(TEXT("Menu"), FVector2D(P.X + 50.0f * UI + 516.0f * UI, BY), BtnSize, EAction::ResultMenu, -1, false);
}

FString ARhythmHUD::FormatNumber(int64 Value) const
{
	FString Raw = FString::Printf(TEXT("%lld"), Value);
	FString Out;
	int32 Count = 0;
	for (int32 i = Raw.Len() - 1; i >= 0; --i)
	{
		Out.InsertAt(0, Raw[i]);
		++Count;
		if (Count % 3 == 0 && i > 0)
		{
			Out.InsertAt(0, TCHAR(','));
		}
	}
	return Out;
}

FString ARhythmHUD::FormatTime(double Ms) const
{
	const int32 Total = FMath::Max(0, FMath::FloorToInt(Ms / 1000.0));
	const int32 Minutes = Total / 60;
	const int32 Seconds = Total % 60;
	return FString::Printf(TEXT("%d:%02d"), Minutes, Seconds);
}
