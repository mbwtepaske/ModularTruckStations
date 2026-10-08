#pragma once

#include "CoreMinimal.h"
#include "Station/MTSStationTypes.h"

/** Pure station rules, shared by the base, the module hologram and tests. */
namespace MTSStationRules
{
	/** A module attaches only to a base of the same station type that has no other module. */
	FORCEINLINE bool CanAttachModule(EMTSStationType BaseType, bool bBaseHasOtherModule, EMTSStationType ModuleType)
	{
		return BaseType == ModuleType && !bBaseHasOtherModule;
	}
}
