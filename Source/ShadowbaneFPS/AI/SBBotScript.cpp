// Copyright shadowbanefps.

#include "SBBotScript.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Core/SBLog.h"

namespace SBBotScriptPrivate
{
	static FString StripComment(const FString& In)
	{
		FString Out = In;
		int32 Hash = INDEX_NONE;
		if (Out.FindChar(TEXT('#'), Hash))
		{
			Out.LeftInline(Hash);
		}
		return Out.TrimStartAndEnd();
	}

	static ESBBotTargetKind ParseTarget(const FString& Token)
	{
		const FString T = Token.ToLower();
		if (T == TEXT("enemy")) return ESBBotTargetKind::Enemy;
		if (T == TEXT("ally_hurt") || T == TEXT("allyhurt") || T == TEXT("ally")) return ESBBotTargetKind::AllyHurt;
		if (T == TEXT("capture") || T == TEXT("cap")) return ESBBotTargetKind::Capture;
		if (T == TEXT("objective") || T == TEXT("obj")) return ESBBotTargetKind::Objective;
		if (T == TEXT("structure") || T == TEXT("gate") || T == TEXT("wall")) return ESBBotTargetKind::Structure;
		if (T == TEXT("spawn")) return ESBBotTargetKind::Spawn;
		if (T == TEXT("else") || T == TEXT("default")) return ESBBotTargetKind::Else;
		return ESBBotTargetKind::None;
	}

	static ESBBotAction ParseAction(const FString& Token)
	{
		const FString T = Token.ToLower();
		if (T == TEXT("pursue") || T == TEXT("go") || T == TEXT("move")) return ESBBotAction::Pursue;
		if (T == TEXT("hold") || T == TEXT("stay")) return ESBBotAction::Hold;
		if (T == TEXT("fire") || T == TEXT("shoot") || T == TEXT("attack")) return ESBBotAction::Fire;
		if (T == TEXT("pursue_fire") || T == TEXT("pursufire") || T == TEXT("engage")) return ESBBotAction::PursueFire;
		if (T == TEXT("retreat") || T == TEXT("fallback")) return ESBBotAction::Retreat;
		return ESBBotAction::Pursue;
	}
}

bool USBBotScriptLibrary::ParseRuleLine(const FString& Line, FSBBotRule& OutRule, FString& OutError)
{
	OutRule = FSBBotRule();
	OutRule.SourceLine = Line;

	FString Body = Line;
	if (Body.StartsWith(TEXT("when "), ESearchCase::IgnoreCase))
	{
		Body.RightChopInline(5);
	}

	FString Left;
	FString Right;
	if (!Body.Split(TEXT("->"), &Left, &Right))
	{
		OutError = TEXT("Rule missing '->' action");
		return false;
	}

	Left.TrimStartAndEndInline();
	Right.TrimStartAndEndInline();

	TArray<FString> Tokens;
	Left.ParseIntoArrayWS(Tokens);
	if (Tokens.Num() == 0)
	{
		OutError = TEXT("Empty rule condition");
		return false;
	}

	OutRule.Target = SBBotScriptPrivate::ParseTarget(Tokens[0]);
	if (OutRule.Target == ESBBotTargetKind::None)
	{
		OutError = FString::Printf(TEXT("Unknown target '%s'"), *Tokens[0]);
		return false;
	}

	for (int32 i = 1; i < Tokens.Num(); ++i)
	{
		const FString& Tok = Tokens[i];
		const FString Lower = Tok.ToLower();
		if (Lower == TEXT("in_range") || Lower == TEXT("range"))
		{
			if (i + 1 < Tokens.Num())
			{
				OutRule.InRange = FCString::Atof(*Tokens[i + 1]);
				++i;
			}
		}
		else if (Lower == TEXT("open") || Lower == TEXT("uncaptured")
			|| Lower == TEXT("intact") || Lower == TEXT("unlocked")
			|| Lower == TEXT("any"))
		{
			OutRule.Qualifier = FName(*Lower);
		}
	}

	// Action side may be "pursue fire" => PursueFire, or single token.
	TArray<FString> ActTokens;
	Right.ParseIntoArrayWS(ActTokens);
	if (ActTokens.Num() == 0)
	{
		OutError = TEXT("Missing action after ->");
		return false;
	}
	if (ActTokens.Num() >= 2
		&& ActTokens[0].Equals(TEXT("pursue"), ESearchCase::IgnoreCase)
		&& (ActTokens[1].Equals(TEXT("fire"), ESearchCase::IgnoreCase)
			|| ActTokens[1].Equals(TEXT("shoot"), ESearchCase::IgnoreCase)))
	{
		OutRule.Action = ESBBotAction::PursueFire;
	}
	else
	{
		OutRule.Action = SBBotScriptPrivate::ParseAction(ActTokens[0]);
	}

	return true;
}

