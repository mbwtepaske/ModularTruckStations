#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Station/MTSStationTypes.h"
#include "MTSBlueprintLibrary.generated.h"

class AMTSStationModule;
class UFGItemDescriptor;

/** Helpers for the module UI. */
UCLASS()
class MODULARTRUCKSTATION_API UMTSBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Items the player can choose as a column filter of the module: everything made or used by the available
	 * recipes that the module can store, without buildings and vehicles, sorted by name.
	 */
	UFUNCTION(BlueprintCallable, Category = "Modular Station|UI")
	static TArray<TSubclassOf<UFGItemDescriptor>> GetFilterableItems(const AMTSStationModule* Module);

	/** Player-facing message for a filter change result; empty when it was applied. */
	UFUNCTION(BlueprintPure, Category = "Modular Station|UI")
	static FText GetFilterChangeResultText(EMTSFilterChangeResult Result, TSubclassOf<UFGItemDescriptor> FailedItem);

	/** Player-facing message for a port toggle result; empty when it was applied. */
	UFUNCTION(BlueprintPure, Category = "Modular Station|UI")
	static FText GetPortToggleResultText(EMTSPortToggleResult Result);
};
