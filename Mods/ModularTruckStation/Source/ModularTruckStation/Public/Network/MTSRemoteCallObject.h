#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "Station/MTSStationTypes.h"
#include "MTSRemoteCallObject.generated.h"

class AMTSStationModule;
class UFGItemDescriptor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FMTSOnFilterChangeResult, AMTSStationModule*, Module, int32, Column, EMTSFilterChangeResult, Result, TSubclassOf<UFGItemDescriptor>, FailedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMTSOnPortToggleResult, AMTSStationModule*, Module, int32, Slot, EMTSPortToggleResult, Result);

/**
 * Player-owned channel for module UI actions. Clients only request; the server validates (player in reach plus the
 * module's own rules), applies, and reports the result back to the requesting player. Applied changes reach every
 * player through the module's replicated filters and port directions.
 */
UCLASS()
class MODULARTRUCKSTATION_API UMTSRemoteCallObject : public UFGRemoteCallObject
{
	GENERATED_BODY()
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** @return the RCO of the local player, or null. */
	UFUNCTION(BlueprintPure, Category = "Modular Station", meta = (WorldContext = "WorldContext"))
	static UMTSRemoteCallObject* GetLocal(const UObject* WorldContext);

	/** Requests a column filter change; null unsets the column. The result arrives in OnFilterChangeResult. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Modular Station")
	void Server_SetColumnFilter(AMTSStationModule* Module, int32 Column, TSubclassOf<UFGItemDescriptor> ItemClass);

	/**
	 * Requests the same filter for all columns (simple mode); null unsets them. Applies entirely or not at all. The
	 * result arrives in OnFilterChangeResult with Column = -1.
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Modular Station")
	void Server_SetAllColumnFilters(AMTSStationModule* Module, TSubclassOf<UFGItemDescriptor> ItemClass);

	/** Requests a port direction toggle. The result arrives in OnPortToggleResult. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Modular Station")
	void Server_TogglePort(AMTSStationModule* Module, int32 Slot);

	/** Requests a port direction; the current direction is read with the module's GetPortDirection. The result arrives in OnPortToggleResult. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Modular Station")
	void Server_SetPortDirection(AMTSStationModule* Module, int32 Slot, EMTSPortDirection Direction);

	/** Broadcast on the requesting player's machine when the server answered a filter change; Column -1 = all columns. */
	UPROPERTY(BlueprintAssignable, Category = "Modular Station")
	FMTSOnFilterChangeResult OnFilterChangeResult;

	/** Broadcast on the requesting player's machine when the server answered a port toggle. */
	UPROPERTY(BlueprintAssignable, Category = "Modular Station")
	FMTSOnPortToggleResult OnPortToggleResult;

private:
	UFUNCTION(Client, Reliable)
	void Client_FilterChangeResult(AMTSStationModule* Module, int32 Column, EMTSFilterChangeResult Result, TSubclassOf<UFGItemDescriptor> FailedItem);

	UFUNCTION(Client, Reliable)
	void Client_PortToggleResult(AMTSStationModule* Module, int32 Slot, EMTSPortToggleResult Result);

	/** @return true if the requesting player's pawn is near the module. */
	bool IsInReach(const AMTSStationModule* Module) const;

	/** An RCO needs at least one replicated property to be replicated. */
	UPROPERTY(Replicated)
	bool mForceNetField_MTSRemoteCallObject = false;
};
