#include "UI/MTSBlueprintLibrary.h"

#include "FGRecipe.h"
#include "FGRecipeManager.h"
#include "Module/MTSStationModule.h"
#include "Resources/FGBuildDescriptor.h"
#include "Resources/FGItemDescriptor.h"

#define LOCTEXT_NAMESPACE "ModularTruckStation"

TArray<TSubclassOf<UFGItemDescriptor>> UMTSBlueprintLibrary::GetFilterableItems(const AMTSStationModule* Module)
{
	TArray<TSubclassOf<UFGItemDescriptor>> Items;
	const AFGRecipeManager* RecipeManager = Module ? AFGRecipeManager::Get(const_cast<AMTSStationModule*>(Module)) : nullptr;
	if (!RecipeManager)
	{
		return Items;
	}

	TArray<TSubclassOf<UFGRecipe>> Recipes;
	RecipeManager->GetAllAvailableRecipes(Recipes);

	TSet<TSubclassOf<UFGItemDescriptor>> Seen;
	auto AddItems = [&](const TArray<FItemAmount>& Amounts)
	{
		for (const FItemAmount& Amount : Amounts)
		{
			const TSubclassOf<UFGItemDescriptor> Item = Amount.ItemClass;
			if (Item && !Seen.Contains(Item) && !Item->IsChildOf(UFGBuildDescriptor::StaticClass()) && Module->CanFilterItem(Item))
			{
				Seen.Add(Item);
				Items.Add(Item);
			}
		}
	};
	for (const TSubclassOf<UFGRecipe>& Recipe : Recipes)
	{
		AddItems(UFGRecipe::GetIngredients(Module, Recipe));
		AddItems(UFGRecipe::GetProducts(Recipe));
	}

	Items.Sort([](const TSubclassOf<UFGItemDescriptor>& A, const TSubclassOf<UFGItemDescriptor>& B)
	{
		return UFGItemDescriptor::GetItemName(A).CompareTo(UFGItemDescriptor::GetItemName(B)) < 0;
	});
	return Items;
}

FText UMTSBlueprintLibrary::GetFilterChangeResultText(EMTSFilterChangeResult Result, TSubclassOf<UFGItemDescriptor> FailedItem)
{
	const FText ItemName = FailedItem ? UFGItemDescriptor::GetItemName(FailedItem) : LOCTEXT("SomeItem", "an item");
	switch (Result)
	{
	case EMTSFilterChangeResult::Applied:
		return FText::GetEmpty();
	case EMTSFilterChangeResult::PoolTooSmall:
		return FText::Format(LOCTEXT("PoolTooSmall", "The stored {0} would not fit. Remove some first."), ItemName);
	case EMTSFilterChangeResult::TypeHasNoPool:
		return FText::Format(LOCTEXT("TypeHasNoPool", "{0} is still stored. Remove it before changing its last column."), ItemName);
	case EMTSFilterChangeResult::InvalidItem:
		return LOCTEXT("InvalidItem", "This item cannot be stored in a cargo module.");
	case EMTSFilterChangeResult::NotInReach:
		return LOCTEXT("FilterNotInReach", "You are too far away from the module.");
	default:
		return LOCTEXT("FilterFailed", "The filter could not be changed.");
	}
}

FText UMTSBlueprintLibrary::GetPortToggleResultText(EMTSPortToggleResult Result)
{
	switch (Result)
	{
	case EMTSPortToggleResult::Applied:
		return FText::GetEmpty();
	case EMTSPortToggleResult::BeltConnected:
		return LOCTEXT("BeltConnected", "Remove the belt from this port first.");
	case EMTSPortToggleResult::NotInReach:
		return LOCTEXT("PortNotInReach", "You are too far away from the module.");
	default:
		return LOCTEXT("PortFailed", "The port could not be changed.");
	}
}

#undef LOCTEXT_NAMESPACE
