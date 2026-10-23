#include "Module/MTSStationModule.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "FGFactoryConnectionComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ModularTruckStation.h"
#include "Net/UnrealNetwork.h"
#include "Resources/FGAnyUndefinedDescriptor.h"
#include "Resources/FGItemDescriptor.h"
#include "Resources/FGWildCardDescriptor.h"
#include "Station/MTSStationBase.h"
#include "Storage/MTSPoolLayout.h"

namespace
{
	/** Max items an input port takes from its belt per factory tick. */
	constexpr int32 MaxGrabsPerTick = 4;

	const TCHAR* PortNamePrefix = TEXT("Port_");
}

AMTSStationModule::AMTSStationModule()
{
}

void AMTSStationModule::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMTSStationModule, mColumnFilters);
	DOREPLIFETIME(AMTSStationModule, mPortDirections);
	DOREPLIFETIME(AMTSStationModule, mStationBase);
}

void AMTSStationModule::BeginPlay()
{
	// New modules start with unset filters and input ports; loaded ones keep their saved settings.
	mColumnFilters.SetNum(mNumColumns);
	mPortDirections.SetNum(GetNumSlots());

	// Directions are applied before the factory tick starts, so loaded belts on output ports keep working.
	GatherSlotConnections();
	ApplyPortDirections();

	Super::BeginPlay();

	if (HasAuthority() && mStationBase)
	{
		mStationBase->AttachModule(this);
	}
	CreateDisplayMaterials();
	UpdateColumnDisplays();
	OnColumnFiltersChanged();
	OnPortDirectionsChanged();
}

void AMTSStationModule::GetDismantleInventoryReturns(TArray<FInventoryStack>& out_returns) const
{
	Super::GetDismantleInventoryReturns(out_returns);

	// The module refunds the station's stock (design D9); the base then only refunds its fuel.
	if (mStationBase && mStationBase->GetAttachedModule() == this)
	{
		mStationBase->GetStorageStacks(out_returns);
	}
}

void AMTSStationModule::Dismantle_Implementation()
{
	if (HasAuthority() && mStationBase)
	{
		mStationBase->DetachModule(this);
	}
	Super::Dismantle_Implementation();
}

void AMTSStationModule::Factory_CollectInput_Implementation()
{
	AMTSStationBase* StationBase = mStationBase;
	if (!StationBase)
	{
		return;
	}

	for (int32 Slot = 0; Slot < mSlotConnections.Num(); ++Slot)
	{
		UFGFactoryConnectionComponent* Connection = mSlotConnections[Slot];
		const TSubclassOf<UFGItemDescriptor> Filter = GetColumnFilter(Slot / NumRows);
		if (!Connection || !Filter || GetPortDirection(Slot) != EMTSPortDirection::Input || !Connection->IsConnected())
		{
			continue;
		}

		// Grab only the column's type and only while its pool has room, so items are never taken and lost.
		for (int32 Grab = 0; Grab < MaxGrabsPerTick && StationBase->HasSpaceInPool(Filter); ++Grab)
		{
			FInventoryItem Item;
			float OffsetBeyond = 0.f;
			if (!Connection->Factory_GrabOutput(Item, OffsetBeyond, Filter))
			{
				break;
			}
			if (!StationBase->AddToPool(Item))
			{
				UE_LOG(LogModularTruckStation, Error, TEXT("%s: grabbed %s but its pool was full; item lost"), *GetName(), *GetNameSafe(Item.GetItemClass()));
				break;
			}
		}
	}
}

bool AMTSStationModule::Factory_PeekOutput_Implementation(const UFGFactoryConnectionComponent* connection, TArray<FInventoryItem>& out_items, TSubclassOf<UFGItemDescriptor> type) const
{
	const TSubclassOf<UFGItemDescriptor> OutputType = GetOutputType(connection, type);
	FInventoryItem Item;
	if (!OutputType || !mStationBase->PeekPool(OutputType, Item))
	{
		return false;
	}
	out_items.Add(Item);
	return true;
}

