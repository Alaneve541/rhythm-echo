#include "RhythmGameMode.h"

#include "RhythmHUD.h"
#include "RhythmPlayerController.h"
#include "RhythmTypes.h"

#include "Components/AudioComponent.h"
#include "Engine/GameEngine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Sound/SoundWaveProcedural.h"
#include "UnrealClient.h"

namespace
{
	constexpr int32 SampleRate = 44100;
	constexpr int32 Channels = 2;
	constexpr int32 BytesPerFrame = Channels * 2;
	constexpr int32 BytesPerSecond = SampleRate * BytesPerFrame;

	FString ArgValue(const TCHAR* Name)
	{
		TArray<FString> Tokens;
		const TCHAR* Stream = FCommandLine::Get();
		FString Token;
		while (FParse::Token(Stream, Token, false))
		{
			Tokens.Add(Token);
		}
		const FString NameStr(Name);
		for (int32 i = 0; i < Tokens.Num(); ++i)
		{
			const FString& T = Tokens[i];
			if (T == NameStr || T == NameStr.Replace(TEXT("--"), TEXT("-")))
			{
				if (Tokens.IsValidIndex(i + 1))
				{
					return Tokens[i + 1];
				}
			}
			const FString WithEq = NameStr + TEXT("=");
			if (T.StartsWith(WithEq))
			{
				return T.Mid(WithEq.Len());
			}
			const FString ShortWithEq = NameStr.Replace(TEXT("--"), TEXT("-")) + TEXT("=");
			if (T.StartsWith(ShortWithEq))
			{
				return T.Mid(ShortWithEq.Len());
			}
		}
		return FString();
	}
}

ARhythmGameMode::ARhythmGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	DefaultPawnClass = nullptr;
	PlayerControllerClass = ARhythmPlayerController::StaticClass();
	HUDClass = ARhythmHUD::StaticClass();
}

void ARhythmGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Deliberately no GameUserSettings changes here: resizing the window from
	// BeginPlay also resizes the editor's Play-In-Editor window and was the
	// trigger for a Slate/LevelEditor shutdown crash. The window size is set by
	// Config/DefaultGameUserSettings.ini and can be overridden with -ResX/-ResY.

	LoadSongs();
	ParseLaunchOptions();

	if (Songs.Num() == 0)
	{
		Screen = ERhythmScreen::Results;
		LaunchError = FString::Printf(TEXT("No 4K charts found in %s"),
			*(FPaths::ProjectContentDir() / TEXT("Rhythm") / TEXT("Songs")));
		return;
	}

	if (bSelfTestMode)
	{
		const double Start = FMath::Max(0.0, Songs[0].FirstTime - 1300.0);
		StartSongByIndex(0, false, Start);
	}
	else if (bScreenshotMode)
	{
		const double Start = FMath::Max(0.0, Songs[0].FirstTime - 600.0);
		StartSongByIndex(0, true, Start);
	}
	else
	{
		Screen = ERhythmScreen::Menu;
		BackgroundTex = GetSongBackgroundTexture(SelectedSong);
	}
}

void ARhythmGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAudio();
	TextureCache.Reset();
	BackgroundCache.Reset();
	BackgroundTex = nullptr;
	Super::EndPlay(EndPlayReason);
}

bool ARhythmGameMode::ParseLaunchOptions()
{
	LaunchScreenshotPath = ArgValue(TEXT("--screenshot"));
	LaunchSelfTestPath = ArgValue(TEXT("--self-test"));
	bScreenshotMode = !LaunchScreenshotPath.IsEmpty();
	bSelfTestMode = !LaunchSelfTestPath.IsEmpty();
	return bScreenshotMode || bSelfTestMode;
}

void ARhythmGameMode::LoadSongs()
{
	const FString Dir = FPaths::ProjectContentDir() / TEXT("Rhythm") / TEXT("Songs");
	Rhythm::ScanSongs(Dir, Songs);

	auto Rank = [](const FRhythmChart& C)
	{
		const FString Name = FPaths::GetCleanFilename(C.Dir);
		if (Name == TEXT("azalea")) return 0;
		if (Name == TEXT("psycho_heroes")) return 1;
		if (Name == TEXT("snow_storm")) return 2;
		if (Name == TEXT("bangin_burst")) return 3;
		if (Name == TEXT("do_it")) return 4;
		if (Name == TEXT("inner_spirit")) return 5;
		return 9;
	};
	Songs.Sort([&Rank](const FRhythmChart& A, const FRhythmChart& B)
	{
		const int32 RA = Rank(A);
		const int32 RB = Rank(B);
		if (RA != RB)
		{
			return RA < RB;
		}
		return A.Title < B.Title;
	});

	if (Songs.Num() == 0)
	{
		LaunchError = FString::Printf(TEXT("No charts found in %s"), *Dir);
	}
}

