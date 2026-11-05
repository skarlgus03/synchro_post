#include "Framework/SPGameMode.h"
#include "Framework/SPPlayerController.h"
#include "Framework/SPPlayerState.h"
#include "Framework/SPGameState.h"
#include "Framework/RunProgressComponent.h"


ASPGameMode::ASPGameMode()
{
	PlayerControllerClass = ASPPlayerController::StaticClass();
	PlayerStateClass = ASPPlayerState::StaticClass();
	GameStateClass = ASPGameState::StaticClass();
}

