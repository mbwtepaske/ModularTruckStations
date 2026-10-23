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

	/** Only solid items; fluids and gases belong to the fluid station. */
	virtual bool CanFilterItem(TSubclassOf<UFGItemDescriptor> ItemClass) const override;
};
