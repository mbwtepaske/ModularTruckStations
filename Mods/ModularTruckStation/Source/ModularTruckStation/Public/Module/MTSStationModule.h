#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildableFactory.h"
#include "Station/MTSStationTypes.h"
#include "MTSStationModule.generated.h"

class AMTSStationBase;
class UFGFactoryConnectionComponent;
class UFGItemDescriptor;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTexture2D;

/**
 * Storage module of a modular station: a matrix of columns x 2 slot ports. Each column has a storage type filter;
 * each slot holds one belt port that is an input or an output. The module has no storage of its own; its columns
 * define the pool layout of the base's storage, and its ports feed and drain the base's pools.
 *
 * Slot ports are the BP's factory connection components named "Port_<column>_<row>" (row 0 = bottom). Column
 * displays are placed by static mesh components named "Display_<column>"; at runtime they are replaced by display
 * meshes using mDisplayMaterial, whose texture parameter mDisplayIconParameter shows the filter's icon.
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

	UFUNCTION(BlueprintPure, Category = "Modular Station")
	EMTSStationType GetStationType() const { return mStationType; }

	UFUNCTION(BlueprintPure, Category = "Modular Station")
	int32 GetNumColumns() const { return mNumColumns; }

	/** Slot index = column * NumRows + row. */
	UFUNCTION(BlueprintPure, Category = "Modular Station")
	int32 GetNumSlots() const { return mNumColumns * NumRows; }

	UFUNCTION(BlueprintPure, Category = "Modular Station")
	int32 GetSlotsPerColumn() const { return mSlotsPerColumn; }

	FORCEINLINE float GetPowerPerColumn() const { return mPowerPerColumn; }
	FORCEINLINE const TArray<TSubclassOf<UFGItemDescriptor>>& GetColumnFilters() const { return mColumnFilters; }

	UFUNCTION(BlueprintPure, Category = "Modular Station")
	AMTSStationBase* GetStationBase() const { return mStationBase; }

	/** @return the column's filter, or null if unset. */
	UFUNCTION(BlueprintPure, Category = "Modular Station")
	TSubclassOf<UFGItemDescriptor> GetColumnFilter(int32 Column) const;

	UFUNCTION(BlueprintPure, Category = "Modular Station")
	EMTSPortDirection GetPortDirection(int32 Slot) const;

	/** @return true if a belt is connected to the slot's port; such a port cannot be toggled. */
	UFUNCTION(BlueprintPure, Category = "Modular Station")
	bool IsSlotConnected(int32 Slot) const;

	/** @return the slot's connection component, or null if the BP has no "Port_<column>_<row>" for it. */
	UFGFactoryConnectionComponent* GetSlotConnection(int32 Slot) const;

	/** Sets the base this module is built on. Called by the hologram before BeginPlay; the module registers itself there. */
	void SetStationBase(AMTSStationBase* StationBase);

	/** @return true if the gap between column Gap and Gap + 1 is linked; linked columns always share one filter. */
	UFUNCTION(BlueprintPure, Category = "Modular Station")
	bool IsColumnLinked(int32 Gap) const;

	/**
	 * Server only. Sets the filter (null = unset) of the column's whole linked group, if the base's stock still fits
	 * the resulting pools; the change applies entirely or not at all.
	 * @param OutFailedItem	the item type that blocked the change, if any.
	 */
	EMTSFilterChangeResult SetColumnFilter(int32 Column, TSubclassOf<UFGItemDescriptor> ItemClass, TSubclassOf<UFGItemDescriptor>* OutFailedItem = nullptr);

	/**
	 * Server only. Links or unlinks column Gap and Gap + 1. Unlinking keeps both filters and always applies. Linking
	 * gives both groups one filter (the left group's, or the right group's when the left one is unset) and is
	 * rejected, keeping the link off, when the stock would not fit.
	 */
	EMTSFilterChangeResult SetColumnLink(int32 Gap, bool bLinked, TSubclassOf<UFGItemDescriptor>* OutFailedItem = nullptr);

	/** Server only. Switches a slot port between input and output, if no belt is connected to it. */
	EMTSPortToggleResult TogglePortDirection(int32 Slot);

	/**
	 * Server only. Sets a slot port's direction, if no belt is connected to it. Setting the current direction is
	 * applied without change, also while a belt is connected.
	 */
	EMTSPortToggleResult SetPortDirection(int32 Slot, EMTSPortDirection Direction);

	/** @return true if the item type can be a column filter of this module type. */
	UFUNCTION(BlueprintPure, Category = "Modular Station")
	virtual bool CanFilterItem(TSubclassOf<UFGItemDescriptor> ItemClass) const;

protected:
	/** Called on server and clients when column filters change, after the column displays are updated. */
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

	/** Link per gap between neighbouring columns (index = left column); new modules start fully linked. */
	UPROPERTY(SaveGame, Replicated)
	TArray<bool> mColumnLinks;

	/** Port direction per slot, index = column * NumRows + row. Saved here because connection directions are not saved. */
	UPROPERTY(SaveGame, ReplicatedUsing = OnRep_PortDirections)
	TArray<EMTSPortDirection> mPortDirections;

	UPROPERTY(SaveGame, Replicated)
	TObjectPtr<AMTSStationBase> mStationBase;

	/** Save data version of this class, for later migrations. 2: column links. */
	UPROPERTY(SaveGame)
	int32 mSaveVersion = 2;

	/** Material of the column displays, set on each "Display_<column>" component at runtime. */
	UPROPERTY(EditDefaultsOnly, Category = "Modular Station|Display")
	TObjectPtr<UMaterialInterface> mDisplayMaterial;

	/** Texture parameter of the column display material that receives the filter's icon. */
	UPROPERTY(EditDefaultsOnly, Category = "Modular Station|Display")
	FName mDisplayIconParameter = TEXT("Icon");

	/** Icon shown on the display of a column without filter. */
	UPROPERTY(EditDefaultsOnly, Category = "Modular Station|Display")
	TObjectPtr<UTexture2D> mUnsetDisplayIcon;

private:
	/**
	 * Hides each "Display_<column>" placeholder of the BP and creates a display mesh in its place, with a dynamic
	 * material from mDisplayMaterial.
	 */
	void CreateDisplayMaterials();

	/** Shows each column filter's icon (or the unset icon) on its display. */
	void UpdateColumnDisplays();

	/** Dynamic display material per column; null where the BP has no display. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> mDisplayMaterials;

	/** Applies new filters if the base's stock fits the resulting pools. */
	EMTSFilterChangeResult ApplyColumnFilters(TArray<TSubclassOf<UFGItemDescriptor>> NewFilters, TSubclassOf<UFGItemDescriptor>* OutFailedItem);

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
