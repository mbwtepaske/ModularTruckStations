#include "Station/MTSStationBase.h"

#include "FGInventoryComponent.h"
#include "Module/MTSStationModule.h"
#include "ModularTruckStation.h"
#include "Net/UnrealNetwork.h"
#include "Resources/FGItemDescriptor.h"
#include "Station/MTSStationRules.h"

namespace
{
	/**
	 * Vanilla truck transfer asserts on an inventory without slots, so the storage always keeps at least one slot.
	 * Slots outside the layout are rejected by the item filter, so this slot never holds items.
	 */
	constexpr int32 MinStorageSize = 1;

	int32 StackSizeOf(TSubclassOf<UFGItemDescriptor> ItemClass)
	{
		return UFGItemDescriptor::GetStackSize(ItemClass);
	}
}

AMTSStationBase::AMTSStationBase()
{
	// No usable cargo storage until a module is attached.
	mStorageInventorySize = MinStorageSize;
}

void AMTSStationBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMTSStationBase, mAttachedModule);
}

void AMTSStationBase::BeginPlay()
{
	// mStorageInventorySize must stay at its class default: a loaded inventory is rebuilt as that default plus its
	// saved size difference (UFGInventoryComponent::mAdjustedSizeDiff).
	Super::BeginPlay();

	if (mInventory)
	{
		mInventory->mItemFilter.BindUObject(this, &AMTSStationBase::FilterStorageItem);
	}

	if (HasAuthority())
	{
		const EMTSFitResult Result = RebuildLayout();
		if (Result != EMTSFitResult::Fits)
		{
			UE_LOG(LogModularTruckStation, Error, TEXT("%s: stored cargo does not fit the module layout after load (%d); storage left unchanged"), *GetName(), static_cast<int32>(Result));
		}
	}
	UE_LOG(LogModularTruckStation, Log, TEXT("%s BeginPlay: module %s, storage size %d"), *GetName(), mAttachedModule ? *mAttachedModule->GetName() : TEXT("none"), mInventory ? mInventory->GetSizeLinear() : -1);
}

void AMTSStationBase::GetDismantleInventoryReturns(TArray<FInventoryStack>& out_returns) const
{
	// The storage is refunded by the attached module, so each item is returned exactly once whether the module is
	// dismantled alone or together with the base. Without a module the storage is normally empty, but anything left
	// there is returned here rather than lost.
	if (!mAttachedModule)
	{
		GetStorageStacks(out_returns);
	}
	if (mFuelInventory)
	{
		TArray<FInventoryStack> FuelStacks;
		mFuelInventory->GetInventoryStacks(FuelStacks);
		out_returns.Append(FuelStacks);
	}
}

void AMTSStationBase::GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const
{
	Super::GetChildDismantleActors_Implementation(out_ChildDismantleActors);
	if (mAttachedModule)
	{
		out_ChildDismantleActors.AddUnique(mAttachedModule);
	}
}

float AMTSStationBase::GetProducingPowerConsumptionBase() const
{
	const AMTSStationModule* Module = mAttachedModule;
	const float ModulePower = Module ? Module->GetNumColumns() * Module->GetPowerPerColumn() : 0.f;
	return Super::GetProducingPowerConsumptionBase() + ModulePower;
}

bool AMTSStationBase::CanAcceptModule(const AMTSStationModule* Module) const
{
	return Module && MTSStationRules::CanAttachModule(mStationType, mAttachedModule && mAttachedModule != Module, Module->GetStationType());
}

FTransform AMTSStationBase::GetModuleAttachTransform() const
{
	TArray<USceneComponent*> AttachPoints;
	GetComponents<USceneComponent>(AttachPoints);
	for (const USceneComponent* Component : AttachPoints)
	{
		if (Component->ComponentHasTag(mModuleAttachPointTag))
		{
			return Component->GetComponentTransform();
		}
	}
	return GetActorTransform();
}

