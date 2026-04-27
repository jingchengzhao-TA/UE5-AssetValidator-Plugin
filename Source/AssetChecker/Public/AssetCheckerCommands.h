// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "AssetCheckerStyle.h"

class FAssetCheckerCommands : public TCommands<FAssetCheckerCommands>
{
public:

	FAssetCheckerCommands()
		: TCommands<FAssetCheckerCommands>(TEXT("AssetChecker"), NSLOCTEXT("Contexts", "AssetChecker", "AssetChecker Plugin"), NAME_None, FAssetCheckerStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};