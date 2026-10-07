#include "Module/MTSStationModuleHologram.h"

#include "Module/MTSStationModule.h"
#include "Station/MTSStationBase.h"
#include "Station/MTSStationRules.h"

bool AMTSStationModuleHologram::IsValidHitResult(const FHitResult& hitResult) const
{
	// Aiming at a base is valid even where a factory hologram would normally need a floor.
	return Cast<AMTSStationBase>(hitResult.GetActor()) || Super::IsValidHitResult(hitResult);
}

bool AMTSStationModuleHologram::TrySnapToActor(const FHitResult& hitResult)
{
	AMTSStationBase* StationBase = Cast<AMTSStationBase>(hitResult.GetActor());
	if (StationBase && CanAttachTo(StationBase))
	{
		SetActorTransform(StationBase->GetModuleAttachTransform());
		mSnappedBuilding = StationBase;
		return true;
	}

	// Never snaps to anything else; CheckValidPlacement rejects the free placement.
	mSnappedBuilding = nullptr;
	return false;
}

void AMTSStationModuleHologram::CheckValidPlacement()
{
	Super::CheckValidPlacement();

	if (!GetSnappedStationBase())
	{
		AddConstructDisqualifier(UMTSCDMustAttachToBase::StaticClass());
	}
}

void AMTSStationModuleHologram::ConfigureActor(AFGBuildable* inBuildable) const
{
	Super::ConfigureActor(inBuildable);

	if (AMTSStationModule* Module = Cast<AMTSStationModule>(inBuildable))
	{
		Module->SetStationBase(GetSnappedStationBase());
	}
}

AMTSStationBase* AMTSStationModuleHologram::GetSnappedStationBase() const
{
	AMTSStationBase* StationBase = Cast<AMTSStationBase>(mSnappedBuilding);
	return StationBase && CanAttachTo(StationBase) ? StationBase : nullptr;
}

bool AMTSStationModuleHologram::CanAttachTo(const AMTSStationBase* StationBase) const
{
	const AMTSStationModule* ModuleDefaults = GetBuildClass() ? Cast<AMTSStationModule>(GetBuildClass()->GetDefaultObject()) : nullptr;
	return StationBase && ModuleDefaults
		&& MTSStationRules::CanAttachModule(StationBase->GetStationType(), StationBase->GetAttachedModule() != nullptr, ModuleDefaults->GetStationType());
}