UTexture2D* ARhythmGameMode::GetTexture(const FString& Name)
{
	if (TObjectPtr<UTexture2D>* Found = TextureCache.Find(Name))
	{
		return *Found;
	}

	FString Base = Name;
	if (Base.EndsWith(TEXT(".png")))
	{
		Base = Base.LeftChop(4);
	}
	const FString SkinDir = FPaths::ProjectContentDir() / TEXT("Rhythm") / TEXT("Skin");
	const FString HiRes = SkinDir / (Base + TEXT("@2x.png"));
	const FString Normal = SkinDir / (Base + TEXT(".png"));
	const FString Full = IFileManager::Get().FileExists(*HiRes) ? HiRes : Normal;
	UTexture2D* Tex = Rhythm::LoadTextureFile(Full);
	TextureCache.Add(Name, Tex);
	return Tex;
}

UTexture2D* ARhythmGameMode::GetBackgroundTexture()
{
	return BackgroundTex;
}

UTexture2D* ARhythmGameMode::GetSongBackgroundTexture(int32 Index)
{
	if (!Songs.IsValidIndex(Index) || Songs[Index].BackgroundPath.IsEmpty())
	{
		return nullptr;
	}
	const FString Path = Songs[Index].BackgroundPath;
	if (TObjectPtr<UTexture2D>* Found = BackgroundCache.Find(Path))
	{
		return *Found;
	}
	UTexture2D* Tex = Rhythm::LoadTextureFile(Path);
	BackgroundCache.Add(Path, Tex);
	return Tex;
}

FString ARhythmGameMode::GetSongBackgroundPath(int32 Index) const
{
	return Songs.IsValidIndex(Index) ? Songs[Index].BackgroundPath : FString();
}

void ARhythmGameMode::UpdateViewport()
{
	float W = 1280.0f;
	float H = 720.0f;
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* VP = World->GetGameViewport())
		{
			FVector2D Size = FVector2D::ZeroVector;
			VP->GetViewportSize(Size);
			if (Size.X > 32.0 && Size.Y > 32.0)
			{
				W = static_cast<float>(Size.X);
				H = static_cast<float>(Size.Y);
			}
		}
	}

	ViewW = W;
	ViewH = H;
	UI = H / 720.0f;
	ColumnW = H * (64.0f / 480.0f);
	StageW = ColumnW * 4.0f;
	StageX = (W - StageW) * 0.5f;
	HitY = H * 0.84f;
	NoteH = ColumnW;

	const double ScrollMs = FMath::Clamp(1440.0 - 30.0 * ScrollSpeed, 240.0, 1400.0);
	PxPerMs = H / ScrollMs;
}

FVector2D ARhythmGameMode::LaneCenter(int32 Lane) const
{
	return FVector2D(StageX + (Lane + 0.5f) * ColumnW, HitY);
}

float ARhythmGameMode::NoteY(double TimeMs) const
{
	return HitY - static_cast<float>(TimeMs - SongTime) * PxPerMs;
}

double ARhythmGameMode::WinPerfect() const { return 16.0; }
double ARhythmGameMode::Win300() const { return FMath::Max(1.0, FMath::TruncToDouble(64.0 - 3.0 * CurrentChart.OD)); }
double ARhythmGameMode::Win200() const { return FMath::Max(2.0, FMath::TruncToDouble(97.0 - 3.0 * CurrentChart.OD)); }
double ARhythmGameMode::Win100() const { return FMath::Max(3.0, FMath::TruncToDouble(127.0 - 3.0 * CurrentChart.OD)); }
double ARhythmGameMode::Win50() const { return FMath::Max(4.0, FMath::TruncToDouble(151.0 - 3.0 * CurrentChart.OD)); }

int32 ARhythmGameMode::JudgeValue(double ErrorMs) const
{
	const double Abs = FMath::Abs(ErrorMs);
	if (Abs <= WinPerfect()) return 320;
	if (Abs <= Win300()) return 300;
	if (Abs <= Win200()) return 200;
	if (Abs <= Win100()) return 100;
	// A late MEH is impossible in osu!mania; only early presses can be MEH.
	if (ErrorMs < 0.0 && Abs <= Win50()) return 50;
	return 0;
}

double ARhythmGameMode::Accuracy() const
{
	const int32 Total = TotalJudged();
	if (Total == 0)
	{
		return 100.0;
	}
	const double Weighted = 300.0 * (Count320 + Count300) + 200.0 * Count200 + 100.0 * Count100 + 50.0 * Count50;
	return 100.0 * Weighted / (300.0 * Total);
}

FString ARhythmGameMode::GradeText() const
{
	if (bFailed)
	{
		return TEXT("F");
	}
	const double Acc = Accuracy();
	if (Acc >= 100.0) return TEXT("SS");
	if (Acc > 95.0) return TEXT("S");
	if (Acc > 90.0) return TEXT("A");
	if (Acc > 80.0) return TEXT("B");
	if (Acc > 70.0) return TEXT("C");
	return TEXT("D");
}

