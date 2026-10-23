#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MTSItemPickerEntry.generated.h"

class UButton;
class UFGItemDescriptor;
class UImage;
class UMTSItemPicker;
class UTextBlock;

/**
 * One result row of the item picker: icon and name of an item. Clicking it picks the item.
 * The BP layout needs a Button named "EntryButton" and a TextBlock named "EntryName"; an Image named "EntryIcon"
 * is optional.
 */
UCLASS(Abstract)
class MODULARTRUCKSTATION_API UMTSItemPickerEntry : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Shows the item; a click is reported to the picker. */
	void SetItem(UMTSItemPicker* Picker, TSubclassOf<UFGItemDescriptor> Item);

	UFUNCTION(BlueprintPure, Category = "Modular Station|UI")
	TSubclassOf<UFGItemDescriptor> GetItem() const { return mItem; }

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, Category = "Modular Station|UI", meta = (BindWidget))
	TObjectPtr<UButton> EntryButton;

	UPROPERTY(BlueprintReadOnly, Category = "Modular Station|UI", meta = (BindWidgetOptional))
	TObjectPtr<UImage> EntryIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Modular Station|UI", meta = (BindWidget))
	TObjectPtr<UTextBlock> EntryName;

private:
	UFUNCTION()
	void HandleClicked();

	TWeakObjectPtr<UMTSItemPicker> mPicker;

	UPROPERTY(Transient)
	TSubclassOf<UFGItemDescriptor> mItem;
};
