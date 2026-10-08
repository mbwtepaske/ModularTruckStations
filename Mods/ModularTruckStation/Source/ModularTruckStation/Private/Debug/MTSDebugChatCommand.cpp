#include "Debug/MTSDebugChatCommand.h"

#include "Command/CommandSender.h"
#include "EngineUtils.h"
#include "FGFactoryConnectionComponent.h"
#include "FGInventoryComponent.h"
#include "Module/MTSStationModule.h"
#include "FGPlayerController.h"
#include "ModularTruckStation.h"
#include "Resources/FGItemDescriptor.h"
#include "Station/MTSStationBase.h"
#include "UObject/UObjectIterator.h"

namespace
{
	/** Finds a loaded item descriptor class by name, with or without the _C suffix (e.g. Desc_IronPlate). */
	TSubclassOf<UFGItemDescriptor> FindItemClass(const FString& Name)
	{
		const FString ClassName = Name.EndsWith(TEXT("_C")) ? Name : Name + TEXT("_C");
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->IsChildOf(UFGItemDescriptor::StaticClass()) && It->GetName().Equals(ClassName, ESearchCase::IgnoreCase))
			{
				return *It;
			}
		}
		return nullptr;
	}

	AMTSStationBase* FindNearestStation(UWorld* World, UCommandSender* Sender)
	{
		const APawn* Pawn = Sender->IsPlayerSender() && Sender->GetPlayer() ? Sender->GetPlayer()->GetPawn() : nullptr;
		AMTSStationBase* Nearest = nullptr;
		double NearestDistSq = TNumericLimits<double>::Max();
		for (TActorIterator<AMTSStationBase> It(World); It; ++It)
		{
			const double DistSq = Pawn ? FVector::DistSquared(Pawn->GetActorLocation(), It->GetActorLocation()) : 0.0;
			if (!Nearest || DistSq < NearestDistSq)
			{
				Nearest = *It;
				NearestDistSq = DistSq;
			}
		}
		return Nearest;
	}

	void Report(UCommandSender* Sender, const FString& Message)
	{
		UE_LOG(LogModularTruckStation, Display, TEXT("[mts] %s"), *Message);
		Sender->SendChatMessage(Message);
	}

	FString ItemName(TSubclassOf<UFGItemDescriptor> ItemClass)
	{
		return ItemClass ? ItemClass->GetName() : TEXT("-");
	}
}

AMTSDebugChatCommand::AMTSDebugChatCommand()
{
	CommandName = TEXT("mts");
	MinNumberOfArguments = 1;
	Usage = NSLOCTEXT("ModularTruckStation", "DebugCommandUsage", "/mts layout <slots per column> <item|-> [...] | /mts filter <column> <item|-> | /mts port <slot> | /mts dump | /mts clear");
}

