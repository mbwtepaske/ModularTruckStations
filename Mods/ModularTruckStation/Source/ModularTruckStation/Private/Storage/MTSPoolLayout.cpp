#include "Storage/MTSPoolLayout.h"

#include "Resources/FGItemDescriptor.h"

namespace
{
	/** Stock of one item type: stateless items merged into one total, stateful stacks kept apart. */
	struct FMTSItemStock
	{
		TSubclassOf<UFGItemDescriptor> ItemClass;
		int32 StatelessItems = 0;
		TArray<FMTSStack> StatefulStacks;

		int32 RequiredSlots(int32 StackSize) const
		{
			return FMath::DivideAndRoundUp(StatelessItems, StackSize) + StatefulStacks.Num();
		}
	};

	int32 SafeStackSize(FMTSGetStackSize GetStackSize, TSubclassOf<UFGItemDescriptor> ItemClass)
	{
		return FMath::Max(1, GetStackSize(ItemClass));
	}

	/** Groups stacks per item type, in order of first appearance. Empty stacks are ignored. */
	TArray<FMTSItemStock> GroupStock(const TArray<FMTSStack>& Stacks)
	{
		TArray<FMTSItemStock> Stock;
		for (const FMTSStack& Stack : Stacks)
		{
			if (!Stack.HasItems())
			{
				continue;
			}

			const TSubclassOf<UFGItemDescriptor> ItemClass = Stack.ItemClass;
			FMTSItemStock* Entry = Stock.FindByPredicate([&ItemClass](const FMTSItemStock& Existing) { return Existing.ItemClass == ItemClass; });
			if (!Entry)
			{
				Entry = &Stock.AddDefaulted_GetRef();
				Entry->ItemClass = ItemClass;
			}

			if (Stack.bHasState)
			{
				Entry->StatefulStacks.Add(Stack);
			}
			else
			{
				Entry->StatelessItems += Stack.NumItems;
			}
		}
		return Stock;
	}
}

FMTSPoolLayout FMTSPoolLayout::Build(const TArray<TSubclassOf<UFGItemDescriptor>>& ColumnFilters, int32 SlotsPerColumn)
{
	FMTSPoolLayout Layout;

	for (const TSubclassOf<UFGItemDescriptor>& Filter : ColumnFilters)
	{
		if (!Filter)
		{
			continue;
		}

		const int32 PoolIndex = Layout.FindPoolIndex(Filter);
		FMTSPool& Pool = PoolIndex != INDEX_NONE ? Layout.Pools[PoolIndex] : Layout.Pools.AddDefaulted_GetRef();
		Pool.ItemClass = Filter;
		++Pool.NumColumns;
	}

	const int32 SlotsPerUnit = FMath::Max(0, SlotsPerColumn);
	for (FMTSPool& Pool : Layout.Pools)
	{
		Pool.FirstSlot = Layout.InventorySize;
		Pool.NumSlots = Pool.NumColumns * SlotsPerUnit;
		Layout.InventorySize += Pool.NumSlots;
	}

	return Layout;
}

int32 FMTSPoolLayout::FindPoolIndex(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	if (!ItemClass)
	{
		return INDEX_NONE;
	}
	return Pools.IndexOfByPredicate([&ItemClass](const FMTSPool& Pool) { return Pool.ItemClass == ItemClass; });
}

int32 FMTSPoolLayout::FindPoolIndexForSlot(int32 Slot) const
{
	return Pools.IndexOfByPredicate([Slot](const FMTSPool& Pool) { return Pool.ContainsSlot(Slot); });
}

TSubclassOf<UFGItemDescriptor> FMTSPoolLayout::GetAllowedItemOnSlot(int32 Slot) const
{
	const int32 PoolIndex = FindPoolIndexForSlot(Slot);
	return PoolIndex != INDEX_NONE ? Pools[PoolIndex].ItemClass : nullptr;
}

bool FMTSPoolLayout::IsItemAllowedOnSlot(TSubclassOf<UFGItemDescriptor> ItemClass, int32 Slot) const
{
	return ItemClass && GetAllowedItemOnSlot(Slot) == ItemClass;
}

TMap<TSubclassOf<UFGItemDescriptor>, int32> FMTSPoolLayout::CountRequiredSlots(const TArray<FMTSStack>& Stacks, FMTSGetStackSize GetStackSize)
{
	TMap<TSubclassOf<UFGItemDescriptor>, int32> Required;
	for (const FMTSItemStock& Stock : GroupStock(Stacks))
	{
		Required.Add(Stock.ItemClass, Stock.RequiredSlots(SafeStackSize(GetStackSize, Stock.ItemClass)));
	}
	return Required;
}

EMTSFitResult FMTSPoolLayout::CheckFit(const TArray<FMTSStack>& Stacks, FMTSGetStackSize GetStackSize, TSubclassOf<UFGItemDescriptor>* OutFailedItem) const
{
	for (const FMTSItemStock& Stock : GroupStock(Stacks))
	{
		const int32 PoolIndex = FindPoolIndex(Stock.ItemClass);
		EMTSFitResult Result = EMTSFitResult::Fits;
		if (PoolIndex == INDEX_NONE)
		{
			Result = EMTSFitResult::TypeHasNoPool;
		}
		else if (Stock.RequiredSlots(SafeStackSize(GetStackSize, Stock.ItemClass)) > Pools[PoolIndex].NumSlots)
		{
			Result = EMTSFitResult::PoolTooSmall;
		}

		if (Result != EMTSFitResult::Fits)
		{
			if (OutFailedItem)
			{
				*OutFailedItem = Stock.ItemClass;
			}
			return Result;
		}
	}
	return EMTSFitResult::Fits;
}

EMTSFitResult FMTSPoolLayout::PlanRedistribution(const TArray<FMTSStack>& Stacks, FMTSGetStackSize GetStackSize, TArray<FMTSStack>& OutSlots, TSubclassOf<UFGItemDescriptor>* OutFailedItem) const
{
	OutSlots.Reset();

	const EMTSFitResult Result = CheckFit(Stacks, GetStackSize, OutFailedItem);
	if (Result != EMTSFitResult::Fits)
	{
		return Result;
	}

	OutSlots.SetNum(InventorySize);
	for (const FMTSItemStock& Stock : GroupStock(Stacks))
	{
		const FMTSPool& Pool = Pools[FindPoolIndex(Stock.ItemClass)];
		const int32 StackSize = SafeStackSize(GetStackSize, Stock.ItemClass);
		int32 Slot = Pool.FirstSlot;

		for (int32 Remaining = Stock.StatelessItems; Remaining > 0; )
		{
			const int32 NumItems = FMath::Min(Remaining, StackSize);
			OutSlots[Slot++] = FMTSStack(Stock.ItemClass, NumItems);
			Remaining -= NumItems;
		}
		for (const FMTSStack& StatefulStack : Stock.StatefulStacks)
		{
			OutSlots[Slot++] = StatefulStack;
		}
	}
	return EMTSFitResult::Fits;
}
