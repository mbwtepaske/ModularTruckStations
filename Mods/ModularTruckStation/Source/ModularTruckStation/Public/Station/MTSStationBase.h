#pragma once

#include "CoreMinimal.h"
#include "Buildables/FGBuildableDockingStation.h"
#include "Station/MTSStationTypes.h"
#include "Storage/MTSPoolLayout.h"
#include "MTSStationBase.generated.h"

class AMTSStationModule;

/**
 * Modular station base: a truck docking station without cargo storage of its own. It accepts one module of the same
 * station type; the module's column filters define the pool layout of the base's storage inventory.
 *
 * The storage inventory is the vanilla mInventory, so vanilla truck transfer, UI and saving keep working on it.
 * It always keeps at least one slot (vanilla asserts on an empty inventory); slots outside the layout accept nothing.
 */
UCLASS(Abstract)
class MODULARTRUCKSTATION_API AMTSStationBase : public AFGBuildableDockingStation
{
	GENERATED_BODY()
public:
	AMTSStationBase();

	// Begin AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	// End AActor interface

	// Begin AFGBuildable interface
	virtual void GetDismantleInventoryReturns(TArray<FInventoryStack>& out_returns) const override;
	virtual void GetChildDismantleActors_Implementation(TArray<AActor*>& out_ChildDismantleActors) const override;
	// End AFGBuildable interface

	// Begin AFGBuildableFactory interface
	/** Vanilla base consumption plus the attached module's per-column consumption. */
	virtual float GetProducingPowerConsumptionBase() const override;
	// End AFGBuildableFactory interface

	FORCEINLINE EMTSStationType GetStationType() const { return mStationType; }
	FORCEINLINE AMTSStationModule* GetAttachedModule() const { return mAttachedModule; }

	/** @return true if the module has this base's station type and the base has no other module. */
	bool CanAcceptModule(const AMTSStationModule* Module) const;

	/** World transform where a module snaps to: the component tagged mModuleAttachPointTag, else the actor transform. */
	FTransform GetModuleAttachTransform() const;

	/** Server only. Links the module and applies its layout. Calling it again for the same module rebuilds the layout. */
	void AttachModule(AMTSStationModule* Module);

	/** Server only. Unlinks the module and drops the storage to the hidden slot; the module has refunded the stock. */
	void DetachModule(AMTSStationModule* Module);

	/** Server only. Applies the layout of the attached module's current filters (empty without a module). */
	EMTSFitResult RebuildLayout(TSubclassOf<UFGItemDescriptor>* OutFailedItem = nullptr);

	/** Checks whether the current stock fits a layout, without changing anything. */
	EMTSFitResult CheckLayout(const FMTSPoolLayout& Layout, TSubclassOf<UFGItemDescriptor>* OutFailedItem = nullptr) const;

	/**
	 * Server only. Resizes the storage to the layout, reserves each slot for its pool's type and moves the stock into
	 * its pools. Nothing changes when the stock does not fit; items are never destroyed.
	 */
	EMTSFitResult ApplyLayout(const FMTSPoolLayout& NewLayout, TSubclassOf<UFGItemDescriptor>* OutFailedItem = nullptr);

	FORCEINLINE const FMTSPoolLayout& GetLayout() const { return mLayout; }

	/** @return true if the pool of this item type can take one more item. */
	bool HasSpaceInPool(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	/** Adds one item to its type's pool. @return false if there is no pool for it or the pool is full. */
	bool AddToPool(const FInventoryItem& Item);

	/** Gets the next item of the type's pool without removing it. @return false if the pool is empty. */
	bool PeekPool(TSubclassOf<UFGItemDescriptor> ItemClass, FInventoryItem& OutItem) const;

	/** Removes one item from the type's pool. @return false if the pool is empty. */
	bool TakeFromPool(TSubclassOf<UFGItemDescriptor> ItemClass, FInventoryItem& OutItem);

	/** All non-empty stacks of the storage. */
	void GetStorageStacks(TArray<FInventoryStack>& OutStacks) const;

protected:
	/** Fixed per class; set by the type-specific subclass. */
	UPROPERTY(VisibleDefaultsOnly, Category = "Modular Station")
	EMTSStationType mStationType = EMTSStationType::Cargo;

	/** Tag of the scene component in the BP that marks where a module snaps to. */
	UPROPERTY(EditDefaultsOnly, Category = "Modular Station")
	FName mModuleAttachPointTag = TEXT("MTSModuleAttach");

private:
	/** Storage inventory item filter: an item is only allowed on the slots of its pool. */
	bool FilterStorageItem(TSubclassOf<UObject> ItemClass, int32 Index) const;

	/** Index of a slot in the pool that can take one more item, or INDEX_NONE. Caller holds mStorageLock. */
	int32 FindSlotWithSpace_Locked(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	/** Index of a non-empty slot in the pool, or INDEX_NONE. Caller holds mStorageLock. */
	int32 FindSlotWithItems_Locked(TSubclassOf<UFGItemDescriptor> ItemClass) const;

	/** Current stock as pool-logic stacks, with source slot indices. */
	TArray<FMTSStack> GatherStock_Locked(TArray<FInventoryStack>& OutSlots) const;

	UPROPERTY(SaveGame, Replicated)
	TObjectPtr<AMTSStationModule> mAttachedModule;

	/** Save data version of this class, for later migrations. */
	UPROPERTY(SaveGame)
	int32 mSaveVersion = 1;

	/** Recomputed from the module's filters after build, attach, detach and load; not saved. */
	FMTSPoolLayout mLayout;

	/** Guards storage access from the base's, the module's and conveyors' factory ticks. */
	mutable FCriticalSection mStorageLock;
};
