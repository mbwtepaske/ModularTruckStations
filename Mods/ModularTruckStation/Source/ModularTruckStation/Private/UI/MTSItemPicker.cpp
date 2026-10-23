#include "UI/MTSItemPicker.h"

#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "ModularTruckStation.h"
#include "Resources/FGItemDescriptor.h"
#include "UI/MTSItemPickerEntry.h"
#include "UI/MTSItemSearch.h"

#define LOCTEXT_NAMESPACE "ModularTruckStation"

UMTSItemPicker::UMTSItemPicker(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mUnsetHintText = LOCTEXT("PickerUnsetHint", "Search item...");
}

void UMTSItemPicker::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (SearchBox)
	{
		SearchBox->OnTextChanged.AddDynamic(this, &UMTSItemPicker::HandleSearchTextChanged);
		SearchBox->OnTextCommitted.AddDynamic(this, &UMTSItemPicker::HandleSearchTextCommitted);
	}
	SetResultsVisible(false);
	UpdateCurrentDisplay();
}

void UMTSItemPicker::NativeDestruct()
{
	// Closing the UI while typing would leave the keyboard focus on a destroyed widget, and the game would no longer
	// receive key presses (e.g. the interact key) until the player clicks.
	if (SearchBox && SearchBox->HasAnyUserFocus() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
	Super::NativeDestruct();
}

void UMTSItemPicker::SetItems(const TArray<TSubclassOf<UFGItemDescriptor>>& Items)
{
	mItems = Items;
	mItemNames.Reset(mItems.Num());
	for (const TSubclassOf<UFGItemDescriptor>& Item : mItems)
	{
		mItemNames.Add(Item ? UFGItemDescriptor::GetItemName(Item).ToString() : FString());
	}
	UpdateResults(SearchBox ? SearchBox->GetText().ToString() : FString());
}

void UMTSItemPicker::SetCurrentItem(TSubclassOf<UFGItemDescriptor> Item, bool bCallDelegate)
{
	mCurrentItem = Item;
	UpdateCurrentDisplay();
	if (bCallDelegate)
	{
		OnItemPicked.Broadcast(Item);
	}
}

void UMTSItemPicker::PickItem(TSubclassOf<UFGItemDescriptor> Item)
{
	UE_LOG(LogModularTruckStation, Log, TEXT("%s picked %s (%d listeners)"), *GetPathNameSafe(this), *GetNameSafe(Item), OnItemPicked.GetAllObjects().Num());
	ClearSearch();
	SetCurrentItem(Item, true);
}

void UMTSItemPicker::ClearSearch()
{
	if (SearchBox)
	{
		SearchBox->SetText(FText::GetEmpty());
	}
	// Explicit, in case setting the text does not fire OnTextChanged.
	UpdateResults(FString());
}

void UMTSItemPicker::HandleSearchTextChanged(const FText& Text)
{
	UpdateResults(Text.ToString());
}

void UMTSItemPicker::HandleSearchTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter && mNumResults > 0)
	{
		PickItem(mEntries[0]->GetItem());
	}
	else if (CommitMethod == ETextCommit::OnCleared)
	{
		ClearSearch();
	}
	// Focus loss keeps the results open: a click on a row takes the focus before the row receives the click.
}

void UMTSItemPicker::UpdateResults(const FString& SearchText)
{
	mNumResults = 0;
	if (ResultList && mEntryClass)
	{
		for (int32 Index = 0; Index < mItems.Num() && mNumResults < mMaxResults; ++Index)
		{
			if (!MTSItemSearch::Matches(mItemNames[Index], SearchText))
			{
				continue;
			}
			if (mNumResults == mEntries.Num())
			{
				UMTSItemPickerEntry* NewEntry = CreateWidget<UMTSItemPickerEntry>(this, mEntryClass);
				ResultList->AddChild(NewEntry);
				mEntries.Add(NewEntry);
			}
			UMTSItemPickerEntry* Entry = mEntries[mNumResults++];
			Entry->SetItem(this, mItems[Index]);
			Entry->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else if (!SearchText.IsEmpty())
	{
		UE_LOG(LogModularTruckStation, Warning, TEXT("%s: cannot list results, %s"), *GetName(), ResultList ? TEXT("no entry class set") : TEXT("no ResultList"));
	}

	for (int32 Index = mNumResults; Index < mEntries.Num(); ++Index)
	{
		mEntries[Index]->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetResultsVisible(mNumResults > 0);
}

void UMTSItemPicker::SetResultsVisible(bool bVisible)
{
	if (UWidget* Area = ResultArea ? ResultArea.Get() : ResultList.Get())
	{
		Area->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UMTSItemPicker::UpdateCurrentDisplay()
{
	if (SearchBox)
	{
		SearchBox->SetHintText(mCurrentItem ? UFGItemDescriptor::GetItemName(mCurrentItem) : mUnsetHintText);
	}
	if (CurrentIcon)
	{
		UTexture2D* Icon = mCurrentItem ? UFGItemDescriptor::GetSmallIcon(mCurrentItem) : mUnsetIcon.Get();
		CurrentIcon->SetBrushFromTexture(Icon);
		CurrentIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

#undef LOCTEXT_NAMESPACE
