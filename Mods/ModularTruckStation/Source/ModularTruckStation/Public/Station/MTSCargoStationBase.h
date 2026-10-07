#pragma once

#include "CoreMinimal.h"
#include "Station/MTSStationBase.h"
#include "MTSCargoStationBase.generated.h"

/** Modular station base for solid cargo. Parent of the base BP. */
UCLASS(Abstract)
class MODULARTRUCKSTATION_API AMTSCargoStationBase : public AMTSStationBase
{
	GENERATED_BODY()
public:
	AMTSCargoStationBase();
};
