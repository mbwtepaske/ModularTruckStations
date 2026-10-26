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
	// A null item is the picker's "No filter" row.
	if (EntryName)
	{
		EntryName->SetText(Item ? UFGItemDescriptor::GetItemName(Item) : Picker ? Picker->GetNoFilterText() : FText::GetEmpty());
	}
	if (EntryIcon)
	{
		UTexture2D* Icon = Item ? UFGItemDescriptor::GetSmallIcon(Item) : Picker ? Picker->GetUnsetIcon() : nullptr;
		EntryIcon->SetBrushFromTexture(Icon);
		EntryIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UMTSItemPickerEntry::HandleClicked()
{
	if (UMTSItemPicker* Picker = mPicker.Get())
	{
		Picker->PickItem(mItem);
	}
}