float ARhythmGameMode::ChartProgress() const
{
	if (CurrentChart.LastTime <= 1.0)
	{
		return 0.0f;
	}
	return FMath::Clamp(static_cast<float>(SongTime / CurrentChart.LastTime), 0.0f, 1.0f);
}

int32 ARhythmGameMode::TotalJudged() const
{
	return Count320 + Count300 + Count200 + Count100 + Count50 + CountMiss;
}

void ARhythmGameMode::OpenSongSelect()
{
	StopAudio();
	Screen = ERhythmScreen::SongSelect;
	SelectedSong = FMath::Clamp(SelectedSong, 0, FMath::Max(0, Songs.Num() - 1));
	BackgroundTex = GetSongBackgroundTexture(SelectedSong);
}

void ARhythmGameMode::OpenMenu()
{
	StopAudio();
	Screen = ERhythmScreen::Menu;
	BackgroundTex = GetSongBackgroundTexture(SelectedSong);
}

void ARhythmGameMode::SelectSong(int32 Index)
{
	if (Songs.Num() == 0)
	{
		return;
	}
	SelectedSong = (Index % Songs.Num() + Songs.Num()) % Songs.Num();
	BackgroundTex = GetSongBackgroundTexture(SelectedSong);
}

void ARhythmGameMode::StartSelectedSong()
{
	StartSongByIndex(SelectedSong, false, FMath::Max(0.0, Songs.IsValidIndex(SelectedSong) ? Songs[SelectedSong].FirstTime - 2500.0 : 0.0));
}

void ARhythmGameMode::StartSongByIndex(int32 Index, bool bAuto, double StartAtMs)
{
	if (!Songs.IsValidIndex(Index))
	{
		return;
	}
	SelectedSong = Index;
	CurrentChart = Songs[Index];
	bChartReady = true;
	Screen = ERhythmScreen::Playing;
	bFinished = false;
	bFailed = false;
	bAutoPlay = bAuto;
	Count320 = Count300 = Count200 = Count100 = Count50 = CountMiss = 0;
	Combo = MaxCombo = 0;
	Score = 0;
	ScoreExact = 0.0;
	ScoreBonus = 100.0;
	Health = 100.0f;
	HudFrames = 0;
	bDrewScore = bDrewAccuracy = bDrewCombo = bDrewHealth = bDrewJudgement = false;
	bScreenshotRequested = false;
	FramesAfterScreenshot = 0;
	bExitPending = false;
	bSelfTestDone = false;
	SelfTestPresses = 0;
	SelfTestApproached = 0;
	SelfTestScrollPx = 0.0;
	HitErrors.Reset();
	LastJudgement = 0;
	LastJudgementTime = -10000.0;
	DisplayFps = 60.0f;

	for (int32 i = 0; i < 4; ++i)
	{
		bLaneDown[i] = false;
		LaneDownTime[i] = 0.0;
		LaneFlashTime[i] = -1000.0;
		BotReleaseAt[i] = -1.0;
	}

	SongTime = FMath::Max(0.0, StartAtMs);
	SelfTestStartTime = SongTime;

	BackgroundTex = nullptr;
	if (!CurrentChart.BackgroundPath.IsEmpty())
	{
		BackgroundTex = Rhythm::LoadTextureFile(CurrentChart.BackgroundPath);
	}

	StartAudio(SongTime);
}

void ARhythmGameMode::RestartSong()
{
	if (Songs.IsValidIndex(SelectedSong))
	{
		StartSongByIndex(SelectedSong, bAutoPlay, FMath::Max(0.0, Songs[SelectedSong].FirstTime - 2500.0));
	}
}

void ARhythmGameMode::Resume()
{
	if (Screen == ERhythmScreen::Paused)
	{
		Screen = ERhythmScreen::Playing;
		if (MusicComp)
		{
			MusicComp->SetPaused(false);
		}
	}
}

void ARhythmGameMode::QuitToMenu()
{
	StopAudio();
	Screen = ERhythmScreen::Menu;
	BackgroundTex = GetSongBackgroundTexture(SelectedSong);
}

void ARhythmGameMode::RequestQuitGame()
{
	ExitGame();
}

void ARhythmGameMode::MenuUp()
{
	if (Screen == ERhythmScreen::SongSelect)
	{
		SelectSong(SelectedSong - 1);
	}
	else
	{
		SelectedMenuButton = FMath::Max(0, SelectedMenuButton - 1);
	}
}

void ARhythmGameMode::MenuDown()
{
	if (Screen == ERhythmScreen::SongSelect)
	{
		SelectSong(SelectedSong + 1);
	}
	else
	{
		SelectedMenuButton = FMath::Min(1, SelectedMenuButton + 1);
	}
}

void ARhythmGameMode::Confirm()
{
	switch (Screen)
	{
	case ERhythmScreen::Menu:
		OpenSongSelect();
		break;
	case ERhythmScreen::SongSelect:
		StartSelectedSong();
		break;
	case ERhythmScreen::Paused:
		Resume();
		break;
	case ERhythmScreen::Results:
		OpenSongSelect();
		break;
	default:
		break;
	}
}

