#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Module/MTSColumnLinks.h"

namespace MTSColumnLinksTests
{
	// Filters as names: NAME_None is unset.
	const FName IronFilter(TEXT("Iron"));
	const FName CopperFilter(TEXT("Copper"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSColumnGroupTest, "ModularTruckStation.Module.Links.Groups", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMTSColumnGroupTest::RunTest(const FString& Parameters)
{
	using namespace MTSColumnLinks;
	using namespace MTSColumnLinksTests;
	int32 First, Last;

	GetGroup({true, true, true}, 2, First, Last);
	TestTrue(TEXT("fully linked XL is one group"), First == 0 && Last == 3);

	GetGroup({true, false, true}, 1, First, Last);
	TestTrue(TEXT("XL 1+2 | 3+4: column 2 in first group"), First == 0 && Last == 1);
	GetGroup({true, false, true}, 2, First, Last);
	TestTrue(TEXT("XL 1+2 | 3+4: column 3 in second group"), First == 2 && Last == 3);

	GetGroup({false, false, false}, 2, First, Last);
	TestTrue(TEXT("unlinked column is its own group"), First == 2 && Last == 2);

	GetGroup({}, 0, First, Last);
	TestTrue(TEXT("S module (no gaps)"), First == 0 && Last == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSColumnGroupFilterTest, "ModularTruckStation.Module.Links.GroupFilter", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMTSColumnGroupFilterTest::RunTest(const FString& Parameters)
{
	using namespace MTSColumnLinks;
	using namespace MTSColumnLinksTests;

	const TArray<FName> All = WithGroupFilter<FName>({NAME_None, NAME_None, NAME_None, NAME_None}, {true, true, true}, 1, IronFilter);
	TestTrue(TEXT("pick on a fully linked module sets all columns"), All == TArray<FName>({IronFilter, IronFilter, IronFilter, IronFilter}));

	const TArray<FName> Group = WithGroupFilter<FName>({IronFilter, IronFilter, NAME_None, NAME_None}, {true, false, true}, 2, CopperFilter);
	TestTrue(TEXT("pick sets only the column's group"), Group == TArray<FName>({IronFilter, IronFilter, CopperFilter, CopperFilter}));

	const TArray<FName> Cleared = WithGroupFilter<FName>({IronFilter, IronFilter, CopperFilter}, {true, false}, 0, NAME_None);
	TestTrue(TEXT("clearing unsets the group"), Cleared == TArray<FName>({NAME_None, NAME_None, CopperFilter}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSColumnLinkTest, "ModularTruckStation.Module.Links.Link", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMTSColumnLinkTest::RunTest(const FString& Parameters)
{
	using namespace MTSColumnLinks;
	using namespace MTSColumnLinksTests;

	TestTrue(TEXT("left set, right unset: left wins"),
		WithLinkedGap<FName>({IronFilter, NAME_None}, {false}, 0) == TArray<FName>({IronFilter, IronFilter}));
	TestTrue(TEXT("left unset, right set: right wins"),
		WithLinkedGap<FName>({NAME_None, CopperFilter}, {false}, 0) == TArray<FName>({CopperFilter, CopperFilter}));
	TestTrue(TEXT("both set: left wins"),
		WithLinkedGap<FName>({IronFilter, CopperFilter}, {false}, 0) == TArray<FName>({IronFilter, IronFilter}));
	TestTrue(TEXT("linking joins whole groups"),
		WithLinkedGap<FName>({IronFilter, IronFilter, CopperFilter, CopperFilter}, {true, false, true}, 1) == TArray<FName>({IronFilter, IronFilter, IronFilter, IronFilter}));
	TestTrue(TEXT("other groups are kept"),
		WithLinkedGap<FName>({IronFilter, NAME_None, CopperFilter, CopperFilter}, {false, false, true}, 0) == TArray<FName>({IronFilter, IronFilter, CopperFilter, CopperFilter}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSColumnLinkConsistencyTest, "ModularTruckStation.Module.Links.Consistency", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMTSColumnLinkConsistencyTest::RunTest(const FString& Parameters)
{
	using namespace MTSColumnLinks;
	using namespace MTSColumnLinksTests;

	TArray<bool> Links = {true, true, true};
	TestTrue(TEXT("older save with mixed filters: links changed"), RemoveInconsistentLinks<FName>(Links, {IronFilter, IronFilter, CopperFilter, NAME_None}));
	TestTrue(TEXT("only links between equal filters stay"), Links == TArray<bool>({true, false, false}));

	Links = {true};
	TestFalse(TEXT("consistent links stay"), RemoveInconsistentLinks<FName>(Links, {NAME_None, NAME_None}));
	TestTrue(TEXT("new module stays linked"), Links[0]);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