bool AMTSStationModule::Factory_GrabOutput_Implementation(UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf<UFGItemDescriptor> type)
{
	const TSubclassOf<UFGItemDescriptor> OutputType = GetOutputType(connection, type);
	if (!OutputType || !mStationBase->TakeFromPool(OutputType, out_item))
	{
		return false;
	}
	out_OffsetBeyond = 0.f;
	return true;
}

TSubclassOf<UFGItemDescriptor> AMTSStationModule::GetColumnFilter(int32 Column) const
{
	return mColumnFilters.IsValidIndex(Column) ? mColumnFilters[Column] : nullptr;
}

EMTSPortDirection AMTSStationModule::GetPortDirection(int32 Slot) const
{
	return mPortDirections.IsValidIndex(Slot) ? mPortDirections[Slot] : EMTSPortDirection::Input;
}

bool AMTSStationModule::IsSlotConnected(int32 Slot) const
{
	const UFGFactoryConnectionComponent* Connection = GetSlotConnection(Slot);
	return Connection && Connection->IsConnected();
}

UFGFactoryConnectionComponent* AMTSStationModule::GetSlotConnection(int32 Slot) const
{
	return mSlotConnections.IsValidIndex(Slot) ? mSlotConnections[Slot] : nullptr;
}

void AMTSStationModule::SetStationBase(AMTSStationBase* StationBase)
{
	mStationBase = StationBase;
}

EMTSFilterChangeResult AMTSStationModule::SetColumnFilter(int32 Column, TSubclassOf<UFGItemDescriptor> ItemClass, TSubclassOf<UFGItemDescriptor>* OutFailedItem)
{
	if (!mColumnFilters.IsValidIndex(Column))
	{
		return EMTSFilterChangeResult::InvalidColumn;
	}
	if (ItemClass && !CanFilterItem(ItemClass))
	{
		return EMTSFilterChangeResult::InvalidItem;
	}

	TArray<TSubclassOf<UFGItemDescriptor>> NewFilters = mColumnFilters;
	NewFilters[Column] = ItemClass;
	return ApplyColumnFilters(MoveTemp(NewFilters), OutFailedItem);
}

EMTSFilterChangeResult AMTSStationModule::SetAllColumnFilters(TSubclassOf<UFGItemDescriptor> ItemClass, TSubclassOf<UFGItemDescriptor>* OutFailedItem)
{
	if (ItemClass && !CanFilterItem(ItemClass))
	{
		return EMTSFilterChangeResult::InvalidItem;
	}

	TArray<TSubclassOf<UFGItemDescriptor>> NewFilters;
	NewFilters.Init(ItemClass, mNumColumns);
	return ApplyColumnFilters(MoveTemp(NewFilters), OutFailedItem);
}

bool AMTSStationModule::HasUniformColumnFilters() const
{
	return !mColumnFilters.ContainsByPredicate([this](const TSubclassOf<UFGItemDescriptor>& Filter) { return Filter != mColumnFilters[0]; });
}

EMTSFilterChangeResult AMTSStationModule::ApplyColumnFilters(TArray<TSubclassOf<UFGItemDescriptor>> NewFilters, TSubclassOf<UFGItemDescriptor>* OutFailedItem)
{
	if (!mStationBase)
	{
		return EMTSFilterChangeResult::NoStationBase;
	}

	// The base checks the fit and only rearranges the stock when it fits.
	switch (mStationBase->ApplyLayout(FMTSPoolLayout::Build(NewFilters, mSlotsPerColumn), OutFailedItem))
	{
	case EMTSFitResult::PoolTooSmall:
		return EMTSFilterChangeResult::PoolTooSmall;
	case EMTSFitResult::TypeHasNoPool:
		return EMTSFilterChangeResult::TypeHasNoPool;
	default:
		break;
	}

	mColumnFilters = MoveTemp(NewFilters);
	OnRep_ColumnFilters();
	return EMTSFilterChangeResult::Applied;
}