void ARhythmGameMode::Back()
{
	switch (Screen)
	{
	case ERhythmScreen::Menu:
		break;
	case ERhythmScreen::SongSelect:
		OpenMenu();
		break;
	case ERhythmScreen::Playing:
		Screen = ERhythmScreen::Paused;
		if (MusicComp)
		{
			MusicComp->SetPaused(true);
		}
		break;
	case ERhythmScreen::Paused:
		Resume();
		break;
	case ERhythmScreen::Results:
		OpenSongSelect();
		break;
	}
}

void ARhythmGameMode::StartAudio(double FromMs)
{
	StopAudio();
	MusicPcm.Reset();
	MusicByteOffset = 0;
	MusicQueuedBytes = 0;
	bMusicStarted = false;

	const FString File = CurrentChart.Dir / TEXT("audio.pcm");
	if (!Rhythm::LoadBytes(File, MusicPcm) || MusicPcm.Num() < 4)
	{
		return;
	}

	int64 Offset = static_cast<int64>((FromMs / 1000.0) * BytesPerSecond);
	Offset -= Offset % BytesPerFrame;
	MusicByteOffset = FMath::Clamp<int64>(Offset, 0, MusicPcm.Num() - 4);

	MusicWave = NewObject<USoundWaveProcedural>(this);
	MusicWave->SetSampleRate(SampleRate);
	MusicWave->NumChannels = Channels;
	MusicWave->Duration = static_cast<float>((MusicPcm.Num() - MusicByteOffset) / static_cast<double>(BytesPerSecond));
	MusicWave->SoundGroup = SOUNDGROUP_Default;
	MusicWave->bLooping = false;

	MusicComp = UGameplayStatics::SpawnSound2D(this, MusicWave, 0.72f, 1.0f, 0.0f, nullptr, false, false);
	if (MusicComp)
	{
		MusicComp->Play();
	}

	const int64 Chunk = FMath::Min<int64>(BytesPerSecond, MusicPcm.Num() - MusicByteOffset);
	if (Chunk > 0 && MusicWave)
	{
		MusicWave->QueueAudio(MusicPcm.GetData() + MusicByteOffset, static_cast<int32>(Chunk));
		MusicByteOffset += Chunk;
		MusicQueuedBytes += Chunk;
	}
	bMusicStarted = true;
}

void ARhythmGameMode::StopAudio()
{
	if (MusicComp)
	{
		MusicComp->Stop();
		MusicComp = nullptr;
	}
	MusicWave = nullptr;
	bMusicStarted = false;
	for (UAudioComponent* Comp : OneShotComps)
	{
		if (Comp)
		{
			Comp->Stop();
			Comp->DestroyComponent();
		}
	}
	OneShotComps.Reset();
	OneShotWaves.Reset();
	OneShotExpiry.Reset();
}

void ARhythmGameMode::PlaySample(const FString& FileName, float Volume)
{
	FString File = FileName;
	if (!File.EndsWith(TEXT(".wav")))
	{
		File += TEXT(".wav");
	}
	const FString Path = FPaths::ProjectContentDir() / TEXT("Rhythm") / TEXT("Skin") / File;
	TArray<uint8> Wav;
	if (!Rhythm::LoadBytes(Path, Wav) || Wav.Num() < 44)
	{
		return;
	}
	if (!(Wav[0] == 'R' && Wav[1] == 'I' && Wav[2] == 'F' && Wav[3] == 'F'))
	{
		return;
	}
	int32 WavChannels = 0;
	int32 WavSampleRate = 0;
	int32 Bits = 0;
	int32 DataOffset = -1;
	int32 DataSize = 0;
	for (int32 i = 12; i + 8 <= Wav.Num();)
	{
		const uint8* Id = &Wav[i];
		const uint32 Size = static_cast<uint32>(Wav[i + 4]) | (static_cast<uint32>(Wav[i + 5]) << 8)
			| (static_cast<uint32>(Wav[i + 6]) << 16) | (static_cast<uint32>(Wav[i + 7]) << 24);
		if (Id[0] == 'f' && Id[1] == 'm' && Id[2] == 't' && Id[3] == ' ')
		{
			if (i + 16 <= Wav.Num())
			{
				const int32 Format = Wav[i + 8] | (Wav[i + 9] << 8);
				if (Format != 1)
				{
					return;
				}
				WavChannels = Wav[i + 10] | (Wav[i + 11] << 8);
				WavSampleRate = Wav[i + 12] | (Wav[i + 13] << 8) | (Wav[i + 14] << 16) | (Wav[i + 15] << 24);
				Bits = Wav[i + 22] | (Wav[i + 23] << 8);
			}
		}
		else if (Id[0] == 'd' && Id[1] == 'a' && Id[2] == 't' && Id[3] == 'a')
		{
			DataOffset = i + 8;
			DataSize = static_cast<int32>(FMath::Min<uint32>(Size, static_cast<uint32>(Wav.Num() - DataOffset)));
			break;
		}
		i += 8 + static_cast<int32>(Size) + (static_cast<int32>(Size) & 1);
	}
	if (WavChannels < 1 || WavChannels > 2 || WavSampleRate < 8000 || DataOffset < 0 || DataSize <= 0 || (Bits != 8 && Bits != 16))
	{
		return;
	}

	TArray<uint8> Pcm;
	if (Bits == 8)
	{
		Pcm.SetNumUninitialized(DataSize * 2);
		for (int32 i = 0; i < DataSize; ++i)
		{
			const int16 Sample = static_cast<int16>((static_cast<int32>(Wav[DataOffset + i]) - 128) << 8);
			Pcm[i * 2] = static_cast<uint8>(Sample & 0xFF);
			Pcm[i * 2 + 1] = static_cast<uint8>((Sample >> 8) & 0xFF);
		}
	}
	else
	{
		Pcm.Append(Wav.GetData() + DataOffset, DataSize);
	}

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(WavSampleRate);
	Wave->NumChannels = WavChannels;
	Wave->Duration = Pcm.Num() / static_cast<float>(WavSampleRate * WavChannels * 2);
	Wave->SoundGroup = SOUNDGROUP_Effects;
	Wave->bLooping = false;
	Wave->QueueAudio(Pcm.GetData(), Pcm.Num());

	UAudioComponent* Comp = UGameplayStatics::SpawnSound2D(this, Wave, Volume, 1.0f, 0.0f, nullptr, false, false);
	if (!Comp)
	{
		return;
	}
	OneShotComps.Add(Comp);
	OneShotWaves.Add(Wave);
	OneShotExpiry.Add(FPlatformTime::Seconds() + 3.0);
}

