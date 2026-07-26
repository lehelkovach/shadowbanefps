// Copyright shadowbanefps.

#include "SBSpawnPoint.h"

ASBSpawnPoint::ASBSpawnPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = false;
}

bool ASBSpawnPoint::IsAvailableFor(ESBTeam InTeam, ESBConquestStage Stage) const
{
	if (Team != InTeam)
	{
		return false;
	}

	return static_cast<uint8>(Stage) >= static_cast<uint8>(MinStage)
		&& static_cast<uint8>(Stage) <= static_cast<uint8>(MaxStage);
}
