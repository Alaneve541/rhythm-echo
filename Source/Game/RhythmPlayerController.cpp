#include "RhythmPlayerController.h"

#include "Kismet/GameplayStatics.h"
#include "RhythmGameMode.h"
#include "RhythmHUD.h"

ARhythmPlayerController::ARhythmPlayerController()
{
	bShowMouseCursor = true;
	PrimaryActorTick.bCanEverTick = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void ARhythmPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);
}

void ARhythmPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!InputComponent)
	{
		return;
	}

	// osu!.cfg: ManiaLayouts4K = A S OemSemicolon OemQuotes
	InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ARhythmPlayerController::OnLane0Down);
	InputComponent->BindKey(EKeys::A, IE_Released, this, &ARhythmPlayerController::OnLane0Up);
	InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ARhythmPlayerController::OnLane1Down);
	InputComponent->BindKey(EKeys::S, IE_Released, this, &ARhythmPlayerController::OnLane1Up);
	InputComponent->BindKey(EKeys::Semicolon, IE_Pressed, this, &ARhythmPlayerController::OnLane2Down);
	InputComponent->BindKey(EKeys::Semicolon, IE_Released, this, &ARhythmPlayerController::OnLane2Up);
	InputComponent->BindKey(EKeys::Quote, IE_Pressed, this, &ARhythmPlayerController::OnLane3Down);
	InputComponent->BindKey(EKeys::Quote, IE_Released, this, &ARhythmPlayerController::OnLane3Up);

	// secondary DFJK layout
	InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ARhythmPlayerController::OnLane0Down);
	InputComponent->BindKey(EKeys::D, IE_Released, this, &ARhythmPlayerController::OnLane0Up);
	InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ARhythmPlayerController::OnLane1Down);
	InputComponent->BindKey(EKeys::F, IE_Released, this, &ARhythmPlayerController::OnLane1Up);
	InputComponent->BindKey(EKeys::J, IE_Pressed, this, &ARhythmPlayerController::OnLane2Down);
	InputComponent->BindKey(EKeys::J, IE_Released, this, &ARhythmPlayerController::OnLane2Up);
	InputComponent->BindKey(EKeys::K, IE_Pressed, this, &ARhythmPlayerController::OnLane3Down);
	InputComponent->BindKey(EKeys::K, IE_Released, this, &ARhythmPlayerController::OnLane3Up);

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ARhythmPlayerController::OnClickPressed);
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ARhythmPlayerController::OnClickReleased);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ARhythmPlayerController::OnEscapePressed);
	InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ARhythmPlayerController::OnUpPressed);
	InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ARhythmPlayerController::OnDownPressed);
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ARhythmPlayerController::OnConfirmPressed);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ARhythmPlayerController::OnConfirmPressed);
}

ARhythmGameMode* ARhythmPlayerController::GetGM() const
{
	return Cast<ARhythmGameMode>(UGameplayStatics::GetGameMode(this));
}

ARhythmHUD* ARhythmPlayerController::GetRhythmHUD() const
{
	return Cast<ARhythmHUD>(GetHUD());
}

void ARhythmPlayerController::OnLane0Down() { if (ARhythmGameMode* GM = GetGM()) GM->PressLane(0); }
void ARhythmPlayerController::OnLane0Up() { if (ARhythmGameMode* GM = GetGM()) GM->ReleaseLane(0); }
void ARhythmPlayerController::OnLane1Down() { if (ARhythmGameMode* GM = GetGM()) GM->PressLane(1); }
void ARhythmPlayerController::OnLane1Up() { if (ARhythmGameMode* GM = GetGM()) GM->ReleaseLane(1); }
void ARhythmPlayerController::OnLane2Down() { if (ARhythmGameMode* GM = GetGM()) GM->PressLane(2); }
void ARhythmPlayerController::OnLane2Up() { if (ARhythmGameMode* GM = GetGM()) GM->ReleaseLane(2); }
void ARhythmPlayerController::OnLane3Down() { if (ARhythmGameMode* GM = GetGM()) GM->PressLane(3); }
void ARhythmPlayerController::OnLane3Up() { if (ARhythmGameMode* GM = GetGM()) GM->ReleaseLane(3); }

void ARhythmPlayerController::OnClickPressed()
{
	ARhythmGameMode* GM = GetGM();
	if (!GM)
	{
		return;
	}
	if (GM->Screen == ERhythmScreen::Playing)
	{
		return;
	}
	if (ARhythmHUD* HUD = GetRhythmHUD())
	{
		float X = 0.0f, Y = 0.0f;
		if (GetMousePosition(X, Y))
		{
			HUD->HandleClick(FVector2D(X, Y));
		}
	}
}

void ARhythmPlayerController::OnClickReleased()
{
}

void ARhythmPlayerController::OnEscapePressed()
{
	if (ARhythmGameMode* GM = GetGM())
	{
		GM->Back();
	}
}

void ARhythmPlayerController::OnUpPressed()
{
	if (ARhythmGameMode* GM = GetGM())
	{
		GM->MenuUp();
	}
}

void ARhythmPlayerController::OnDownPressed()
{
	if (ARhythmGameMode* GM = GetGM())
	{
		GM->MenuDown();
	}
}

void ARhythmPlayerController::OnConfirmPressed()
{
	if (ARhythmGameMode* GM = GetGM())
	{
		if (GM->Screen == ERhythmScreen::Playing)
		{
			return;
		}
		GM->Confirm();
	}
}
