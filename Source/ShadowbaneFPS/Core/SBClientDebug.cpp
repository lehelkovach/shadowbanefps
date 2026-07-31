// Copyright shadowbanefps.

#include "SBClientDebug.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

TArray<FSBClientDebug::FEntry> FSBClientDebug::Messages;

bool FSBClientDebug::IsEnabled()
{
	const TCHAR* Cmd = FCommandLine::Get();
	return FParse::Param(Cmd, TEXT("SBDebug")) || FParse::Param(Cmd, TEXT("SBVerbose"));
}

bool FSBClientDebug::IsVerbose()
{
	return FParse::Param(FCommandLine::Get(), TEXT("SBVerbose"));
}

void FSBClientDebug::PushMessage(const FString& Message, float SecondsToLive)
{
	if (Message.IsEmpty())
	{
		return;
	}

	FEntry Entry;
	Entry.Text = Message;
	Entry.Remaining = FMath::Max(0.5f, SecondsToLive);
	Messages.Insert(Entry, 0);
	while (Messages.Num() > 12)
	{
		Messages.RemoveAt(Messages.Num() - 1);
	}
}

void FSBClientDebug::TickMessages(float DeltaSeconds)
{
	for (int32 i = Messages.Num() - 1; i >= 0; --i)
	{
		Messages[i].Remaining -= DeltaSeconds;
		if (Messages[i].Remaining <= 0.f)
		{
			Messages.RemoveAt(i);
		}
	}
}

void FSBClientDebug::GetActiveMessages(TArray<FString>& OutMessages)
{
	OutMessages.Reset();
	for (const FEntry& Entry : Messages)
	{
		OutMessages.Add(Entry.Text);
	}
}
