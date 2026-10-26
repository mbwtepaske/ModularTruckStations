#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MTSColumnFilterButton.generated.h"

class UButton;
class UFGItemDescriptor;
class UImage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMTSOnColumnButtonEvent, int32, Column);

/**
 * One module column in the filter UI: a button that shows the column filter's icon. A left click asks to open the
 * item picker for the column, a right click asks to clear its filter; the module widget handles both.
 *
 * The BP layout needs a Button named "IconButton" with an Image named "FilterIcon" inside.
 */
UCLASS(Abstract)
class MODULARTRUCKSTATION_API UMTSColumnFilterButton : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Shows the item's icon, or the unset icon for null. */
	UFUNCTION(BlueprintCallable, Category = "Modular Station|UI")
	void SetFilter(TSubclassOf<UFGItemDescriptor> Item);

	/** Column this button stands for; passed to the events. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modular Station|UI", meta = (ExposeOnSpawn))
	int32 Column = 0;

	/** Broadcast on a left click. */
	UPROPERTY(BlueprintAssignable, Category = "Modular Station|UI")
	FMTSOnColumnButtonEvent OnPickRequested;

	/** Broadcast on a right click. */
	UPROPERTY(BlueprintAssignable, Category = "Modular Station|UI")
	FMTSOnColumnButtonEvent OnClearRequested;

protected:
	virtual void NativeOnInitialized() override;

	/** A UMG Button only handles the left mouse button; the right click reaches the widget itself. */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Icon shown while the column has no filter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modular Station|UI")
	TObjectPtr<UTexture2D> mUnsetIcon;

	UPROPERTY(BlueprintReadOnly, Category = "Modular Station|UI", meta = (BindWidget))
	TObjectPtr<UButton> IconButton;

	UPROPERTY(BlueprintReadOnly, Category = "Modular Station|UI", meta = (BindWidget))
	TObjectPtr<UImage> FilterIcon;

private:
	UFUNCTION()
	void HandleClicked();
};
