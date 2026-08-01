// Copyright shadowbanefps.

#include "SBSiegeGameState.h"
#include "Characters/SBCharacterArchetype.h"
#include "Characters/SBPilotRoster.h"
#include "Core/SBMatchShopCatalog.h"
#include "SBClientDebug.h"
#include "SBLog.h"
#include "Net/UnrealNetwork.h"

ASBSiegeGameState::ASBSiegeGameState()
{
	SetNetUpdateFrequency(10.0f);
}

void ASBSiegeGameState::BeginPlay()
{
	Super::BeginPlay();

	if (LocalRoster.Num() == 0)
	{
		USBPilotRoster::BuildDefaultRoster(this, LocalRoster);
	}

	// Warm match-shop catalog for future shop UI (items-catalog.json).
	USBMatchShopCatalog::Get().EnsureLoaded();
}

USBMatchShopCatalog& ASBSiegeGameState::GetMatchShopCatalog() const
{
	return USBMatchShopCatalog::Get();
}

void ASBSiegeGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASBSiegeGameState, Phase);
	DOREPLIFETIME(ASBSiegeGameState, ConquestStage);
	DOREPLIFETIME(ASBSiegeGameState, Result);
	DOREPLIFETIME(ASBSiegeGameState, MatchMode);
	DOREPLIFETIME(ASBSiegeGameState, FinalObjectiveProgress);
	DOREPLIFETIME(ASBSiegeGameState, RegulationDeadlineServerTime);
}

float ASBSiegeGameState::GetRemainingRegulationSeconds() const
{
	const double Now = GetServerWorldTimeSeconds();
	return FMath::Max(0.f, static_cast<float>(RegulationDeadlineServerTime - Now));
}

USBCharacterArchetype* ASBSiegeGameState::FindArchetypeById(FName ArchetypeId) const
{
	if (ArchetypeId.IsNone())
	{
		return nullptr;
	}

	for (USBCharacterArchetype* Arch : LocalRoster)
	{
		if (Arch && Arch->ArchetypeId == ArchetypeId)
		{
			return Arch;
		}
	}
	return nullptr;
}

void ASBSiegeGameState::ServerSetPhase(ESBMatchPhase NewPhase)
{
	if (HasAuthority() && Phase != NewPhase)
	{
		Phase = NewPhase;
		OnRep_Phase();
	}
}

void ASBSiegeGameState::ServerSetConquestStage(ESBConquestStage NewStage)
{
	if (HasAuthority() && ConquestStage != NewStage)
	{
		ConquestStage = NewStage;
		OnRep_ConquestStage();
	}
}

void ASBSiegeGameState::OnRep_Phase()
{
	UE_LOG(LogShadowbaneNet, Log, TEXT("OnRep_Phase -> %s"), *UEnum::GetValueAsString(Phase));
	UE_LOG(LogShadowbaneClient, Log, TEXT("Client match phase -> %s"), *UEnum::GetValueAsString(Phase));
	FSBClientDebug::PushMessage(FString::Printf(TEXT("Phase -> %s"), *UEnum::GetDisplayValueAsText(Phase).ToString()), 5.f);
	OnPhaseChanged.Broadcast(Phase);
}

void ASBSiegeGameState::OnRep_ConquestStage()
{
	UE_LOG(LogShadowbaneNet, Log, TEXT("OnRep_ConquestStage -> %s"), *UEnum::GetValueAsString(ConquestStage));
	UE_LOG(LogShadowbaneClient, Log, TEXT("Client conquest stage -> %s"), *UEnum::GetValueAsString(ConquestStage));
	FSBClientDebug::PushMessage(FString::Printf(TEXT("Front line -> %s"), *UEnum::GetDisplayValueAsText(ConquestStage).ToString()), 5.f);
	OnConquestStageChanged.Broadcast(ConquestStage);
}
