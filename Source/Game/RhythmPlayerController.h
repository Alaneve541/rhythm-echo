#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RhythmPlayerController.generated.h"

class ARhythmGameMode;
class ARhythmHUD;

UCLASS()
class GAME_API ARhythmPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARhythmPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void OnLane0Down();
	void OnLane0Up();
	void OnLane1Down();
	void OnLane1Up();
	void OnLane2Down();
	void OnLane2Up();
	void OnLane3Down();
	void OnLane3Up();
	void OnClickPressed();
	void OnClickReleased();
	void OnEscapePressed();
	void OnUpPressed();
	void OnDownPressed();
	void OnConfirmPressed();

	ARhythmGameMode* GetGM() const;
	ARhythmHUD* GetRhythmHUD() const;
};
