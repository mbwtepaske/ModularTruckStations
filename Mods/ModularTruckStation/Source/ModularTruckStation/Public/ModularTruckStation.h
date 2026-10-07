// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

MODULARTRUCKSTATION_API DECLARE_LOG_CATEGORY_EXTERN(LogModularTruckStation, Log, All);

class FModularTruckStationModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