EMTSPortToggleResult AMTSStationModule::TogglePortDirection(int32 Slot)
{
	if (!mPortDirections.IsValidIndex(Slot))
	{
		return EMTSPortToggleResult::InvalidSlot;
	}
	return SetPortDirection(Slot, mPortDirections[Slot] == EMTSPortDirection::Input ? EMTSPortDirection::Output : EMTSPortDirection::Input);
}

EMTSPortToggleResult AMTSStationModule::SetPortDirection(int32 Slot, EMTSPortDirection Direction)
{
	if (!mPortDirections.IsValidIndex(Slot))
	{
		return EMTSPortToggleResult::InvalidSlot;
	}
	if (mPortDirections[Slot] == Direction)
	{
		return EMTSPortToggleResult::Applied;
	}
	if (IsSlotConnected(Slot))
	{
		return EMTSPortToggleResult::BeltConnected;
	}

	mPortDirections[Slot] = Direction;
	OnRep_PortDirections();
	return EMTSPortToggleResult::Applied;
}

bool AMTSStationModule::CanFilterItem(TSubclassOf<UFGItemDescriptor> ItemClass) const
{
	return ItemClass != nullptr;
}

void AMTSStationModule::OnRep_ColumnFilters()
{
	UpdateColumnDisplays();
	OnColumnFiltersChanged();
}

void AMTSStationModule::CreateDisplayMaterials()
{
	mDisplayMaterials.Reset();
	mDisplayMaterials.SetNum(mNumColumns);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (!mDisplayMaterial)
	{
		UE_LOG(LogModularTruckStation, Warning, TEXT("%s: no display material configured"), *GetClass()->GetName());
		return;
	}

	TArray<UStaticMeshComponent*> Meshes;
	GetComponents<UStaticMeshComponent>(Meshes);
	for (int32 Column = 0; Column < mNumColumns; ++Column)
	{
		const FString PlaceholderName = FString::Printf(TEXT("Display_%d"), Column);
		UStaticMeshComponent* const* Placeholder = Meshes.FindByPredicate([&PlaceholderName](const UStaticMeshComponent* Mesh) { return Mesh->GetName() == PlaceholderName; });
		if (!Placeholder)
		{
			UE_LOG(LogModularTruckStation, Warning, TEXT("%s: no display component %s"), *GetClass()->GetName(), *PlaceholderName);
			continue;
		}

		// The game takes over the rendering of a building BP's mesh components, so the BP display only marks the
		// place: it is hidden and replaced by a display created here, whose material stays under our control.
		UStaticMeshComponent* Display = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("MTSDisplay_%d"), Column));
		Display->SetStaticMesh((*Placeholder)->GetStaticMesh());
		Display->SetupAttachment((*Placeholder)->GetAttachParent());
		Display->SetRelativeTransform((*Placeholder)->GetRelativeTransform());
		// Slightly larger, so it covers the placeholder in case the game keeps drawing that one.
		Display->SetRelativeScale3D((*Placeholder)->GetRelativeScale3D() * 1.02);
		Display->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Display->SetCanEverAffectNavigation(false);
		AddInstanceComponent(Display);
		Display->RegisterComponent();
		(*Placeholder)->SetVisibility(false);

		mDisplayMaterials[Column] = Display->CreateDynamicMaterialInstance(0, mDisplayMaterial);
		UE_LOG(LogModularTruckStation, Log, TEXT("%s: display %d created over %s (placeholder material %s)"), *GetName(), Column, *PlaceholderName, *GetNameSafe((*Placeholder)->GetMaterial(0)));
	}
}

