#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "MTSItemPicker.generated.h"

class UEditableTextBox;
class UFGItemDescriptor;
class UImage;
class UMTSItemPickerEntry;
class UPanelWidget;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMTSOnItemPicked, TSubclassOf<UFGItemDescriptor>, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMTSOnPickerCancelled);

/**
 * Searchable item picker. Shows the current item (icon, and its name as the search field's hint); typing lists the
 * matching items below the field, and clicking one (or Enter for the first) picks it.
 *
 * Popup mode (mListAllWhenEmpty, mShowNoFilterEntry, mMaxResults 0): the list shows every item while the search is
 * empty, with a "No filter" row first that picks null.
 *
 * The BP layout needs an EditableTextBox named "SearchBox" and a panel named "ResultList" (e.g. a Vertical Box in a
 * Scroll Box) that receives the result rows. Optional: an Image "CurrentIcon", and a widget "ResultArea" that is
 * shown and hidden with the results (e.g. the Scroll Box); without it, ResultList itself is shown and hidden.
 */
UCLASS(Abstract)
class MODULARTRUCKSTATION_API UMTSItemPicker : public UUserWidget
{
	GENERATED_BODY()
public:
	UMTSItemPicker(const FObjectInitializer& ObjectInitializer);

	/** Sets the items the player can search. */
	UFUNCTION(BlueprintCallable, Category = "Modular Station|UI")
	void SetItems(const TArray<TSubclassOf<UFGItemDescriptor>>& Items);

	/**
	 * Shows an item as the current one; null shows none.
	 * @param bCallDelegate	broadcast OnItemPicked; false when only showing state, e.g. on refresh.
	 */
	UFUNCTION(BlueprintCallable, Category = "Modular Station|UI")
	void SetCurrentItem(TSubclassOf<UFGItemDescriptor> Item, bool bCallDelegate);

	UFUNCTION(BlueprintPure, Category = "Modular Station|UI")
	TSubclassOf<UFGItemDescriptor> GetCurrentItem() const { return mCurrentItem; }

	/** Empties the search field (listing all items in popup mode) and optionally gives it keyboard focus, e.g. on open. */
	UFUNCTION(BlueprintCallable, Category = "Modular Station|UI")
	void ResetSearch(bool bFocus);

	/** Broadcast when the player picks an item; null for the "No filter" row. */
	UPROPERTY(BlueprintAssignable, Category = "Modular Station|UI")
	FMTSOnItemPicked OnItemPicked;

	/** Broadcast when the player presses Escape in the search field; a popup closes on it. */
	UPROPERTY(BlueprintAssignable, Category = "Modular Station|UI")
	FMTSOnPickerCancelled OnCancelled;

	/** Called by a result row when it is clicked. */
	void PickItem(TSubclassOf<UFGItemDescriptor> Item);

	FORCEINLINE const FText& GetNoFilterText() const { return mNoFilterText; }
	FORCEINLINE UTexture2D* GetUnsetIcon() const { return mUnsetIcon.Get(); }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	/** Widget class of a result row. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular Station|UI")
	TSubclassOf<UMTSItemPickerEntry> mEntryClass;

	/** Maximum number of item rows, 0 = no limit (for a scrolling list). Keeps an inline list short while the search is vague. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular Station|UI", meta = (ClampMin = 0))
	int32 mMaxResults = 40;

	/** List every item while the search field is empty, instead of hiding the list. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular Station|UI")
	bool mListAllWhenEmpty = false;

	/** Show a row that picks null ("No filter") first while the search field is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular Station|UI")
	bool mShowNoFilterEntry = false;

	/** Name of the null row. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular Station|UI")
	FText mNoFilterText;

	/** Hint of the search field while no item is set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular Station|UI")
	FText mUnsetHintText;

	/** Icon shown while no item is set, and on the null row; hidden if null. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular Station|UI")
	TObjectPtr<UTexture2D> mUnsetIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Modular Station|UI", meta = (BindWidget))
	TObjectPtr<UEditableTextBox> SearchBox;

	UPROPERTY(BlueprintReadOnly, Category = "Modular Station|UI", meta = (BindWidget))
	TObjectPtr<UPanelWidget> ResultList;

	UPROPERTY(BlueprintReadOnly, Category = "Modular Station|UI", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ResultArea;

	UPROPERTY(BlueprintReadOnly, Category = "Modular Station|UI", meta = (BindWidgetOptional))
	TObjectPtr<UImage> CurrentIcon;

private:
	UFUNCTION()
	void HandleSearchTextChanged(const FText& Text);

	UFUNCTION()
	void HandleSearchTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/** Lists the items matching the search text, reusing result rows; hides the results if none match. */
	void UpdateResults(const FString& SearchText);

	/** Shows the next result row (created on demand) with Item. */
	void AddResult(TSubclassOf<UFGItemDescriptor> Item);

	/** Empties the search field and hides the results. */
	void ClearSearch();

	void SetResultsVisible(bool bVisible);

	/** Shows the current item's icon and name hint. */
	void UpdateCurrentDisplay();

	UPROPERTY(Transient)
	TArray<TSubclassOf<UFGItemDescriptor>> mItems;

	/** Display name per entry of mItems, cached for searching. */
	TArray<FString> mItemNames;

	UPROPERTY(Transient)
	TSubclassOf<UFGItemDescriptor> mCurrentItem;

	/** Result rows, created on demand and reused; the first mNumResults are in use. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMTSItemPickerEntry>> mEntries;

	int32 mNumResults = 0;

	/** Index of the first item row in mEntries (1 when the null row is shown), for Enter. */
	int32 mFirstItemResult = 0;
};