void ARhythmGameMode::UpdateOneShots()
{
	const double Now = FPlatformTime::Seconds();
	for (int32 i = OneShotComps.Num() - 1; i >= 0; --i)
	{
		if (!OneShotComps.IsValidIndex(i) || !OneShotComps[i])
		{
			OneShotComps.RemoveAt(i);
			OneShotWaves.RemoveAt(i);
			OneShotExpiry.RemoveAt(i);
			continue;
		}
		if (OneShotExpiry.IsValidIndex(i) && Now < OneShotExpiry[i])
		{
			continue;
		}
		OneShotComps[i]->Stop();
		OneShotComps[i]->DestroyComponent();
		OneShotComps.RemoveAt(i);
		OneShotWaves.RemoveAt(i);
		OneShotExpiry.RemoveAt(i);
	}
}

void ARhythmGameMode::PlayHitSound(int32 HitSound)
{
	PlaySample(TEXT("normal-hitnormal"), 0.38f);
	if (HitSound & 8) PlaySample(TEXT("normal-hitclap"), 0.30f);
	if (HitSound & 4) PlaySample(TEXT("normal-hitfinish"), 0.30f);
	if (HitSound & 2) PlaySample(TEXT("normal-hitwhistle"), 0.25f);
}

void ARhythmGameMode::RecordHitError(double ErrorMs)
{
	FHitError E;
	E.ErrorMs = ErrorMs;
	E.Time = SongTime;
	HitErrors.Add(E);
	if (HitErrors.Num() > 20)
	{
		HitErrors.RemoveAt(0);
	}
}

void ARhythmGameMode::UpdateAudio()
{
	if (!MusicComp || !MusicWave || !bMusicStarted)
	{
		return;
	}
	int32 Buffered = MusicWave->GetAvailableAudioByteCount();
	const int32 Target = BytesPerSecond / 2;
	int64 Remaining = MusicPcm.Num() - MusicByteOffset;
	int32 Guard = 0;
	while (Buffered < Target && Remaining > 0 && Guard++ < 32)
	{
		int32 Chunk = static_cast<int32>(FMath::Min<int64>(Target - Buffered, Remaining));
		Chunk -= Chunk % BytesPerFrame;
		if (Chunk <= 0)
		{
			break;
		}
		MusicWave->QueueAudio(MusicPcm.GetData() + MusicByteOffset, Chunk);
		MusicByteOffset += Chunk;
		Remaining -= Chunk;
		Buffered += Chunk;
		MusicQueuedBytes += Chunk;
	}
}

void ARhythmGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateViewport();
	UpdateOneShots();

	if (DeltaSeconds > 0.0001f)
	{
		DisplayFps = FMath::Lerp(DisplayFps, 1.0f / DeltaSeconds, 0.08f);
	}

	if (Screen == ERhythmScreen::Playing && !bFinished)
	{
		SongTime += static_cast<double>(DeltaSeconds) * 1000.0;

		if (bAutoPlay || bSelfTestMode)
		{
			UpdateScriptedInput(DeltaSeconds);
		}

		UpdateNotes(DeltaSeconds);
		UpdateAudio();

		if (!bFailed && CurrentChart.HP > 0.0f)
		{
			Health -= DeltaSeconds * (0.10f + 0.08f * CurrentChart.HP);
			Health = FMath::Clamp(Health, 0.0f, 100.0f);
			if (Health <= 0.0f)
			{
				bFailed = true;
				FinishSong();
			}
		}

		if (!bFinished && SongTime > CurrentChart.LastTime + 1500.0)
		{
			FinishSong();
		}

		if (bSelfTestMode && !bSelfTestDone)
		{
			const int32 Hits = Count320 + Count300 + Count200 + Count100 + Count50;
			if (((Hits >= 6 && HudFrames > 20) || SongTime - SelfTestStartTime > 8000.0) && SongTime - SelfTestStartTime > 1000.0)
			{
				WriteSelfTest();
				bSelfTestDone = true;
				bExitPending = true;
			}
		}

		if (bScreenshotMode && bScreenshotRequested)
		{
			FramesAfterScreenshot++;
			const FString Path = FPaths::ConvertRelativePathToFull(LaunchScreenshotPath);
			if (IFileManager::Get().FileExists(*Path) && FramesAfterScreenshot > 3)
			{
				bExitPending = true;
			}
			else if (FramesAfterScreenshot > 240)
			{
				CaptureScreenshotNow();
				bExitPending = true;
			}
		}
	}

	if (bExitPending)
	{
		ExitGame();
	}
}

void ARhythmGameMode::OnHudDrawn()
{
	HudFrames++;
	if (bScreenshotMode && !bScreenshotRequested && HudFrames >= 120)
	{
		bScreenshotRequested = true;
		const FString Path = FPaths::ConvertRelativePathToFull(LaunchScreenshotPath);
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
		FScreenshotRequest::RequestScreenshot(Path, false, false);
	}
}

void ARhythmGameMode::PressLane(int32 Lane)
{
	if (Lane < 0 || Lane >= 4 || Screen != ERhythmScreen::Playing || bFinished)
	{
		return;
	}

	const bool bWasDown = bLaneDown[Lane];
	if (bWasDown)
	{
		return;
	}
	bLaneDown[Lane] = true;
	LaneDownTime[Lane] = SongTime;
	LaneFlashTime[Lane] = SongTime;
	if (bSelfTestMode && !bWasDown)
	{
		SelfTestPresses++;
	}

	// If a hold in this lane is still running, pressing again only re-grips it.
	for (FManiaNote& N : CurrentChart.Notes)
	{
		if (N.Lane == Lane && N.bHold && N.bHeadHit && !N.bJudged)
		{
			N.bHeld = true;
			return;
		}
	}

	FManiaNote* Best = nullptr;
	double BestAbs = 1e9;
	for (FManiaNote& N : CurrentChart.Notes)
	{
		if (N.Lane != Lane || N.bJudged || N.bHeadHit)
		{
			continue;
		}
		const double Delta = SongTime - N.Time;
		const double Abs = FMath::Abs(Delta);
		if (Abs > Win50())
		{
			continue;
		}
		if (Abs < BestAbs)
		{
			BestAbs = Abs;
			Best = &N;
		}
	}

	if (!Best)
	{
		return;
	}

	const int32 Value = JudgeValue(SongTime - Best->Time);
	if (Value <= 0)
	{
		return;
	}

	PlayHitSound(Best->HitSound);

	if (Best->bHold)
	{
		Best->bHeadHit = true;
		Best->bHeld = true;
		Best->bReleasedEarly = false;
		Best->HeadValue = Value;
		Best->HeadError = SongTime - Best->Time;
	}
	else
	{
		const double Error = SongTime - Best->Time;
		Best->bJudged = true;
		Best->FinalValue = Value;
		ApplyJudgement(Value, Error, Lane);
	}
}

void ARhythmGameMode::ReleaseLane(int32 Lane)
{
	if (Lane < 0 || Lane >= 4)
	{
		return;
	}
	bLaneDown[Lane] = false;

	if (Screen != ERhythmScreen::Playing || bFinished)
	{
		return;
	}

	for (FManiaNote& N : CurrentChart.Notes)
	{
		if (N.Lane != Lane || !N.bHold || !N.bHeadHit || N.bJudged || !N.bHeld)
		{
			continue;
		}
		const double TailError = SongTime - N.EndTime;
		if (SongTime >= N.EndTime - Win50())
		{
			N.bHeld = false;
			FinalizeHold(N, TailError);
		}
		else
		{
			N.bHeld = false;
			N.bReleasedEarly = true;
		}
		return;
	}
}

