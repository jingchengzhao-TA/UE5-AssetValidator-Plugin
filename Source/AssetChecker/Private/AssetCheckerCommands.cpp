// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetCheckerCommands.h"

#define LOCTEXT_NAMESPACE "FAssetCheckerModule"

void FAssetCheckerCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "AssetChecker", "Bring up AssetChecker window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