bool USBBotScriptLibrary::ParseScriptText(const FString& Text, FSBBotScript& OutScript, FString& OutError)
{
	OutScript = FSBBotScript();
	OutError.Reset();

	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, false);

	for (const FString& Raw : Lines)
	{
		const FString Line = SBBotScriptPrivate::StripComment(Raw);
		if (Line.IsEmpty())
		{
			continue;
		}

		if (Line.StartsWith(TEXT("when "), ESearchCase::IgnoreCase)
			|| Line.Contains(TEXT("->")))
		{
			FSBBotRule Rule;
			FString RuleError;
			if (!ParseRuleLine(Line, Rule, RuleError))
			{
				OutError = RuleError;
				return false;
			}
			OutScript.Rules.Add(Rule);
			continue;
		}

		FString Key;
		FString Value;
		if (Line.Split(TEXT(":"), &Key, &Value))
		{
			Key.TrimStartAndEndInline();
			Value.TrimStartAndEndInline();
			const FString K = Key.ToLower();
			if (K == TEXT("id") || K == TEXT("script"))
			{
				OutScript.ScriptId = FName(*Value);
			}
			else if (K == TEXT("role"))
			{
				OutScript.Role = FName(*Value);
			}
			else if (K == TEXT("retarget"))
			{
				OutScript.RetargetSeconds = FCString::Atof(*Value);
			}
			else if (K == TEXT("engage") || K == TEXT("engage_range"))
			{
				OutScript.EngageRange = FCString::Atof(*Value);
			}
			else if (K == TEXT("fire") || K == TEXT("fire_interval"))
			{
				OutScript.FireInterval = FCString::Atof(*Value);
			}
		}
	}

	if (OutScript.Rules.Num() == 0)
	{
		OutError = TEXT("No rules parsed");
		return false;
	}
	if (OutScript.ScriptId.IsNone())
	{
		OutScript.ScriptId = FName(TEXT("Unnamed"));
	}
	return true;
}

FSBBotScript USBBotScriptLibrary::MakeDefaultScript(FName ScriptId)
{
	FSBBotScript Script;
	Script.ScriptId = ScriptId.IsNone() ? FName(TEXT("Default")) : ScriptId;
	Script.Role = FName(TEXT("general"));
	Script.RetargetSeconds = 1.25f;
	Script.EngageRange = 2800.f;
	Script.FireInterval = 0.55f;

	auto Add = [&](ESBBotTargetKind Target, float Range, FName Qual, ESBBotAction Action)
	{
		FSBBotRule R;
		R.Target = Target;
		R.InRange = Range;
		R.Qualifier = Qual;
		R.Action = Action;
		Script.Rules.Add(R);
	};

	Add(ESBBotTargetKind::Enemy, 2500.f, NAME_None, ESBBotAction::PursueFire);
	Add(ESBBotTargetKind::Capture, 0.f, FName(TEXT("open")), ESBBotAction::Pursue);
	Add(ESBBotTargetKind::Objective, 0.f, FName(TEXT("unlocked")), ESBBotAction::Pursue);
	Add(ESBBotTargetKind::Structure, 0.f, FName(TEXT("intact")), ESBBotAction::PursueFire);
	Add(ESBBotTargetKind::Else, 0.f, NAME_None, ESBBotAction::Hold);
	return Script;
}

bool USBBotScriptLibrary::LoadScriptById(FName ScriptId, FSBBotScript& OutScript)
{
	const FString Dir = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("BotScripts"));
	const FString Preferred = FPaths::Combine(Dir, FString::Printf(TEXT("%s.sbbot"), *ScriptId.ToString()));
	const FString Fallback = FPaths::Combine(Dir, TEXT("Default.sbbot"));

	FString Path;
	if (FPaths::FileExists(Preferred))
	{
		Path = Preferred;
	}
	else if (FPaths::FileExists(Fallback))
	{
		Path = Fallback;
	}
	else
	{
		OutScript = MakeDefaultScript(ScriptId);
		UE_LOG(LogShadowbaneServer, Verbose, TEXT("Bot script %s missing — using built-in default"), *ScriptId.ToString());
		return true;
	}

	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		OutScript = MakeDefaultScript(ScriptId);
		UE_LOG(LogShadowbaneServer, Warning, TEXT("Failed to read bot script %s — using default"), *Path);
		return false;
	}

	FString Error;
	if (!ParseScriptText(Text, OutScript, Error))
	{
		UE_LOG(LogShadowbaneServer, Warning, TEXT("Bot script parse error (%s): %s — using default"), *Path, *Error);
		OutScript = MakeDefaultScript(ScriptId);
		return false;
	}

	if (OutScript.ScriptId.IsNone())
	{
		OutScript.ScriptId = ScriptId;
	}

	UE_LOG(LogShadowbaneServer, Log, TEXT("Loaded bot script %s (%d rules) from %s"),
		*OutScript.ScriptId.ToString(), OutScript.Rules.Num(), *Path);
	return true;
}

