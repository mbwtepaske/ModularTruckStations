#pragma once

#include "CoreMinimal.h"

/**
 * Pure rules for linked module columns, shared by the module and tests. Links[Gap] joins column Gap and Gap + 1;
 * linked neighbours form a group that always has one filter. Filters are templated so tests need no item classes;
 * a default-constructed value (e.g. null) means unset.
 */
namespace MTSColumnLinks
{
	/** Finds the first and last column of the group that contains Column. */
	inline void GetGroup(const TArray<bool>& Links, int32 Column, int32& OutFirst, int32& OutLast)
	{
		OutFirst = Column;
		while (Links.IsValidIndex(OutFirst - 1) && Links[OutFirst - 1])
		{
			--OutFirst;
		}
		OutLast = Column;
		while (Links.IsValidIndex(OutLast) && Links[OutLast])
		{
			++OutLast;
		}
	}

	/** @return the filters after setting Column's whole group to Value. */
	template <typename T>
	TArray<T> WithGroupFilter(TArray<T> Filters, const TArray<bool>& Links, int32 Column, const T& Value)
	{
		int32 First, Last;
		GetGroup(Links, Column, First, Last);
		for (int32 Index = First; Index <= Last; ++Index)
		{
			Filters[Index] = Value;
		}
		return Filters;
	}

	/**
	 * @return the filters after linking Gap: both groups get the left group's filter, or the right group's when the
	 * left one is unset. Links must still have Gap off.
	 */
	template <typename T>
	TArray<T> WithLinkedGap(TArray<T> Filters, const TArray<bool>& Links, int32 Gap)
	{
		int32 LeftFirst, LeftLast, RightFirst, RightLast;
		GetGroup(Links, Gap, LeftFirst, LeftLast);
		GetGroup(Links, Gap + 1, RightFirst, RightLast);
		const T Value = Filters[Gap] != T() ? Filters[Gap] : Filters[Gap + 1];
		for (int32 Index = LeftFirst; Index <= RightLast; ++Index)
		{
			Filters[Index] = Value;
		}
		return Filters;
	}

	/**
	 * Switches off every link between columns with different filters, e.g. in a save made before links existed.
	 * @return true if a link was changed.
	 */
	template <typename T>
	bool RemoveInconsistentLinks(TArray<bool>& Links, const TArray<T>& Filters)
	{
		bool bChanged = false;
		for (int32 Gap = 0; Gap < Links.Num(); ++Gap)
		{
			if (Links[Gap] && (!Filters.IsValidIndex(Gap + 1) || Filters[Gap] != Filters[Gap + 1]))
			{
				Links[Gap] = false;
				bChanged = true;
			}
		}
		return bChanged;
	}
}