void ARhythmGameMode::ApplyJudgement(int32 Value, double ErrorMs, int32 Lane)
{
	const int32 PreviousCombo = Combo;

	if (Value > 0)
	{
		Combo++;
		MaxCombo = FMath::Max(MaxCombo, Combo);
		if (Value == 320) { Count320++; Health += 1.2f; }
		else if (Value == 300) { Count300++; Health += 1.0f; }
		else if (Value == 200) { Count200++; Health += 0.4f; }
		else if (Value == 100) { Count100++; Health += 0.1f; }
		else { Count50++; Health -= 0.5f; }
	}
	else
	{
		CountMiss++;
		Combo = 0;
		Health -= 5.0f;
		if (PreviousCombo >= 10)
		{
			PlaySample(TEXT("combobreak"), 0.55f);
		}
	}
	Health = FMath::Clamp(Health, 0.0f, 100.0f);
	RecordHitError(Value > 0 ? ErrorMs : 0.0);

	const int32 TotalNotes = FMath::Max(1, CurrentChart.Notes.Num());
	const double Unit = 500000.0 / static_cast<double>(TotalNotes);

	int32 BonusValue = 0;
	int32 BonusAdd = 0;
	int32 Punish = 0;
	switch (Value)
	{
	case 320: BonusValue = 32; BonusAdd = 2; break;
	case 300: BonusValue = 32; BonusAdd = 1; break;
	case 200: BonusValue = 16; Punish = 8; break;
	case 100: BonusValue = 8; Punish = 24; break;
	case 50: BonusValue = 4; Punish = 44; break;
	default: ScoreBonus = 0.0; break;
	}
	if (Value > 0)
	{
		ScoreBonus = FMath::Clamp(ScoreBonus + BonusAdd - Punish, 0.0, 100.0);
	}
	ScoreExact += Unit * (static_cast<double>(Value) / 320.0);
	ScoreExact += Unit * (BonusValue * FMath::Sqrt(ScoreBonus) / 320.0);
	Score = static_cast<int64>(FMath::RoundToDouble(ScoreExact));

	LastJudgement = Value;
	LastJudgementLane = Lane;
	LastJudgementTime = SongTime;
}

void ARhythmGameMode::FinalizeHold(FManiaNote& Note, double TailError)
{
	if (Note.bJudged)
	{
		return;
	}
	int32 Value = 0;
	if (Note.bReleasedEarly)
	{
		Value = 50;
	}
	else
	{
		const double HeadError = FMath::Abs(Note.HeadError);
		const double Combined = HeadError + FMath::Abs(TailError);
		if (HeadError <= WinPerfect() * 1.2 && Combined <= WinPerfect() * 2.4) Value = 320;
		else if (HeadError <= Win300() * 1.1 && Combined <= Win300() * 2.2) Value = 300;
		else if (HeadError <= Win200() && Combined <= Win200() * 2.0) Value = 200;
		else if (HeadError <= Win100() && Combined <= Win100() * 2.0) Value = 100;
		else Value = 50;
	}
	Note.bJudged = true;
	Note.FinalValue = Value;
	ApplyJudgement(Value, Note.HeadError, Note.Lane);
}

void ARhythmGameMode::UpdateNotes(double DeltaSeconds)
{
	for (FManiaNote& N : CurrentChart.Notes)
	{
		if (N.bJudged)
		{
			continue;
		}

		if (!N.bHeadHit)
		{
			// Late MEH is impossible: after the OK window the note is a miss.
			if (SongTime > N.Time + Win100())
			{
				N.bJudged = true;
				N.FinalValue = 0;
				ApplyJudgement(0, 0.0, N.Lane);
			}
			continue;
		}

		if (N.bHeld)
		{
			if (SongTime >= N.EndTime)
			{
				N.bHeld = false;
				FinalizeHold(N, SongTime - N.EndTime);
			}
		}
		else if (SongTime > N.EndTime - Win50())
		{
			// The key was not held from the tail's early MEH window onwards.
			N.bJudged = true;
			N.FinalValue = 0;
			ApplyJudgement(0, 0.0, N.Lane);
		}
	}
}

void ARhythmGameMode::UpdateScriptedInput(double DeltaSeconds)
{
	for (int32 Lane = 0; Lane < 4; ++Lane)
	{
		if (BotReleaseAt[Lane] > 0.0 && SongTime >= BotReleaseAt[Lane])
		{
			ReleaseLane(Lane);
			BotReleaseAt[Lane] = -1.0;
		}

		// Keep a hold gripped until its tail.
		FManiaNote* ActiveHold = nullptr;
		for (FManiaNote& N : CurrentChart.Notes)
		{
			if (N.Lane == Lane && N.bHold && N.bHeadHit && !N.bJudged)
			{
				ActiveHold = &N;
				break;
			}
		}
		if (ActiveHold)
		{
			if (!bLaneDown[Lane])
			{
				PressLane(Lane);
			}
			BotReleaseAt[Lane] = ActiveHold->EndTime + 12.0;
			continue;
		}

		// A tap key is still held down: let its release fire before the next note.
		if (bLaneDown[Lane])
		{
			continue;
		}

		FManiaNote* Next = nullptr;
		for (FManiaNote& N : CurrentChart.Notes)
		{
			if (N.Lane != Lane || N.bJudged || N.bHeadHit)
			{
				continue;
			}
			if (N.Time < SongTime - 30.0)
			{
				continue;
			}
			if (N.Time > SongTime + 60.0)
			{
				break;
			}
			if (!Next || N.Time < Next->Time)
			{
				Next = &N;
			}
		}

		if (Next && SongTime >= Next->Time - 6.0)
		{
			if (!bLaneDown[Lane])
			{
				PressLane(Lane);
			}
			BotReleaseAt[Lane] = Next->bHold ? (Next->EndTime + 12.0) : (Next->Time + 32.0);
		}
	}

	int32 ApproachedNow = 0;
	for (FManiaNote& N : CurrentChart.Notes)
	{
		if (N.Time <= SongTime && N.Time >= SelfTestStartTime)
		{
			++ApproachedNow;
		}
	}
	SelfTestApproached = FMath::Max(SelfTestApproached, ApproachedNow);
	SelfTestScrollPx = FMath::Max(SelfTestScrollPx, (SongTime - SelfTestStartTime) * PxPerMs);
}