bool USBBotScriptLibrary::EvaluateRules(const FSBBotScript& Script, const FSBBotWorldFacts& Facts, FSBBotDecision& OutDecision)
{
	OutDecision = FSBBotDecision();

	auto ApplyAction = [&](ESBBotAction Action, AActor* Target)
	{
		OutDecision.MatchedAction = Action;
		switch (Action)
		{
		case ESBBotAction::Hold:
			OutDecision.bHold = true;
			OutDecision.MoveTarget = nullptr;
			OutDecision.bWantFire = false;
			break;
		case ESBBotAction::Fire:
			OutDecision.MoveTarget = Target;
			OutDecision.bWantFire = true;
			break;
		case ESBBotAction::PursueFire:
			OutDecision.MoveTarget = Target;
			OutDecision.bWantFire = true;
			break;
		case ESBBotAction::Retreat:
			OutDecision.bRetreat = true;
			OutDecision.MoveTarget = Target; // steer away from this
			OutDecision.bWantFire = false;
			break;
		case ESBBotAction::Pursue:
		default:
			OutDecision.MoveTarget = Target;
			OutDecision.bWantFire = false;
			break;
		}
	};

	auto InRangeOk = [](float Required, float Actual) -> bool
	{
		return Required <= 0.f || Actual <= Required;
	};

	for (const FSBBotRule& Rule : Script.Rules)
	{
		if (Rule.Target == ESBBotTargetKind::Enemy)
		{
			if (!Facts.NearestEnemy || !InRangeOk(Rule.InRange, Facts.EnemyDistance))
			{
				continue;
			}
			OutDecision.MatchedTarget = ESBBotTargetKind::Enemy;
			ApplyAction(Rule.Action, Facts.NearestEnemy);
			return true;
		}
		if (Rule.Target == ESBBotTargetKind::AllyHurt)
		{
			if (!Facts.HurtAlly || !InRangeOk(Rule.InRange, Facts.AllyDistance))
			{
				continue;
			}
			OutDecision.MatchedTarget = ESBBotTargetKind::AllyHurt;
			ApplyAction(Rule.Action == ESBBotAction::PursueFire ? ESBBotAction::Pursue : Rule.Action, Facts.HurtAlly);
			OutDecision.bWantFire = false;
			return true;
		}
		if (Rule.Target == ESBBotTargetKind::Capture)
		{
			if (!Facts.OpenCapture)
			{
				continue;
			}
			OutDecision.MatchedTarget = ESBBotTargetKind::Capture;
			ApplyAction(Rule.Action == ESBBotAction::Hold ? ESBBotAction::Hold : ESBBotAction::Pursue, Facts.OpenCapture);
			OutDecision.bWantFire = false;
			return true;
		}
		if (Rule.Target == ESBBotTargetKind::Objective)
		{
			if (!Facts.Objective)
			{
				continue;
			}
			OutDecision.MatchedTarget = ESBBotTargetKind::Objective;
			ApplyAction(Rule.Action == ESBBotAction::Hold ? ESBBotAction::Hold : ESBBotAction::Pursue, Facts.Objective);
			OutDecision.bWantFire = false;
			return true;
		}
		if (Rule.Target == ESBBotTargetKind::Structure)
		{
			if (!Facts.IntactStructure)
			{
				continue;
			}
			OutDecision.MatchedTarget = ESBBotTargetKind::Structure;
			ApplyAction(Rule.Action, Facts.IntactStructure);
			return true;
		}
		if (Rule.Target == ESBBotTargetKind::Else)
		{
			OutDecision.MatchedTarget = ESBBotTargetKind::Else;
			ApplyAction(Rule.Action, nullptr);
			return true;
		}
	}

	return false;
}
