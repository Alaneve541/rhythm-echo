#include "Modules/ModuleManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

/**
 * The engine treats the first bare argument as the map to load, so a test harness
 * calling "Game.exe --screenshot shot.png" would try to load shot.png as a map.
 * Rewrite the space-separated form to the switch form before the engine parses it.
 * Only the standalone game is rewritten: in the editor this would change the
 * editor's own command line and is unnecessary (and unsafe).
 */
class FRhythmGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		if (!FApp::IsGame())
		{
			return;
		}
		const FString CommandLine = FCommandLine::Get();
		TArray<FString> Tokens;
		const TCHAR* Stream = *CommandLine;
		FString Token;
		while (FParse::Token(Stream, Token, false))
		{
			Tokens.Add(Token);
		}

		TArray<FString> Kept;
		Kept.Reserve(Tokens.Num());
		bool bChanged = false;
		for (int32 i = 0; i < Tokens.Num(); ++i)
		{
			const FString& T = Tokens[i];
			const bool bNamed = T == TEXT("--screenshot") || T == TEXT("-screenshot")
				|| T == TEXT("--self-test") || T == TEXT("-self-test");
			if (bNamed && Tokens.IsValidIndex(i + 1) && !Tokens[i + 1].StartsWith(TEXT("-")))
			{
				Kept.Add(T + TEXT("=") + Tokens[i + 1]);
				++i;
				bChanged = true;
			}
			else
			{
				Kept.Add(T);
			}
		}

		if (bChanged)
		{
			const FString NewCommandLine = FString::Join(Kept, TEXT(" "));
			FCommandLine::Set(*NewCommandLine);
		}
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FRhythmGameModule, Game, "Game");