EExecutionStatus AMTSDebugChatCommand::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label)
{
	AMTSStationBase* Station = FindNearestStation(GetWorld(), Sender);
	if (!Station)
	{
		Report(Sender, TEXT("No modular station base in the world."));
		return EExecutionStatus::UNCOMPLETED;
	}

	UFGInventoryComponent* Storage = Station->GetInventory();
	if (!Storage)
	{
		Report(Sender, FString::Printf(TEXT("%s has no storage inventory."), *Station->GetName()));
		return EExecutionStatus::UNCOMPLETED;
	}

	const FString& SubCommand = Arguments[0];

	if (SubCommand.Equals(TEXT("layout"), ESearchCase::IgnoreCase))
	{
		if (Arguments.Num() < 2 || !Arguments[1].IsNumeric())
		{
			return EExecutionStatus::BAD_ARGUMENTS;
		}

		TArray<TSubclassOf<UFGItemDescriptor>> ColumnFilters;
		for (int32 Index = 2; Index < Arguments.Num(); ++Index)
		{
			if (Arguments[Index] == TEXT("-"))
			{
				ColumnFilters.Add(nullptr);
				continue;
			}

			const TSubclassOf<UFGItemDescriptor> ItemClass = FindItemClass(Arguments[Index]);
			if (!ItemClass)
			{
				Report(Sender, FString::Printf(TEXT("Unknown item '%s'."), *Arguments[Index]));
				return EExecutionStatus::BAD_ARGUMENTS;
			}
			ColumnFilters.Add(ItemClass);
		}

		if (Station->GetAttachedModule())
		{
			Report(Sender, TEXT("Station has a module; its filters define the layout."));
			return EExecutionStatus::UNCOMPLETED;
		}

		const FMTSPoolLayout Layout = FMTSPoolLayout::Build(ColumnFilters, FCString::Atoi(*Arguments[1]));
		TSubclassOf<UFGItemDescriptor> FailedItem;
		const EMTSFitResult Result = Station->ApplyLayout(Layout, &FailedItem);
		if (Result != EMTSFitResult::Fits)
		{
			Report(Sender, FString::Printf(TEXT("Layout refused: stock of %s %s."), *ItemName(FailedItem),
				Result == EMTSFitResult::TypeHasNoPool ? TEXT("has no pool in the new layout") : TEXT("does not fit its smaller pool")));
			return EExecutionStatus::UNCOMPLETED;
		}
		Report(Sender, FString::Printf(TEXT("%s: layout applied, %d pools, storage size %d."), *Station->GetName(), Layout.GetPools().Num(), Storage->GetSizeLinear()));
		return EExecutionStatus::COMPLETED;
	}

	if (SubCommand.Equals(TEXT("dump"), ESearchCase::IgnoreCase))
	{
		Report(Sender, FString::Printf(TEXT("%s: module %s, storage size %d, load mode %d, fuel slots %d."), *Station->GetName(), *GetNameSafe(Station->GetAttachedModule()), Storage->GetSizeLinear(), Station->GetIsInLoadMode(), Station->GetFuelInventory() ? Station->GetFuelInventory()->GetSizeLinear() : -1));
		if (const AMTSStationModule* Module = Station->GetAttachedModule())
		{
			for (int32 Slot = 0; Slot < Module->GetNumSlots(); ++Slot)
			{
				const UFGFactoryConnectionComponent* Connection = Module->GetSlotConnection(Slot);
				Report(Sender, FString::Printf(TEXT("  slot %d (column %d): filter %s, %s, %s"), Slot, Slot / AMTSStationModule::NumRows,
					*ItemName(Module->GetColumnFilter(Slot / AMTSStationModule::NumRows)), *UEnum::GetValueAsString(Module->GetPortDirection(Slot)),
					!Connection ? TEXT("NO PORT COMPONENT") : Connection->IsConnected() ? TEXT("belt") : TEXT("free")));
			}
		}
		for (const FMTSPool& Pool : Station->GetLayout().GetPools())
		{
			Report(Sender, FString::Printf(TEXT("  pool %s: slots %d..%d, %d items"), *ItemName(Pool.ItemClass), Pool.FirstSlot, Pool.FirstSlot + Pool.NumSlots - 1, Storage->GetNumItems(Pool.ItemClass)));
		}
		for (int32 Slot = 0; Slot < Storage->GetSizeLinear(); ++Slot)
		{
			FInventoryStack Stack;
			if (Storage->GetStackFromIndex(Slot, Stack) && Stack.HasItems())
			{
				const TSubclassOf<UFGItemDescriptor> ItemClass = Stack.Item.GetItemClass();
				const bool bInPool = Station->GetLayout().IsItemAllowedOnSlot(ItemClass, Slot);
				Report(Sender, FString::Printf(TEXT("  slot %d: %d x %s%s"), Slot, Stack.NumItems, *ItemName(ItemClass), bInPool ? TEXT("") : TEXT("  <-- NOT ALLOWED HERE")));
			}
		}
		return EExecutionStatus::COMPLETED;
	}

	if (SubCommand.Equals(TEXT("filter"), ESearchCase::IgnoreCase) || SubCommand.Equals(TEXT("port"), ESearchCase::IgnoreCase))
	{
		AMTSStationModule* Module = Station->GetAttachedModule();
		if (!Module)
		{
			Report(Sender, TEXT("Station has no module."));
			return EExecutionStatus::UNCOMPLETED;
		}
		if (Arguments.Num() < 2 || !Arguments[1].IsNumeric())
		{
			return EExecutionStatus::BAD_ARGUMENTS;
		}
		const int32 Index = FCString::Atoi(*Arguments[1]);

		if (SubCommand.Equals(TEXT("port"), ESearchCase::IgnoreCase))
		{
			const EMTSPortToggleResult Result = Module->TogglePortDirection(Index);
			Report(Sender, FString::Printf(TEXT("port %d: %s"), Index, *UEnum::GetValueAsString(Result)));
			return Result == EMTSPortToggleResult::Applied ? EExecutionStatus::COMPLETED : EExecutionStatus::UNCOMPLETED;
		}

		TSubclassOf<UFGItemDescriptor> ItemClass;
		if (Arguments.Num() >= 3 && Arguments[2] != TEXT("-"))
		{
			ItemClass = FindItemClass(Arguments[2]);
			if (!ItemClass)
			{
				Report(Sender, FString::Printf(TEXT("Unknown item '%s'."), *Arguments[2]));
				return EExecutionStatus::BAD_ARGUMENTS;
			}
		}
		TSubclassOf<UFGItemDescriptor> FailedItem;
		const EMTSFilterChangeResult Result = Module->SetColumnFilter(Index, ItemClass, &FailedItem);
		Report(Sender, FString::Printf(TEXT("column %d filter %s: %s%s"), Index, *ItemName(ItemClass), *UEnum::GetValueAsString(Result),
			FailedItem ? *FString::Printf(TEXT(" (%s)"), *ItemName(FailedItem)) : TEXT("")));
		return Result == EMTSFilterChangeResult::Applied ? EExecutionStatus::COMPLETED : EExecutionStatus::UNCOMPLETED;
	}

	if (SubCommand.Equals(TEXT("clear"), ESearchCase::IgnoreCase))
	{
		Storage->Empty();
		Report(Sender, FString::Printf(TEXT("%s: storage emptied."), *Station->GetName()));
		return EExecutionStatus::COMPLETED;
	}

	return EExecutionStatus::BAD_ARGUMENTS;
}
