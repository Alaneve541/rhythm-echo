#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** One osu!mania note. A hold note is judged once, at its tail. */
struct FManiaNote
{
	int32 Lane = 0;
	double Time = 0.0;
	double EndTime = 0.0;
	bool bHold = false;
	int32 HitSound = 0;

	// runtime state
	bool bJudged = false;
	bool bHeadHit = false;
	bool bHeld = false;
	bool bReleasedEarly = false;
	int32 HeadValue = 0;
	double HeadError = 0.0;
	int32 FinalValue = 0;
};

struct FRhythmChart
{
	FString Dir;
	FString Title = TEXT("untitled");
	FString Artist = TEXT("unknown");
	FString Creator;
	FString Version;
	FString BackgroundPath;

	float OD = 6.5f;
	float HP = 7.0f;
	int32 Keys = 4;
	double BPM = 180.0;

	double FirstTime = 0.0;
	double LastTime = 0.0;
	int32 HoldCount = 0;

	TArray<FManiaNote> Notes;

	bool IsValid() const { return Notes.Num() > 0; }
	int32 ObjectCount() const { return Notes.Num(); }
};

namespace Rhythm
{
	bool LoadChartFile(const FString& File, FRhythmChart& Out);
	int32 ScanSongs(const FString& SongsDir, TArray<FRhythmChart>& Out);
	UTexture2D* LoadTextureFile(const FString& File);
	bool LoadBytes(const FString& File, TArray<uint8>& Out);
	FString FindBackground(const FString& Dir);
}
