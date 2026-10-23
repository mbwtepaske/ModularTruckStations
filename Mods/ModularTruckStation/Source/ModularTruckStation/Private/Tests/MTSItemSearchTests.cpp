#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/MTSItemSearch.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSItemSearchMatchesTest, "ModularTruckStation.UI.ItemSearch.Matches", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMTSItemSearchMatchesTest::RunTest(const FString& Parameters)
{
	using namespace MTSItemSearch;

	TestTrue(TEXT("part of a word"), Matches(TEXT("Iron Plate"), TEXT("pla")));
	TestTrue(TEXT("ignores case"), Matches(TEXT("Iron Plate"), TEXT("IRON")));
	TestTrue(TEXT("all words, any order"), Matches(TEXT("Reinforced Iron Plate"), TEXT("plate iron")));
	TestTrue(TEXT("extra spaces"), Matches(TEXT("Iron Plate"), TEXT("  iron   plate ")));
	TestFalse(TEXT("one word missing"), Matches(TEXT("Iron Plate"), TEXT("iron rod")));
	TestFalse(TEXT("empty search"), Matches(TEXT("Iron Plate"), TEXT("")));
	TestFalse(TEXT("only spaces"), Matches(TEXT("Iron Plate"), TEXT("   ")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
