#pragma once

#include "CoreMinimal.h"

/** Pure search rules of the item picker, shared by the picker and tests. */
namespace MTSItemSearch
{
	/**
	 * @return true if every word of the search text occurs in the item name, ignoring case and word order.
	 * An empty search text matches nothing; the picker then hides its results.
	 */
	inline bool Matches(const FString& ItemName, const FString& SearchText)
	{
		TArray<FString> Words;
		SearchText.ParseIntoArrayWS(Words);
		if (Words.IsEmpty())
		{
			return false;
		}
		for (const FString& Word : Words)
		{
			if (!ItemName.Contains(Word, ESearchCase::IgnoreCase))
			{
				return false;
			}
		}
		return true;
	}
}
