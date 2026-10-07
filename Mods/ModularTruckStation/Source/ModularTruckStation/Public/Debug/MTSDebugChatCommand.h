#pragma once

#include "CoreMinimal.h"
#include "Command/ChatCommandInstance.h"
#include "MTSDebugChatCommand.generated.h"

/**
 * Debug chat command for the vanilla behavior spikes. Acts on the modular station base nearest to the player.
 *   /mts layout <slots per column> <item|-> [<item|-> ...]	apply a pool layout to a base without module; stock is kept
 *   /mts filter <column> <item|->						set or unset a module column filter (storage rules apply)
 *   /mts port <slot>										toggle a module slot port (slot = column * 2 + row)
 *   /mts dump												print module, layout and storage contents (also to the log)
 *   /mts clear												empty the station storage
 */
UCLASS()
class MODULARTRUCKSTATION_API AMTSDebugChatCommand : public AChatCommandInstance
{
	GENERATED_BODY()
public:
	AMTSDebugChatCommand();

	virtual EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
