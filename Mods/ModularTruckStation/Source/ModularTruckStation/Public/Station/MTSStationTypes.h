#pragma once

#include "CoreMinimal.h"
#include "MTSStationTypes.generated.h"

/** Kind of cargo a modular station handles. A module only attaches to a base of the same type. */
UENUM(BlueprintType)
enum class EMTSStationType : uint8
{
	Cargo,
	Fluid
};

/** Direction of a module slot port. */
UENUM(BlueprintType)
enum class EMTSPortDirection : uint8
{
	Input,
	Output
};

/** Outcome of a column filter change request; shown to the player when rejected. */
UENUM(BlueprintType)
enum class EMTSFilterChangeResult : uint8
{
	Applied,
	InvalidColumn,
	/** The item cannot be stored by this station type, e.g. a fluid in a cargo module. */
	InvalidItem,
	NoStationBase,
	/** The requesting player is not near the module. */
	NotInReach,
	/** The stock of a type that keeps its pool would not fit the smaller pool. */
	PoolTooSmall,
	/** A type with stock would lose its last column. */
	TypeHasNoPool
};

/** Outcome of a port direction toggle request; shown to the player when rejected. */
UENUM(BlueprintType)
enum class EMTSPortToggleResult : uint8
{
	Applied,
	InvalidSlot,
	/** Ports can only be toggled while no belt is connected. */
	BeltConnected,
	/** The requesting player is not near the module. */
	NotInReach
};
