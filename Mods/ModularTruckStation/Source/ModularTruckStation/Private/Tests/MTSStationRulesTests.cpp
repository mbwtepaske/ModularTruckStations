#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Station/MTSStationRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMTSCanAttachModuleTest, "ModularTruckStation.Station.Rules.CanAttachModule", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMTSCanAttachModuleTest::RunTest(const FString& Parameters)
{
	using namespace MTSStationRules;

	TestTrue(TEXT("cargo module on free cargo base"), CanAttachModule(EMTSStationType::Cargo, false, EMTSStationType::Cargo));
	TestFalse(TEXT("cargo module on cargo base with a module"), CanAttachModule(EMTSStationType::Cargo, true, EMTSStationType::Cargo));
	TestFalse(TEXT("fluid module on free cargo base"), CanAttachModule(EMTSStationType::Cargo, false, EMTSStationType::Fluid));
	TestFalse(TEXT("cargo module on free fluid base"), CanAttachModule(EMTSStationType::Fluid, false, EMTSStationType::Cargo));
	TestTrue(TEXT("fluid module on free fluid base"), CanAttachModule(EMTSStationType::Fluid, false, EMTSStationType::Fluid));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
