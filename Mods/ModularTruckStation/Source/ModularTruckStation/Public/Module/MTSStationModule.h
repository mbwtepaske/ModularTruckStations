#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildableFactory.h"
#include "Station/MTSStationTypes.h"
#include "MTSStationModule.generated.h"

class AMTSStationBase;
class UFGFactoryConnectionComponent;
class UFGItemDescriptor;

/**
 * Storage module of a modular station: a matrix of columns x 2 slot ports. Each column has a storage type filter;
 * each slot holds one belt port that is an input or an output. The module has no storage of its own; its columns
 * define the pool layout of the base's storage, and its ports feed and drain the base's pools.
 *
 * Slot ports are the BP's factory connection components named "Port_<column>_<row>" (row 0 = bottom).
 */
UCLASS(Abstract)
class MODULARTRUCKSTATION_API AMTSStationModule : public AFGBuildableFactory
{
	GENERATED_BODY()
public:
	/** Slots per column, stacked vertically. */
	static constexpr int32 NumRows = 2;

	AMTSStationModule();

	// Begin AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	// End AActor interface

	// Begin AFGBuildable interface
	virtual void GetDismantleInventoryReturns(TArray<FInventoryStack>& out_returns) const override;
	virtual void Dismantle_Implementation() override;
	virtual bool Factory_PeekOutput_Implementation(const UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const override;
	virtual bool Factory_GrabOutput_Implementation(UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type) override;
	// End AFGBuildable interface

	// Begin AFGBuildableFactory interface
	virtual void Factory_CollectInput_Implementation() override;
	// End AFGBuildableFactory interface

	FORCEINLINE EMTSStationType GetStationType() const { return mStationType; }
	FORCEINLINE int32 GetNumColumns() const { return mNumColumns; }
	FORCEINLINE int32 GetNumSlots() const { return mNumColumns * NumRows; }
	FORCEINLINE int32 GetSlotsPerColumn() const { return mSlotsPerColumn; }
	FORCEINLINE float GetPowerPerColumn() const { return mPowerPerColumn; }
	FORCEINLINE const TArray<TSubclassOf<UFGItemDescriptor>>& GetColumnFilters() const { return mColumnFilters; }
	FORCEINLINE AMTSStationBase* GetStationBase() const { return mStationBase; }

	TSubclassOf<UFGItemDescriptor> GetColumnFilter(int32 Column) const;
	EMTSPortDirection GetPortDirection(int32 Slot) const;

	/** @return the slot's connection component, or null if the BP has no "Port_<column>_<row>" for it. */
	UFGFactoryConnectionComponent* GetSlotConnection(int32 Slot) const;

	/** Sets the base this module is built on. Called by the hologram before BeginPlay; the module registers itself there. */
	void SetStationBase(AMTSStationBase* StationBase);

	/**
	 * Server only. Sets a column's filter (null = unset) if the base's stock still fits the resulting pools.
	 * @param OutFailedItem	the item type that blocked the change, if any.
	 */
	EMTSFilterChangeResult SetColumnFilter(int32 Column, TSubclassOf<UFGItemDescriptor> ItemClass, TSubclassOf<UFGItemDescriptor>* OutFailedItem = nullptr);

	/** Server only. Switches a slot port between input and output, if no belt is connected to it. */
	EMTSPortToggleResult TogglePortDirection(int32 Slot);

protected:
	/** Called on server and clients when column filters change; the BP updates the column displays (task 4.5). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Modular Station")
	void OnColumnFiltersChanged();

	/** Called on server and clients when port directions change; the BP updates the port visuals. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Modular Station")
	void OnPortDirectionsChanged();

	UFUNCTION()
	virtual void OnRep_ColumnFilters();

	UFUNCTION()
	virtual void OnRep_PortDirections();

	/** Fixed per class; set by the type-specific subclass. */
	UPROPERTY(VisibleDefaultsOnly, Category = "Modular Station")
	EMTSStationType mStationType = EMTSStationType::Cargo;

	/** Module size: S = 1, M = 2, L = 3, XL = 4. Set per size BP. */
	UPROPERTY(EditDefaultsOnly, Category = "Modular Station", meta = (ClampMin = 1, ClampMax = 4))
	int32 mNumColumns = 1;

	/** Capacity unit: storage slots each column with a filter adds to its pool. */
	UPROPERTY(EditDefaultsOnly, Category = "Modular Station", meta = (ClampMin = 1))
	int32 mSlotsPerColumn = 12;

	/** Power the base consumes extra per column of this module, in MW. */
	UPROPERTY(EditDefaultsOnly, Category = "Modular Station", meta = (ClampMin = 0))
	float mPowerPerColumn = 5.f;

	/** Storage type filter per column; null means unset (inactive column). */
	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_ColumnFilters)
	TArray<TSubclassOf<UFGItemDescriptor>> mColumnFilters;

	/** Port direction per slot, index = column * NumRows + row. Saved here because connection directions are not saved. */
	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_PortDirections)
	TArray<EMTSPortDirection> mPortDirections;

	UPROPERTY(SaveGame, Replicated)
	TObjectPtr<AMTSStationBase> mStationBase;

	/** Save data version of this class, for later migrations. */
	UPROPERTY(SaveGame)
	int32 mSaveVersion = 1;

private:
	/** Finds the "Port_<column>_<row>" connection components. */
	void GatherSlotConnections();

	/** Sets each slot connection's direction from mPortDirections. */
	void ApplyPortDirections();

	/** @return the slot index of the connection, or INDEX_NONE. */
	int32 FindSlot(const UFGFactoryConnectionComponent* Connection) const;

	/** @return the filter of an output slot's column if the requested type matches it, else null. */
	TSubclassOf<UFGItemDescriptor> GetOutputType(const UFGFactoryConnectionComponent* Connection, TSubclassOf<UFGItemDescriptor> RequestedType) const;

	/** Connection component per slot; null where the BP has none. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UFGFactoryConnectionComponent>> mSlotConnections;
};