void ARhythmGameMode::FinishSong()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	StopAudio();
	Screen = ERhythmScreen::Results;
}

void ARhythmGameMode::CaptureScreenshotNow()
{
	const FString Path = FPaths::ConvertRelativePathToFull(LaunchScreenshotPath);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);

	UGameViewportClient* VP = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!VP || !VP->Viewport)
	{
		return;
	}
	FViewport* View = VP->Viewport;
	const FIntPoint Size = View->GetSizeXY();
	if (Size.X <= 0 || Size.Y <= 0)
	{
		return;
	}
	TArray<FColor> Pixels;
	FReadSurfaceDataFlags Flags(RCM_UNorm);
	Flags.SetLinearToGamma(false);
	FlushRenderingCommands();
	View->ReadPixels(Pixels, Flags, FIntRect(0, 0, Size.X, Size.Y));
	if (Pixels.Num() < Size.X * Size.Y)
	{
		return;
	}
	TArray<FColor> Flipped;
	Flipped.SetNumUninitialized(Size.X * Size.Y);
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		FMemory::Memcpy(&Flipped[Y * Size.X], &Pixels[(Size.Y - 1 - Y) * Size.X], Size.X * sizeof(FColor));
	}
	TArray64<uint8> Png;
	FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Flipped, Png);
	TArray<uint8> Bytes;
	Bytes.Append(Png.GetData(), static_cast<int32>(Png.Num()));
	FFileHelper::SaveArrayToFile(Bytes, *Path);
}

void ARhythmGameMode::ExitGame()
{
	StopAudio();
	FPlatformMisc::RequestExit(false);
}

void ARhythmGameMode::WriteSelfTest()
{
	const FString Path = FPaths::ConvertRelativePathToFull(LaunchSelfTestPath);
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);

	const int32 Hits = Count320 + Count300 + Count200 + Count100 + Count50;
	const bool bMove = SelfTestScrollPx > 200.0 && SelfTestApproached >= 3 && SelfTestPresses >= 3;
	const bool bAct = Hits >= 3;
	const bool bHud = bDrewScore && bDrewAccuracy && bDrewCombo && bDrewHealth;

	const FString MoveDetail = FString::Printf(
		TEXT("playfield scrolled %.0f px, %d notes approached, %d key presses"),
		SelfTestScrollPx, SelfTestApproached, SelfTestPresses);
	const FString ActDetail = FString::Printf(
		TEXT("hit %d notes: %d perfect, %d great, %d good, %d ok, %d meh, %d miss, combo %d"),
		Hits, Count320, Count300, Count200, Count100, Count50, CountMiss, MaxCombo);
	const FString HudDetail = FString::Printf(
		TEXT("drawn: score=%s accuracy=%s combo=%s health=%s judgement=%s"),
		bDrewScore ? TEXT("yes") : TEXT("no"),
		bDrewAccuracy ? TEXT("yes") : TEXT("no"),
		bDrewCombo ? TEXT("yes") : TEXT("no"),
		bDrewHealth ? TEXT("yes") : TEXT("no"),
		bDrewJudgement ? TEXT("yes") : TEXT("no"));

	FString Json = TEXT("{\n  \"version\": 1,\n  \"steps\": [\n");
	Json += FString::Printf(TEXT("    {\"step\": \"move\", \"passed\": %s, \"detail\": \"%s\"},\n"), bMove ? TEXT("true") : TEXT("false"), *MoveDetail);
	Json += FString::Printf(TEXT("    {\"step\": \"act\", \"passed\": %s, \"detail\": \"%s\"},\n"), bAct ? TEXT("true") : TEXT("false"), *ActDetail);
	Json += FString::Printf(TEXT("    {\"step\": \"hud\", \"passed\": %s, \"detail\": \"%s\"}\n"), bHud ? TEXT("true") : TEXT("false"), *HudDetail);
	Json += TEXT("  ]\n}\n");

	if (!FFileHelper::SaveStringToFile(Json, *Path))
	{
		UE_LOG(LogTemp, Error, TEXT("Could not write self test to %s"), *Path);
	}
}
