#include "UI/MTSItemPickerEntry.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Resources/FGItemDescriptor.h"
#include "UI/MTSItemPicker.h"

void UMTSItemPickerEntry::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (EntryButton)
	{
		EntryButton->OnClicked.AddDynamic(this, &UMTSItemPickerEntry::HandleClicked);
	}
}

void UMTSItemPickerEntry::SetItem(UMTSItemPicker* Picker, TSubclassOf<UFGItemDescriptor> Item)
{
	mPicker = Picker;
	mItem = Item;
	if (EntryName)
	{
		EntryName->SetText(Item ? UFGItemDescriptor::GetItemName(Item) : FText::GetEmpty());
	}
	if (EntryIcon)
	{
		EntryIcon->SetBrushFromTexture(Item ? UFGItemDescriptor::GetSmallIcon(Item) : nullptr);
	}
}

void UMTSItemPickerEntry::HandleClicked()
{
	if (UMTSItemPicker* Picker = mPicker.Get())
	{
		Picker->PickItem(mItem);
	}
}
