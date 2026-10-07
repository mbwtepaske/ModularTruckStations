#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Storage/MTSPoolLayout.h"
#include "Resources/FGConsumableDescriptor.h"
#include "Resources/FGEquipmentDescriptor.h"
#include "Resources/FGItemDescriptor.h"
#include "Resources/FGResourceDescriptor.h"

namespace MTSPoolLayoutTests
{
	using FItemClass = TSubclassOf<UFGItemDescriptor>;

	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	// Distinct native classes stand in for item types; the layout only compares classes.
	FItemClass Iron() { return UFGResourceDescriptor::StaticClass(); }
	FItemClass Copper() { return UFGEquipmentDescriptor::StaticClass(); }
	FItemClass Concrete() { return UFGConsumableDescriptor::StaticClass(); }
	FItemClass Wire() { return UFGItemDescriptor::StaticClass(); }

	/** Every test item stacks to 100. */
	int32 StackSize(FItemClass) { return 100; }

	int32 CountItems(const TArray<FMTSStack>& Stacks, FItemClass ItemClass)
	{
		int32 Count = 0;
		for (const FMTSStack& Stack : Stacks)
		{
			if (Stack.HasItems() && Stack.ItemClass == ItemClass)
			{
				Count += Stack.NumItems;
			}
		}
		return Count;
	}
}

using namespace MTSPoolLayoutTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSPoolLayoutModuleSizesTest, "ModularTruckStation.Storage.PoolLayout.ModuleSizes", TestFlags)
bool FMTSPoolLayoutModuleSizesTest::RunTest(const FString& Parameters)
{
	const TArray<FItemClass> Items = { Iron(), Copper(), Concrete(), Wire() };
	const TCHAR* SizeNames[] = { TEXT("S"), TEXT("M"), TEXT("L"), TEXT("XL") };

	for (int32 NumColumns = 1; NumColumns <= 4; ++NumColumns)
	{
		TArray<FItemClass> Filters;
		for (int32 Column = 0; Column < NumColumns; ++Column)
		{
			Filters.Add(Items[Column]);
		}

		const FMTSPoolLayout Layout = FMTSPoolLayout::Build(Filters, 12);
		const FString Size = SizeNames[NumColumns - 1];
		TestEqual(Size + TEXT(" inventory size"), Layout.GetInventorySize(), NumColumns * 12);
		TestEqual(Size + TEXT(" pool count"), Layout.GetPools().Num(), NumColumns);
	}

	const FMTSPoolLayout XL = FMTSPoolLayout::Build({ Iron(), Copper(), Concrete(), Wire() }, 12);
	TestEqual(TEXT("XL with C = 12 equals the vanilla 48 slots"), XL.GetInventorySize(), 48);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSPoolLayoutSharedPoolsTest, "ModularTruckStation.Storage.PoolLayout.SharedPools", TestFlags)