void AMTSStationModule::UpdateColumnDisplays()
{
	for (int32 Column = 0; Column < mDisplayMaterials.Num(); ++Column)
	{
		if (UMaterialInstanceDynamic* Material = mDisplayMaterials[Column])
		{
			const TSubclassOf<UFGItemDescriptor> Filter = GetColumnFilter(Column);
			UTexture2D* Icon = Filter ? UFGItemDescriptor::GetBigIcon(Filter) : mUnsetDisplayIcon.Get();
			if (Icon)
			{
				// Stream in the full resolution right away instead of showing the low mips first.
				Icon->SetForceMipLevelsToBeResident(5.f);
			}
			Material->SetTextureParameterValue(mDisplayIconParameter, Icon);
			UE_LOG(LogModularTruckStation, Verbose, TEXT("%s: display %d icon %s"), *GetName(), Column, *GetNameSafe(Icon));
		}
	}
}

void AMTSStationModule::OnRep_PortDirections()
{
	ApplyPortDirections();
	OnPortDirectionsChanged();
}

void AMTSStationModule::GatherSlotConnections()
{
	mSlotConnections.Reset();
	mSlotConnections.SetNum(GetNumSlots());

	TArray<UFGFactoryConnectionComponent*> Connections;
	GetComponents<UFGFactoryConnectionComponent>(Connections);
	for (UFGFactoryConnectionComponent* Connection : Connections)
	{
		// "Port_<column>_<row>"
		FString Column;
		FString Row;
		const FString Name = Connection->GetName();
		if (!Name.StartsWith(PortNamePrefix) || !Name.RightChop(FCString::Strlen(PortNamePrefix)).Split(TEXT("_"), &Column, &Row))
		{
			continue;
		}

		const int32 Slot = FCString::Atoi(*Column) * NumRows + FCString::Atoi(*Row);
		if (mSlotConnections.IsValidIndex(Slot) && Column.IsNumeric() && Row.IsNumeric())
		{
			mSlotConnections[Slot] = Connection;
			// Belts only call this module's Factory_PeekOutput / Factory_GrabOutput when the connection forwards to it.
			Connection->SetForwardPeekAndGrabToBuildable(true);
		}
		else
		{
			UE_LOG(LogModularTruckStation, Warning, TEXT("%s: connection %s does not match a slot of a %d-column module"), *GetClass()->GetName(), *Name, mNumColumns);
		}
	}

	for (int32 Slot = 0; Slot < mSlotConnections.Num(); ++Slot)
	{
		if (!mSlotConnections[Slot])
		{
			UE_LOG(LogModularTruckStation, Warning, TEXT("%s: no connection component Port_%d_%d"), *GetClass()->GetName(), Slot / NumRows, Slot % NumRows);
		}
	}
}

void AMTSStationModule::ApplyPortDirections()
{
	for (int32 Slot = 0; Slot < mSlotConnections.Num(); ++Slot)
	{
		if (UFGFactoryConnectionComponent* Connection = mSlotConnections[Slot])
		{
			Connection->SetDirection(GetPortDirection(Slot) == EMTSPortDirection::Output ? EFactoryConnectionDirection::FCD_OUTPUT : EFactoryConnectionDirection::FCD_INPUT);
		}
	}
}

int32 AMTSStationModule::FindSlot(const UFGFactoryConnectionComponent* Connection) const
{
	return Connection ? mSlotConnections.IndexOfByKey(Connection) : INDEX_NONE;
}

TSubclassOf<UFGItemDescriptor> AMTSStationModule::GetOutputType(const UFGFactoryConnectionComponent* Connection, TSubclassOf<UFGItemDescriptor> RequestedType) const
{
	const int32 Slot = FindSlot(Connection);
	if (!mStationBase || Slot == INDEX_NONE || GetPortDirection(Slot) != EMTSPortDirection::Output)
	{
		return nullptr;
	}

	const TSubclassOf<UFGItemDescriptor> Filter = GetColumnFilter(Slot / NumRows);
	const bool bAnyType = !RequestedType || RequestedType->IsChildOf(UFGWildCardDescriptor::StaticClass()) || RequestedType->IsChildOf(UFGAnyUndefinedDescriptor::StaticClass());
	return Filter && (bAnyType || RequestedType == Filter) ? Filter : nullptr;
}
