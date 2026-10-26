#include "UI/MTSColumnFilterButton.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Resources/FGItemDescriptor.h"

void UMTSColumnFilterButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (IconButton)
	{
		IconButton->OnClicked.AddDynamic(this, &UMTSColumnFilterButton::HandleClicked);
	}
	SetFilter(nullptr);
}

FReply UMTSColumnFilterButton::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnClearRequested.Broadcast(Column);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UMTSColumnFilterButton::SetFilter(TSubclassOf<UFGItemDescriptor> Item)
{
	if (FilterIcon)
	{
		UTexture2D* Icon = Item ? UFGItemDescriptor::GetBigIcon(Item) : mUnsetIcon.Get();
		FilterIcon->SetBrushFromTexture(Icon);
		FilterIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UMTSColumnFilterButton::HandleClicked()
{
	OnPickRequested.Broadcast(Column);
}