bool FMTSPoolLayoutSharedPoolsTest::RunTest(const FString& Parameters)
{
	const FMTSPoolLayout Layout = FMTSPoolLayout::Build({ Copper(), Iron(), Iron() }, 12);
	const TArray<FMTSPool>& Pools = Layout.GetPools();

	if (!TestEqual(TEXT("pool count"), Pools.Num(), 2))
	{
		return false;
	}
	TestTrue(TEXT("pool 0 is copper"), Pools[0].ItemClass == Copper());
	TestEqual(TEXT("copper columns"), Pools[0].NumColumns, 1);
	TestEqual(TEXT("copper first slot"), Pools[0].FirstSlot, 0);
	TestEqual(TEXT("copper slots"), Pools[0].NumSlots, 12);
	TestTrue(TEXT("pool 1 is iron"), Pools[1].ItemClass == Iron());
	TestEqual(TEXT("iron columns"), Pools[1].NumColumns, 2);
	TestEqual(TEXT("iron first slot"), Pools[1].FirstSlot, 12);
	TestEqual(TEXT("iron slots"), Pools[1].NumSlots, 24);
	TestEqual(TEXT("inventory size"), Layout.GetInventorySize(), 36);

	TestEqual(TEXT("FindPoolIndex iron"), Layout.FindPoolIndex(Iron()), 1);
	TestEqual(TEXT("FindPoolIndex unknown type"), Layout.FindPoolIndex(Concrete()), INDEX_NONE);
	TestTrue(TEXT("slot 11 is copper"), Layout.GetAllowedItemOnSlot(11) == Copper());
	TestTrue(TEXT("slot 12 is iron"), Layout.GetAllowedItemOnSlot(12) == Iron());
	TestTrue(TEXT("iron allowed on slot 35"), Layout.IsItemAllowedOnSlot(Iron(), 35));
	TestFalse(TEXT("copper refused on iron slot"), Layout.IsItemAllowedOnSlot(Copper(), 12));
	TestFalse(TEXT("null item refused"), Layout.IsItemAllowedOnSlot(nullptr, 0));
	TestFalse(TEXT("slot past the end refused"), Layout.IsItemAllowedOnSlot(Iron(), 36));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSPoolLayoutUnsetColumnsTest, "ModularTruckStation.Storage.PoolLayout.UnsetColumns", TestFlags)
