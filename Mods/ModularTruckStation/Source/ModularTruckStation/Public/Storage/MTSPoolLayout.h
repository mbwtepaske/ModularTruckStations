#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "Templates/SubclassOf.h"
#include "FGInventoryComponent.h"

class UFGItemDescriptor;

/** Returns the number of items of the given type that fit in one inventory slot. */
using FMTSGetStackSize = TFunctionRef<int32(TSubclassOf<UFGItemDescriptor>)>;

/** Result of checking whether existing stock fits a pool layout. */
enum class EMTSFitResult : uint8
{
	/** Every item type has a pool with enough slots. */
	Fits,
	/** An item type still has a pool, but the pool has fewer slots than its stock needs. */
	PoolTooSmall,
	/** An item type with stock has no pool in the layout anymore. */
	TypeHasNoPool
};

/**
 * Item type and count of one inventory slot. The pool logic uses this instead of FInventoryStack so it only depends on
 * inline FactoryGame code and runs in editor automation tests, where FactoryGame implementations are stubs.
 */
struct MODULARTRUCKSTATION_API FMTSStack
{
	TSubclassOf<UFGItemDescriptor> ItemClass;
	int32 NumItems = 0;

	/** Stacks with item state are never merged with other stacks. */
	bool bHasState = false;

	/** Index of the inventory slot this stack came from; used to copy item state when the stack is moved. */
	int32 SourceIndex = INDEX_NONE;

	FMTSStack() = default;
	FMTSStack(TSubclassOf<UFGItemDescriptor> InItemClass, int32 InNumItems, bool bInHasState = false, int32 InSourceIndex = INDEX_NONE)
		: ItemClass(InItemClass), NumItems(InNumItems), bHasState(bInHasState), SourceIndex(InSourceIndex)
	{
	}

	static FMTSStack FromInventoryStack(const FInventoryStack& Stack, int32 SourceIndex)
	{
		return FMTSStack(Stack.Item.GetItemClass(), Stack.NumItems, Stack.Item.HasState(), SourceIndex);
	}

	FORCEINLINE bool HasItems() const { return ItemClass && NumItems > 0; }
};

/** One typed pool: a contiguous slot range in the station inventory, reserved for one item type. */
struct MODULARTRUCKSTATION_API FMTSPool
{
	TSubclassOf<UFGItemDescriptor> ItemClass;

	/** Number of module columns that filter this item type. */
	int32 NumColumns = 0;

	int32 FirstSlot = 0;
	int32 NumSlots = 0;

	FORCEINLINE bool ContainsSlot(int32 Slot) const { return Slot >= FirstSlot && Slot < FirstSlot + NumSlots; }
};

/**
 * Pure storage layout of a modular station: column filters and the capacity unit map to ordered pools and slot ranges.
 * Pools are ordered by the index of their first column. Unset (null) columns add no pool and no slots.
 */
struct MODULARTRUCKSTATION_API FMTSPoolLayout
{
	/** Builds the layout for the given column filters, each active column contributing SlotsPerColumn slots. */
	static FMTSPoolLayout Build(const TArray<TSubclassOf<UFGItemDescriptor>>& ColumnFilters, int32 SlotsPerColumn);

	FORCEINLINE const TArray<FMTSPool>& GetPools() const { return Pools; }
	FORCEINLINE int32 GetInventorySize() const { return InventorySize; }

	/** @return index into GetPools() of the pool for the item type, or INDEX_NONE. */
	int32 FindPoolIndex(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	/** @return index into GetPools() of the pool owning the slot, or INDEX_NONE. */
	int32 FindPoolIndexForSlot(int32 Slot) const;

	/** @return the item type the slot is reserved for, or null if the slot is outside the layout. */
	TSubclassOf<UFGItemDescriptor> GetAllowedItemOnSlot(int32 Slot) const;

	/** @return true if the item may be stored in the slot. */
	bool IsItemAllowedOnSlot(TSubclassOf<UFGItemDescriptor> ItemClass, int32 Slot) const;

	/**
	 * Counts the slots the stacks need per item type. Stacks of the same item without item state are merged first;
	 * stacks with item state are never merged and need one slot each.
	 */
	static TMap<TSubclassOf<UFGItemDescriptor>, int32> CountRequiredSlots(const TArray<FMTSStack>& Stacks, FMTSGetStackSize GetStackSize);

	/**
	 * Checks whether the stacks fit this layout.
	 * @param OutFailedItem	set to the first item type that does not fit, when the result is not Fits.
	 */
	EMTSFitResult CheckFit(const TArray<FMTSStack>& Stacks, FMTSGetStackSize GetStackSize, TSubclassOf<UFGItemDescriptor>* OutFailedItem = nullptr) const;

	/**
	 * Plans the slot contents of this layout for the given stacks, without touching any inventory.
	 * On success OutSlots has GetInventorySize() entries (empty stacks for free slots) and holds exactly the same items;
	 * stacks with item state keep their SourceIndex, merged stacks have none. On failure OutSlots is empty.
	 */
	EMTSFitResult PlanRedistribution(const TArray<FMTSStack>& Stacks, FMTSGetStackSize GetStackSize, TArray<FMTSStack>& OutSlots, TSubclassOf<UFGItemDescriptor>* OutFailedItem = nullptr) const;

private:
	TArray<FMTSPool> Pools;
	int32 InventorySize = 0;
};
