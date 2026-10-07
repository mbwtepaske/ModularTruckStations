#pragma once

#include "CoreMinimal.h"
#include "FGConstructDisqualifier.h"
#include "Hologram/FGFactoryHologram.h"
#include "Station/MTSStationTypes.h"
#include "MTSStationModuleHologram.generated.h"

class AMTSStationBase;

/** Shown when a module hologram is not snapped to a free modular station base of its type. */
UCLASS()
class MODULARTRUCKSTATION_API UMTSCDMustAttachToBase : public UFGConstructDisqualifier
{
	GENERATED_BODY()
public:
	UMTSCDMustAttachToBase()
	{
		mDisqfualifyingText = NSLOCTEXT("ModularTruckStation", "MustAttachToBase", "Must be attached to a free modular station base of the same type");
	}
};

/**
 * Hologram of a station module: it only places by snapping to the module attach point of a modular station base of
 * the same type that has no module yet, and links the built module to that base.
 */
UCLASS()
class MODULARTRUCKSTATION_API AMTSStationModuleHologram : public AFGFactoryHologram
{
	GENERATED_BODY()
public:
	// Begin AFGHologram interface
	virtual bool IsValidHitResult(const FHitResult& hitResult) const override;
	virtual bool TrySnapToActor(const FHitResult& hitResult) override;
	virtual void CheckValidPlacement() override;
	// End AFGHologram interface

protected:
	// Begin AFGBuildableHologram interface
	virtual void ConfigureActor(AFGBuildable* inBuildable) const override;
	// End AFGBuildableHologram interface

private:
	/** @return the base the hologram is snapped to, or null. */
	AMTSStationBase* GetSnappedStationBase() const;

	/** @return true if a module of this hologram's class may attach to the base. */
	bool CanAttachTo(const AMTSStationBase* StationBase) const;
};
