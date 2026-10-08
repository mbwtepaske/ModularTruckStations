#pragma once

#include "CoreMinimal.h"
#include "Module/MTSStationModule.h"
#include "MTSCargoModule.generated.h"

/** Storage module for solid cargo. Parent of the S, M, L and XL module BPs. */
UCLASS(Abstract)
class MODULARTRUCKSTATION_API AMTSCargoModule : public AMTSStationModule
{
	GENERATED_BODY()
public:
	AMTSCargoModule();
};
