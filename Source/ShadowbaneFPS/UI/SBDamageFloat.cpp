// Copyright shadowbanefps.

#include "UI/SBDamageFloat.h"
#include "UI/SBSiegeHUD.h"

void SBDamageFloat::Push(UWorld* World, const FVector& WorldLocation, float Amount, bool bHeal)
{
	ASBSiegeHUD::AddDamageFloater(World, WorldLocation, Amount, bHeal);
}