void AMTSStationBase::AttachModule(AMTSStationModule* Module)
{
	if (!HasAuthority() || !CanAcceptModule(Module))
	{
		return;
	}

	mAttachedModule = Module;
	TSubclassOf<UFGItemDescriptor> FailedItem;
	const EMTSFitResult Result = RebuildLayout(&FailedItem);
	if (Result != EMTSFitResult::Fits)
	{
		UE_LOG(LogModularTruckStation, Error, TEXT("%s: stock of %s does not fit the layout of %s (%d)"), *GetName(), *GetNameSafe(FailedItem), *Module->GetName(), static_cast<int32>(Result));
	}
}

void AMTSStationBase::DetachModule(AMTSStationModule* Module)
{
	if (!HasAuthority() || !Module || mAttachedModule != Module)
	{
		return;
	}

	mAttachedModule = nullptr;
	{
		FScopeLock Lock(&mStorageLock);
		if (mInventory)
		{
			mInventory->Empty();
		}
	}
	ApplyLayout(FMTSPoolLayout());
}

EMTSFitResult AMTSStationBase::RebuildLayout(TSubclassOf<UFGItemDescriptor>* OutFailedItem)
{
	const FMTSPoolLayout NewLayout = mAttachedModule
		? FMTSPoolLayout::Build(mAttachedModule->GetColumnFilters(), mAttachedModule->GetSlotsPerColumn())
		: FMTSPoolLayout();
	return ApplyLayout(NewLayout, OutFailedItem);
}

EMTSFitResult AMTSStationBase::CheckLayout(const FMTSPoolLayout& Layout, TSubclassOf<UFGItemDescriptor>* OutFailedItem) const
{
	FScopeLock Lock(&mStorageLock);
	TArray<FInventoryStack> Slots;
	return Layout.CheckFit(GatherStock_Locked(Slots), &StackSizeOf, OutFailedItem);
}

EMTSFitResult AMTSStationBase::ApplyLayout(const FMTSPoolLayout& NewLayout, TSubclassOf<UFGItemDescriptor>* OutFailedItem)
{
	FScopeLock Lock(&mStorageLock);
	if (!mInventory)
	{
		mLayout = NewLayout;
		return EMTSFitResult::Fits;
	}

	// Plan first: nothing is touched when the stock does not fit.
	TArray<FInventoryStack> OldSlots;
	TArray<FMTSStack> Plan;
	const EMTSFitResult Result = NewLayout.PlanRedistribution(GatherStock_Locked(OldSlots), &StackSizeOf, Plan, OutFailedItem);
	if (Result != EMTSFitResult::Fits)
	{
		return Result;
	}

	mLayout = NewLayout;
	mInventory->Empty();
	mInventory->Resize(FMath::Max(MinStorageSize, mLayout.GetInventorySize()));
	for (int32 Slot = 0; Slot < mInventory->GetSizeLinear(); ++Slot)
	{
		mInventory->SetAllowedItemOnIndex(Slot, mLayout.GetAllowedItemOnSlot(Slot));
	}

	for (int32 Slot = 0; Slot < Plan.Num(); ++Slot)
	{
		const FMTSStack& Planned = Plan[Slot];
		if (!Planned.HasItems())
		{
			continue;
		}

		// Stacks with item state are copied whole from their source slot so the state is kept.
		const FInventoryStack Stack = Planned.SourceIndex != INDEX_NONE ? OldSlots[Planned.SourceIndex] : FInventoryStack(Planned.NumItems, Planned.ItemClass);
		const int32 NumAdded = mInventory->AddStackToIndex(Slot, Stack);
		if (NumAdded != Stack.NumItems)
		{
			UE_LOG(LogModularTruckStation, Error, TEXT("%s: only %d of %d %s re-added to slot %d"), *GetName(), NumAdded, Stack.NumItems, *GetNameSafe(Planned.ItemClass), Slot);
		}
	}
	return EMTSFitResult::Fits;
}

bool AMTSStationBase::HasSpaceInPool(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	FScopeLock Lock(&mStorageLock);
	return FindSlotWithSpace_Locked(ItemClass) != INDEX_NONE;
}

bool AMTSStationBase::AddToPool(const FInventoryItem& Item)
{
	FScopeLock Lock(&mStorageLock);
	const int32 Slot = FindSlotWithSpace_Locked(Item.GetItemClass());
	return Slot != INDEX_NONE && mInventory->AddStackToIndex(Slot, FInventoryStack(Item)) == 1;
}

