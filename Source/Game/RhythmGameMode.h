#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RhythmTypes.h"
#include "RhythmGameMode.generated.h"

class UTexture2D;
class USoundWaveProcedural;
class UAudioComponent;

UENUM()
enum class ERhythmScreen : uint8
{
	Menu,
	SongSelect,
	Playing,
	Paused,
	Results
};

UCLASS()
class GAME_API ARhythmGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ARhythmGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ---------------- screens ----------------
	ERhythmScreen Screen = ERhythmScreen::Menu;
	TArray<FRhythmChart> Songs;
	int32 SelectedSong = 0;
	int32 SelectedMenuButton = 0;

	// ---------------- play state (read by the HUD) ----------------
	FRhythmChart CurrentChart;
	bool bChartReady = false;

	bool bLaneDown[4] = { false, false, false, false };
	double LaneDownTime[4] = { 0.0, 0.0, 0.0, 0.0 };
	double LaneFlashTime[4] = { -1000.0, -1000.0, -1000.0, -1000.0 };
	bool bAutoPlay = false;
	int32 ScrollSpeed = 24;

	int32 Count320 = 0;
	int32 Count300 = 0;
	int32 Count200 = 0;
	int32 Count100 = 0;
	int32 Count50 = 0;
	int32 CountMiss = 0;
	int32 Combo = 0;
	int32 MaxCombo = 0;
	int64 Score = 0;
	double ScoreExact = 0.0;
	double ScoreBonus = 100.0;
	double SongTime = 0.0;
	float Health = 100.0f;
	bool bFailed = false;
	bool bFinished = false;

	int32 LastJudgement = 0;
	int32 LastJudgementLane = 0;
	double LastJudgementTime = -10000.0;

	double Accuracy() const;
	FString GradeText() const;
	float ChartProgress() const;
	int32 TotalJudged() const;
	double WinPerfect() const;
	double Win300() const;
	double Win200() const;
	double Win100() const;
	double Win50() const;
	int32 JudgeValue(double ErrorMs) const;

	// HUD trackers (used by --self-test)
	bool bDrewScore = false;
	bool bDrewAccuracy = false;
	bool bDrewCombo = false;
	bool bDrewHealth = false;
	bool bDrewJudgement = false;
	int32 HudFrames = 0;
	float DisplayFps = 60.0f;

	struct FHitError
	{
		double ErrorMs = 0.0;
		double Time = 0.0;
	};
	TArray<FHitError> HitErrors;

	// playfield geometry in screen pixels
	float ViewW = 1280.0f;
	float ViewH = 720.0f;
	float UI = 1.0f;
	float ColumnW = 96.0f;
	float StageW = 384.0f;
	float StageX = 448.0f;
	float HitY = 604.0f;
	float NoteH = 96.0f;
	float PxPerMs = 1.0f;

	void UpdateViewport();
	FVector2D LaneCenter(int32 Lane) const;
	float NoteY(double TimeMs) const;

	// ---------------- actions ----------------
	void PressLane(int32 Lane);
	void ReleaseLane(int32 Lane);
	void MenuUp();
	void MenuDown();
	void Confirm();
	void Back();
	void OpenSongSelect();
	void OpenMenu();
	void SelectSong(int32 Index);
	void StartSelectedSong();
	void StartSongByIndex(int32 Index, bool bAuto, double StartAtMs);
	void RestartSong();
	void Resume();
	void QuitToMenu();
	void RequestQuitGame();

	void OnHudDrawn();

	// ---------------- assets ----------------
	UTexture2D* GetTexture(const FString& Name);
	UTexture2D* GetBackgroundTexture();
	UTexture2D* GetSongBackgroundTexture(int32 Index);
	FString GetSongBackgroundPath(int32 Index) const;

	// mode flags for automation
	bool bScreenshotMode = false;
	bool bSelfTestMode = false;
	FString LaunchScreenshotPath;
	FString LaunchSelfTestPath;
	FString LaunchError;

private:
	bool ParseLaunchOptions();
	void LoadSongs();
	void ResetPlayState(double StartAtMs);
	void StartAudio(double FromMs);
	void StopAudio();
	void UpdateAudio();
	void ApplyJudgement(int32 Value, double ErrorMs, int32 Lane);
	void FinalizeHold(FManiaNote& Note, double TailError);
	void UpdateNotes(double DeltaSeconds);
	void UpdateScriptedInput(double DeltaSeconds);
	void UpdateOneShots();
	void FinishSong();
	void CaptureScreenshotNow();
	void WriteSelfTest();
	void ExitGame();
	void PlaySample(const FString& FileName, float Volume);
	void PlayHitSound(int32 HitSound);
	void RecordHitError(double ErrorMs);

	UPROPERTY()
	TMap<FString, TObjectPtr<UTexture2D>> TextureCache;

	UPROPERTY()
	TMap<FString, TObjectPtr<UTexture2D>> BackgroundCache;

	UPROPERTY()
	USoundWaveProcedural* MusicWave = nullptr;

	UPROPERTY()
	UAudioComponent* MusicComp = nullptr;

	UPROPERTY()
	UTexture2D* BackgroundTex = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UAudioComponent>> OneShotComps;

	UPROPERTY()
	TArray<TObjectPtr<USoundWaveProcedural>> OneShotWaves;

	TArray<double> OneShotExpiry;

	TArray<uint8> MusicPcm;
	int64 MusicByteOffset = 0;
	int64 MusicQueuedBytes = 0;
	double MusicQueuedMs = 0.0;
	bool bMusicStarted = false;

	// automation
	double SelfTestStartTime = 0.0;
	double SelfTestScrollPx = 0.0;
	int32 SelfTestPresses = 0;
	int32 SelfTestApproached = 0;
	bool bSelfTestDone = false;
	bool bExitPending = false;
	int32 FramesAfterScreenshot = 0;
	bool bScreenshotRequested = false;
	double BotReleaseAt[4] = { -1.0, -1.0, -1.0, -1.0 };
};