bool FMTSPoolLayoutUnsetColumnsTest::RunTest(const FString& Parameters)
{
	const FMTSPoolLayout Partial = FMTSPoolLayout::Build({ Copper(), Iron(), Iron(), nullptr }, 12);
	TestEqual(TEXT("unset column adds no slots"), Partial.GetInventorySize(), 36);
	TestEqual(TEXT("unset column adds no pool"), Partial.GetPools().Num(), 2);
	TestEqual(TEXT("no pool past the end"), Partial.FindPoolIndexForSlot(36), INDEX_NONE);

	const FMTSPoolLayout Gap = FMTSPoolLayout::Build({ nullptr, Iron() }, 12);
	TestEqual(TEXT("leading unset column: size"), Gap.GetInventorySize(), 12);
	TestEqual(TEXT("leading unset column: iron starts at 0"), Gap.GetPools()[0].FirstSlot, 0);

	const FMTSPoolLayout Empty = FMTSPoolLayout::Build({ nullptr, nullptr, nullptr, nullptr }, 12);
	TestEqual(TEXT("all unset: size"), Empty.GetInventorySize(), 0);
	TestEqual(TEXT("all unset: pools"), Empty.GetPools().Num(), 0);
	TestEqual(TEXT("all unset: FindPoolIndex"), Empty.FindPoolIndex(Iron()), INDEX_NONE);

	const FMTSPoolLayout NoModule = FMTSPoolLayout::Build({}, 12);
	TestEqual(TEXT("no columns: size"), NoModule.GetInventorySize(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSPoolLayoutOrderingTest, "ModularTruckStation.Storage.PoolLayout.PoolOrdering", TestFlags)
bool FMTSPoolLayoutOrderingTest::RunTest(const FString& Parameters)
{
	const FMTSPoolLayout Layout = FMTSPoolLayout::Build({ Iron(), Copper(), Iron(), Concrete() }, 10);
	const TArray<FMTSPool>& Pools = Layout.GetPools();

	if (!TestEqual(TEXT("pool count"), Pools.Num(), 3))
	{
		return false;
	}
	TestTrue(TEXT("first pool from column 0 is iron"), Pools[0].ItemClass == Iron());
	TestEqual(TEXT("iron uses both of its columns"), Pools[0].NumSlots, 20);
	TestTrue(TEXT("second pool is copper"), Pools[1].ItemClass == Copper());
	TestEqual(TEXT("copper starts after iron"), Pools[1].FirstSlot, 20);
	TestTrue(TEXT("third pool is concrete"), Pools[2].ItemClass == Concrete());
	TestEqual(TEXT("concrete starts after copper"), Pools[2].FirstSlot, 30);
	TestEqual(TEXT("inventory size"), Layout.GetInventorySize(), 40);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSPoolLayoutAllowedShrinkTest, "ModularTruckStation.Storage.FitCheck.AllowedShrink", TestFlags)
bool FMTSPoolLayoutAllowedShrinkTest::RunTest(const FString& Parameters)
{
	// Old layout: two iron columns of 2 slots. Three partial stacks (150 items) need 2 slots after merging.
	const TArray<FMTSStack> Stock = { FMTSStack(Iron(), 60), FMTSStack(Iron(), 60), FMTSStack(Iron(), 30) };
	TestEqual(TEXT("required slots after merge"), FMTSPoolLayout::CountRequiredSlots(Stock, StackSize).FindRef(Iron()), 2);

	// Column 2 changes to copper: iron pool shrinks to 2 slots, which still fits.
	const FMTSPoolLayout NewLayout = FMTSPoolLayout::Build({ Iron(), Copper() }, 2);
	TestEqual(TEXT("fit result"), NewLayout.CheckFit(Stock, StackSize), EMTSFitResult::Fits);

	TArray<FMTSStack> Slots;
	TestEqual(TEXT("plan result"), NewLayout.PlanRedistribution(Stock, StackSize, Slots), EMTSFitResult::Fits);
	TestEqual(TEXT("planned slot count"), Slots.Num(), 4);
	TestEqual(TEXT("iron kept"), CountItems(Slots, Iron()), 150);
	TestEqual(TEXT("copper pool empty"), CountItems(Slots, Copper()), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSPoolLayoutRejectedShrinkTest, "ModularTruckStation.Storage.FitCheck.RejectedShrink", TestFlags)
bool FMTSPoolLayoutRejectedShrinkTest::RunTest(const FString& Parameters)
{
	// 250 iron needs 3 slots; the shrunk iron pool has 2.
	const TArray<FMTSStack> Stock = { FMTSStack(Iron(), 100), FMTSStack(Iron(), 100), FMTSStack(Iron(), 50) };
	const FMTSPoolLayout NewLayout = FMTSPoolLayout::Build({ Iron(), Copper() }, 2);

	FItemClass FailedItem;
	TestEqual(TEXT("fit result"), NewLayout.CheckFit(Stock, StackSize, &FailedItem), EMTSFitResult::PoolTooSmall);
	TestTrue(TEXT("failed item is iron"), FailedItem == Iron());

	TArray<FMTSStack> Slots = { FMTSStack(Wire(), 1) };
	TestEqual(TEXT("plan result"), NewLayout.PlanRedistribution(Stock, StackSize, Slots), EMTSFitResult::PoolTooSmall);
	TestEqual(TEXT("no plan on failure"), Slots.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSPoolLayoutRejectedRemovalTest, "ModularTruckStation.Storage.FitCheck.RejectedRemovalOfNonEmptyType", TestFlags)
bool FMTSPoolLayoutRejectedRemovalTest::RunTest(const FString& Parameters)
{
	// The only copper column is changed to iron while copper stock remains.
	const TArray<FMTSStack> Stock = { FMTSStack(Copper(), 1), FMTSStack(Iron(), 40) };
	const FMTSPoolLayout NewLayout = FMTSPoolLayout::Build({ Iron(), Iron() }, 12);

	FItemClass FailedItem;
	TestEqual(TEXT("fit result"), NewLayout.CheckFit(Stock, StackSize, &FailedItem), EMTSFitResult::TypeHasNoPool);
	TestTrue(TEXT("failed item is copper"), FailedItem == Copper());

	// Unsetting the copper column instead is rejected the same way.
	const FMTSPoolLayout UnsetLayout = FMTSPoolLayout::Build({ Iron(), nullptr }, 12);
	TestEqual(TEXT("unset fit result"), UnsetLayout.CheckFit(Stock, StackSize), EMTSFitResult::TypeHasNoPool);

	// Removing a type without stock is allowed.
	const TArray<FMTSStack> IronOnly = { FMTSStack(Iron(), 40) };
	TestEqual(TEXT("removing empty type"), NewLayout.CheckFit(IronOnly, StackSize), EMTSFitResult::Fits);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSPoolLayoutRedistributionTest, "ModularTruckStation.Storage.FitCheck.NoItemCountChange", TestFlags)
bool FMTSPoolLayoutRedistributionTest::RunTest(const FString& Parameters)
{
	// Current inventory as it could look before a filter change: partial stacks, empty slots, mixed order.
	const TArray<FMTSStack> Stock = {
		FMTSStack(Copper(), 30),
		FMTSStack(),
		FMTSStack(Iron(), 100),
		FMTSStack(Iron(), 75),
		FMTSStack(),
		FMTSStack(Copper(), 99),
		FMTSStack(Copper(), 1),
		FMTSStack(Iron(), 55),
	};
	const FMTSPoolLayout NewLayout = FMTSPoolLayout::Build({ Iron(), Concrete(), Copper(), Iron() }, 3);

	TArray<FMTSStack> Slots;
	if (!TestEqual(TEXT("plan result"), NewLayout.PlanRedistribution(Stock, StackSize, Slots), EMTSFitResult::Fits))
	{
		return false;
	}

	TestEqual(TEXT("one entry per inventory slot"), Slots.Num(), NewLayout.GetInventorySize());
	TestEqual(TEXT("iron count unchanged"), CountItems(Slots, Iron()), 230);
	TestEqual(TEXT("copper count unchanged"), CountItems(Slots, Copper()), 130);
	TestEqual(TEXT("concrete count unchanged"), CountItems(Slots, Concrete()), 0);

	for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
	{
		const FMTSStack& Stack = Slots[Slot];
		if (!Stack.HasItems())
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("slot %d holds its pool's type"), Slot), NewLayout.IsItemAllowedOnSlot(Stack.ItemClass, Slot));
		TestTrue(FString::Printf(TEXT("slot %d within stack size"), Slot), Stack.NumItems <= StackSize(Stack.ItemClass));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSPoolLayoutStatefulStacksTest, "ModularTruckStation.Storage.FitCheck.StatefulStacksNotMerged", TestFlags)
bool FMTSPoolLayoutStatefulStacksTest::RunTest(const FString& Parameters)
{
	// Two stacks with item state need a slot each; the stateless items merge into one slot.
	const TArray<FMTSStack> Stock = {
		FMTSStack(Iron(), 1, true, 4),
		FMTSStack(Iron(), 20),
		FMTSStack(Iron(), 1, true, 7),
		FMTSStack(Iron(), 30),
	};
	TestEqual(TEXT("required slots"), FMTSPoolLayout::CountRequiredSlots(Stock, StackSize).FindRef(Iron()), 3);
	TestEqual(TEXT("two slots too few"), FMTSPoolLayout::Build({ Iron() }, 2).CheckFit(Stock, StackSize), EMTSFitResult::PoolTooSmall);

	TArray<FMTSStack> Slots;
	if (!TestEqual(TEXT("plan result"), FMTSPoolLayout::Build({ Iron() }, 3).PlanRedistribution(Stock, StackSize, Slots), EMTSFitResult::Fits))
	{
		return false;
	}

	TArray<int32> StatefulSources;
	for (const FMTSStack& Stack : Slots)
	{
		if (Stack.bHasState)
		{
			StatefulSources.Add(Stack.SourceIndex);
		}
		else if (Stack.HasItems())
		{
			TestEqual(TEXT("merged stack has no source"), Stack.SourceIndex, static_cast<int32>(INDEX_NONE));
		}
	}
	TestEqual(TEXT("stateful stacks kept apart"), StatefulSources, TArray<int32>({ 4, 7 }));
	TestEqual(TEXT("item count unchanged"), CountItems(Slots, Iron()), 52);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