bool AMTSStationBase::PeekPool(TSubclassOf<UFGItemDescriptor> ItemClass, FInventoryItem& OutItem) const
{
	FScopeLock Lock(&mStorageLock);
	const int32 Slot = FindSlotWithItems_Locked(ItemClass);
	FInventoryStack Stack;
	if (Slot == INDEX_NONE || !mInventory->GetStackFromIndex(Slot, Stack))
	{
		return false;
	}
	OutItem = Stack.Item;
	return true;
}

bool AMTSStationBase::TakeFromPool(TSubclassOf<UFGItemDescriptor> ItemClass, FInventoryItem& OutItem)
{
	FScopeLock Lock(&mStorageLock);
	const int32 Slot = FindSlotWithItems_Locked(ItemClass);
	FInventoryStack Stack;
	if (Slot == INDEX_NONE || !mInventory->GetStackFromIndex(Slot, Stack))
	{
		return false;
	}
	OutItem = Stack.Item;
	mInventory->RemoveFromIndex(Slot, 1);
	return true;
}

void AMTSStationBase::GetStorageStacks(TArray<FInventoryStack>& OutStacks) const
{
	FScopeLock Lock(&mStorageLock);
	if (mInventory)
	{
		TArray<FInventoryStack> Stacks;
		mInventory->GetInventoryStacks(Stacks);
		OutStacks.Append(Stacks);
	}
}

bool AMTSStationBase::FilterStorageItem(TSubclassOf<UObject> ItemClass, int32 Index) const
{
	FScopeLock Lock(&mStorageLock);
	const TSubclassOf<UFGItemDescriptor> ItemDescriptor = *ItemClass;
	if (Index == INDEX_NONE)
	{
		// Asked without a slot: allowed if any pool holds this type.
		return mLayout.FindPoolIndex(ItemDescriptor) != INDEX_NONE;
	}
	return mLayout.IsItemAllowedOnSlot(ItemDescriptor, Index);
}

int32 AMTSStationBase::FindSlotWithSpace_Locked(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	const int32 PoolIndex = mLayout.FindPoolIndex(ItemClass);
	if (!mInventory || PoolIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	// Top up partial stacks before starting a new one.
	const FMTSPool& Pool = mLayout.GetPools()[PoolIndex];
	const int32 StackSize = StackSizeOf(ItemClass);
	int32 EmptySlot = INDEX_NONE;
	for (int32 Slot = Pool.FirstSlot; Slot < Pool.FirstSlot + Pool.NumSlots; ++Slot)
	{
		FInventoryStack Stack;
		if (!mInventory->GetStackFromIndex(Slot, Stack) || !Stack.HasItems())
		{
			EmptySlot = EmptySlot == INDEX_NONE ? Slot : EmptySlot;
		}
		else if (!Stack.Item.HasState() && Stack.NumItems < StackSize)
		{
			return Slot;
		}
	}
	return EmptySlot;
}

int32 AMTSStationBase::FindSlotWithItems_Locked(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	const int32 PoolIndex = mLayout.FindPoolIndex(ItemClass);
	if (!mInventory || PoolIndex == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	const FMTSPool& Pool = mLayout.GetPools()[PoolIndex];
	for (int32 Slot = Pool.FirstSlot; Slot < Pool.FirstSlot + Pool.NumSlots; ++Slot)
	{
		if (!mInventory->IsIndexEmpty(Slot))
		{
			return Slot;
		}
	}
	return INDEX_NONE;
}

TArray<FMTSStack> AMTSStationBase::GatherStock_Locked(TArray<FInventoryStack>& OutSlots) const
{
	TArray<FMTSStack> Stock;
	OutSlots.Reset();
	if (!mInventory)
	{
		return Stock;
	}

	OutSlots.SetNum(mInventory->GetSizeLinear());
	for (int32 Slot = 0; Slot < OutSlots.Num(); ++Slot)
	{
		if (mInventory->GetStackFromIndex(Slot, OutSlots[Slot]) && OutSlots[Slot].HasItems())
		{
			Stock.Add(FMTSStack::FromInventoryStack(OutSlots[Slot], Slot));
		}
	}
	return Stock;
}
