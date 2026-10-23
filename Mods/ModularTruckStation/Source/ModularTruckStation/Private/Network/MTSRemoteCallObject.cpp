#include "Network/MTSRemoteCallObject.h"

#include "Engine/Engine.h"
#include "FGPlayerController.h"
#include "Module/MTSStationModule.h"
#include "ModularTruckStation.h"
#include "Net/UnrealNetwork.h"
#include "Resources/FGItemDescriptor.h"

namespace
{
	/** Generous reach (cm) for module UI actions; a module is several meters wide. */
	constexpr double MaxReach = 3000.0;
}

void UMTSRemoteCallObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMTSRemoteCallObject, mForceNetField_MTSRemoteCallObject);
}

UMTSRemoteCallObject* UMTSRemoteCallObject::GetLocal(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	AFGPlayerController* PlayerController = World ? Cast<AFGPlayerController>(World->GetFirstPlayerController()) : nullptr;
	return PlayerController ? PlayerController->GetRemoteCallObjectOfClass<UMTSRemoteCallObject>() : nullptr;
}

void UMTSRemoteCallObject::Server_SetColumnFilter_Implementation(AMTSStationModule* Module, int32 Column, TSubclassOf<UFGItemDescriptor> ItemClass)
{
	EMTSFilterChangeResult Result = EMTSFilterChangeResult::NotInReach;
	TSubclassOf<UFGItemDescriptor> FailedItem;
	if (IsInReach(Module))
	{
		Result = Module->SetColumnFilter(Column, ItemClass, &FailedItem);
	}
	UE_LOG(LogModularTruckStation, Log, TEXT("%s filter %d -> %s: %s"), *GetNameSafe(Module), Column, *GetNameSafe(ItemClass), *UEnum::GetValueAsString(Result));
	Client_FilterChangeResult(Module, Column, Result, FailedItem);
}

void UMTSRemoteCallObject::Server_SetAllColumnFilters_Implementation(AMTSStationModule* Module, TSubclassOf<UFGItemDescriptor> ItemClass)
{
	EMTSFilterChangeResult Result = EMTSFilterChangeResult::NotInReach;
	TSubclassOf<UFGItemDescriptor> FailedItem;
	if (IsInReach(Module))
	{
		Result = Module->SetAllColumnFilters(ItemClass, &FailedItem);
	}
	UE_LOG(LogModularTruckStation, Log, TEXT("%s all filters -> %s: %s"), *GetNameSafe(Module), *GetNameSafe(ItemClass), *UEnum::GetValueAsString(Result));
	Client_FilterChangeResult(Module, INDEX_NONE, Result, FailedItem);
}

void UMTSRemoteCallObject::Server_TogglePort_Implementation(AMTSStationModule* Module, int32 Slot)
{
	const EMTSPortToggleResult Result = IsInReach(Module) ? Module->TogglePortDirection(Slot) : EMTSPortToggleResult::NotInReach;
	UE_LOG(LogModularTruckStation, Log, TEXT("%s port %d: %s"), *GetNameSafe(Module), Slot, *UEnum::GetValueAsString(Result));
	Client_PortToggleResult(Module, Slot, Result);
}

void UMTSRemoteCallObject::Server_SetPortDirection_Implementation(AMTSStationModule* Module, int32 Slot, EMTSPortDirection Direction)
{
	const EMTSPortToggleResult Result = IsInReach(Module) ? Module->SetPortDirection(Slot, Direction) : EMTSPortToggleResult::NotInReach;
	UE_LOG(LogModularTruckStation, Log, TEXT("%s port %d -> %s: %s"), *GetNameSafe(Module), Slot, *UEnum::GetValueAsString(Direction), *UEnum::GetValueAsString(Result));
	Client_PortToggleResult(Module, Slot, Result);
}

void UMTSRemoteCallObject::Client_FilterChangeResult_Implementation(AMTSStationModule* Module, int32 Column, EMTSFilterChangeResult Result, TSubclassOf<UFGItemDescriptor> FailedItem)
{
	OnFilterChangeResult.Broadcast(Module, Column, Result, FailedItem);
}

void UMTSRemoteCallObject::Client_PortToggleResult_Implementation(AMTSStationModule* Module, int32 Slot, EMTSPortToggleResult Result)
{
	OnPortToggleResult.Broadcast(Module, Slot, Result);
}

bool UMTSRemoteCallObject::IsInReach(const AMTSStationModule* Module) const
{
	const AFGPlayerController* PlayerController = GetOwnerPlayerController();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	return IsValid(Module) && Pawn && FVector::DistSquared(Pawn->GetActorLocation(), Module->GetActorLocation()) <= FMath::Square(MaxReach);
}
