#include "Module/MTSCargoModule.h"

AMTSCargoModule::AMTSCargoModule()
{
	mStationType = EMTSStationType::Cargo;
	// 12 slots per column: an XL module equals the 48 slots of a vanilla truck station.
	mSlotsPerColumn = 12;
	// Placeholder balance: an XL module doubles the vanilla station's 20 MW.
	mPowerPerColumn = 5.f;
}
