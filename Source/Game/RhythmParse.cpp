#include "RhythmTypes.h"

#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FString Trimmed(const FString& In)
	{
		return In.TrimStartAndEnd();
	}

	void ParseKeyValue(const FString& Line, FString& Key, FString& Value)
	{
		int32 Colon = INDEX_NONE;
		if (Line.FindChar(TEXT(':'), Colon))
		{
			Key = Trimmed(Line.Left(Colon));
			Value = Trimmed(Line.Mid(Colon + 1));
		}
		else
		{
			Key = Trimmed(Line);
			Value.Empty();
		}
	}
}

bool Rhythm::LoadChartFile(const FString& File, FRhythmChart& Out)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *File))
	{
		return false;
	}

	Out = FRhythmChart();
	Out.Dir = FPaths::GetPath(File);

	FString Section;
	TArray<FString> HitLines;
	int32 Mode = 0;
	double FirstBeatLength = 0.0;
	FString BackgroundName;

	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, false);
	for (const FString& RawLine : Lines)
	{
		const FString Line = Trimmed(RawLine);
		if (Line.IsEmpty() || Line.StartsWith(TEXT("//")))
		{
			continue;
		}
		if (Line.StartsWith(TEXT("[")) && Line.EndsWith(TEXT("]")))
		{
			Section = Line.Mid(1, Line.Len() - 2);
			continue;
		}

		if (Section == TEXT("General"))
		{
			FString Key, Value;
			ParseKeyValue(Line, Key, Value);
			if (Key == TEXT("Mode"))
			{
				Mode = FCString::Atoi(*Value);
			}
		}
		else if (Section == TEXT("Metadata"))
		{
			FString Key, Value;
			ParseKeyValue(Line, Key, Value);
			if (Key == TEXT("Title") && !Value.IsEmpty())
			{
				Out.Title = Value;
			}
			else if (Key == TEXT("TitleUnicode") && Out.Title == TEXT("untitled") && !Value.IsEmpty())
			{
				Out.Title = Value;
			}
			else if (Key == TEXT("Artist") && !Value.IsEmpty())
			{
				Out.Artist = Value;
			}
			else if (Key == TEXT("Creator"))
			{
				Out.Creator = Value;
			}
			else if (Key == TEXT("Version"))
			{
				Out.Version = Value;
			}
		}
		else if (Section == TEXT("Difficulty"))
		{
			FString Key, Value;
			ParseKeyValue(Line, Key, Value);
			if (Key == TEXT("CircleSize")) Out.Keys = FCString::Atoi(*Value);
			else if (Key == TEXT("OverallDifficulty")) Out.OD = FCString::Atof(*Value);
			else if (Key == TEXT("HPDrainRate")) Out.HP = FCString::Atof(*Value);
		}
		else if (Section == TEXT("Events"))
		{
			if (BackgroundName.IsEmpty() && Line.StartsWith(TEXT("0,0,")))
			{
				const int32 FirstQuote = Line.Find(TEXT("\""));
				const int32 SecondQuote = Line.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				if (FirstQuote != INDEX_NONE && SecondQuote > FirstQuote)
				{
					BackgroundName = Line.Mid(FirstQuote + 1, SecondQuote - FirstQuote - 1);
				}
			}
		}
		else if (Section == TEXT("TimingPoints"))
		{
			TArray<FString> F;
			Line.ParseIntoArray(F, TEXT(","), false);
			if (F.Num() >= 2 && FirstBeatLength <= 0.0)
			{
				const double Beat = FCString::Atof(*F[1]);
				if (Beat > 1.0)
				{
					FirstBeatLength = Beat;
				}
			}
		}
		else if (Section == TEXT("HitObjects"))
		{
			HitLines.Add(Line);
		}
	}

	if (Mode != 3 || Out.Keys != 4)
	{
		return false;
	}

	Out.BPM = FirstBeatLength > 1.0 ? FMath::RoundToDouble(60000.0 / FirstBeatLength * 10.0) / 10.0 : 180.0;

	if (!BackgroundName.IsEmpty())
	{
		const FString Full = FPaths::Combine(Out.Dir, BackgroundName);
		if (IFileManager::Get().FileExists(*Full))
		{
			Out.BackgroundPath = Full;
		}
	}
	if (Out.BackgroundPath.IsEmpty())
	{
		Out.BackgroundPath = Rhythm::FindBackground(Out.Dir);
	}

	for (const FString& Line : HitLines)
	{
		TArray<FString> F;
		Line.ParseIntoArray(F, TEXT(","), false);
		if (F.Num() < 4)
		{
			continue;
		}

		FManiaNote Note;
		const float X = FCString::Atof(*F[0]);
		Note.Lane = FMath::Clamp(FMath::FloorToInt(X * Out.Keys / 512.0f), 0, Out.Keys - 1);
		Note.Time = FCString::Atof(*F[2]);
		const int32 Type = FCString::Atoi(*F[3]);
		Note.HitSound = (F.Num() > 4) ? FCString::Atoi(*F[4]) : 0;
		Note.EndTime = Note.Time;
		Note.bHold = (Type & 128) != 0;

		if (Note.bHold && F.Num() > 5)
		{
			FString EndToken = F[5];
			int32 Colon = INDEX_NONE;
			if (EndToken.FindChar(TEXT(':'), Colon))
			{
				EndToken = EndToken.Left(Colon);
			}
			Note.EndTime = FMath::Max(Note.Time + 1.0, FCString::Atof(*EndToken));
			Out.HoldCount++;
		}

		Out.Notes.Add(Note);
	}

	if (Out.Notes.Num() == 0)
	{
		return false;
	}

	Out.Notes.Sort([](const FManiaNote& A, const FManiaNote& B) { return A.Time < B.Time; });
	Out.FirstTime = Out.Notes[0].Time;
	Out.LastTime = Out.Notes[0].EndTime;
	for (const FManiaNote& N : Out.Notes)
	{
		Out.LastTime = FMath::Max(Out.LastTime, FMath::Max(N.Time, N.EndTime));
	}
	return true;
}

FString Rhythm::FindBackground(const FString& Dir)
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Dir / TEXT("bg.*")), true, false);
	if (Files.Num() > 0)
	{
		Files.Sort();
		return Dir / Files[0];
	}
	return FString();
}

int32 Rhythm::ScanSongs(const FString& SongsDir, TArray<FRhythmChart>& Out)
{
	Out.Reset();
	TArray<FString> Dirs;
	IFileManager::Get().FindFiles(Dirs, *(SongsDir / TEXT("*")), false, true);
	Dirs.Sort();
	for (const FString& Name : Dirs)
	{
		const FString Dir = SongsDir / Name;
		const FString ChartFile = Dir / TEXT("chart.osu");
		if (!IFileManager::Get().FileExists(*ChartFile))
		{
			continue;
		}
		FRhythmChart Chart;
		if (Rhythm::LoadChartFile(ChartFile, Chart))
		{
			Chart.Dir = Dir;
			Out.Add(Chart);
		}
	}
	return Out.Num();
}

UTexture2D* Rhythm::LoadTextureFile(const FString& File)
{
	TArray<uint8> Data;
	if (!FFileHelper::LoadFileToArray(Data, *File) || Data.Num() == 0)
	{
		return nullptr;
	}
	return FImageUtils::ImportBufferAsTexture2D(Data);
}

bool Rhythm::LoadBytes(const FString& File, TArray<uint8>& Out)
{
	return FFileHelper::LoadFileToArray(Out, *File);
}
