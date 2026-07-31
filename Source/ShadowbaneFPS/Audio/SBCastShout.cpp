// Copyright shadowbanefps.

#include "Audio/SBCastShout.h"
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	struct FCachedPCM
	{
		TArray<uint8> Samples;
		int32 SampleRate = 22050;
		int32 NumChannels = 1;
		float Duration = 0.f;
		bool bReady = false;
	};

	FCachedPCM GPCM;

	bool ParseWavFile(const TArray<uint8>& FileBytes, FCachedPCM& Out)
	{
		if (FileBytes.Num() < 44)
		{
			return false;
		}

		auto ReadU16 = [&FileBytes](int32 Offset) -> uint16
		{
			return static_cast<uint16>(FileBytes[Offset] | (FileBytes[Offset + 1] << 8));
		};
		auto ReadU32 = [&FileBytes](int32 Offset) -> uint32
		{
			return static_cast<uint32>(FileBytes[Offset]
				| (FileBytes[Offset + 1] << 8)
				| (FileBytes[Offset + 2] << 16)
				| (FileBytes[Offset + 3] << 24));
		};

		if (FileBytes[0] != 'R' || FileBytes[1] != 'I' || FileBytes[2] != 'F' || FileBytes[3] != 'F'
			|| FileBytes[8] != 'W' || FileBytes[9] != 'A' || FileBytes[10] != 'V' || FileBytes[11] != 'E')
		{
			return false;
		}

		int32 Offset = 12;
		int32 DataOffset = -1;
		int32 DataSize = 0;
		int32 SampleRate = 22050;
		int32 NumChannels = 1;
		int32 BitsPerSample = 16;

		while (Offset + 8 <= FileBytes.Num())
		{
			const uint32 ChunkId = ReadU32(Offset);
			const uint32 ChunkSize = ReadU32(Offset + 4);
			const int32 ChunkData = Offset + 8;

			if (ChunkId == 0x20746D66) // "fmt "
			{
				if (ChunkData + 16 > FileBytes.Num())
				{
					return false;
				}
				NumChannels = ReadU16(ChunkData + 2);
				SampleRate = static_cast<int32>(ReadU32(ChunkData + 4));
				BitsPerSample = ReadU16(ChunkData + 14);
			}
			else if (ChunkId == 0x61746164) // "data"
			{
				DataOffset = ChunkData;
				DataSize = static_cast<int32>(ChunkSize);
				break;
			}

			Offset = ChunkData + static_cast<int32>(ChunkSize) + (ChunkSize & 1);
		}

		if (DataOffset < 0 || BitsPerSample != 16 || NumChannels < 1 || SampleRate <= 0)
		{
			return false;
		}

		const int32 End = FMath::Min(FileBytes.Num(), DataOffset + DataSize);
		const int32 CopySize = End - DataOffset;
		if (CopySize <= 0)
		{
			return false;
		}

		Out.Samples.SetNumUninitialized(CopySize);
		FMemory::Memcpy(Out.Samples.GetData(), FileBytes.GetData() + DataOffset, CopySize);
		Out.SampleRate = SampleRate;
		Out.NumChannels = NumChannels;
		Out.Duration = static_cast<float>(CopySize) / static_cast<float>(SampleRate * NumChannels * 2);
		Out.bReady = Out.Duration > 0.01f;
		return Out.bReady;
	}

	void SynthesizeYellPCM(FCachedPCM& Out)
	{
		// Fallback facsimile: two-beat theurgic AH–TAH (Ateh / Atah) vibration.
		constexpr int32 SampleRate = 22050;
		constexpr float DurationSec = 1.15f;
		const int32 NumSamples = static_cast<int32>(SampleRate * DurationSec);
		Out.SampleRate = SampleRate;
		Out.NumChannels = 1;
		Out.Duration = DurationSec;
		Out.Samples.SetNumUninitialized(NumSamples * sizeof(int16));

		int16* Dest = reinterpret_cast<int16*>(Out.Samples.GetData());
		FRandomStream Rng(1337);
		for (int32 i = 0; i < NumSamples; ++i)
		{
			const float T = static_cast<float>(i) / static_cast<float>(SampleRate);
			const bool bSecondSyllable = T >= 0.48f;
			const float LocalT = bSecondSyllable ? (T - 0.48f) : T;
			const float SyllableLen = bSecondSyllable ? 0.67f : 0.48f;
			const float Env = FMath::Clamp(LocalT / 0.05f, 0.f, 1.f)
				* FMath::Clamp((SyllableLen - LocalT) / 0.12f, 0.f, 1.f);
			const float F0 = bSecondSyllable ? 310.f : 260.f;
			const float Vibrato = 1.f + 0.03f * FMath::Sin(2.f * PI * 5.5f * T);
			const float Tone = FMath::Sin(2.f * PI * F0 * Vibrato * T)
				+ 0.5f * FMath::Sin(2.f * PI * F0 * 2.f * Vibrato * T)
				+ 0.22f * FMath::Sin(2.f * PI * F0 * 3.f * Vibrato * T);
			const float Noise = (Rng.FRand() * 2.f - 1.f) * 0.2f;
			const float Sample = FMath::Clamp((Tone + Noise) * Env * 0.62f, -1.f, 1.f);
			Dest[i] = static_cast<int16>(Sample * 28000.f);
		}
		Out.bReady = true;
	}

	void EnsurePCMLoaded()
	{
		if (GPCM.bReady)
		{
			return;
		}

		const FString WavPath = FPaths::ProjectContentDir() / TEXT("Audio/SB_CastShout.wav");
		TArray<uint8> FileBytes;
		if (FPaths::FileExists(WavPath) && FFileHelper::LoadFileToArray(FileBytes, *WavPath) && ParseWavFile(FileBytes, GPCM))
		{
			return;
		}

		SynthesizeYellPCM(GPCM);
	}
}

USoundBase* SBCastShout::GetOrCreateShout(UObject* WorldContextObject)
{
	EnsurePCMLoaded();
	UObject* Outer = WorldContextObject;
	if (!Outer)
	{
		Outer = GetTransientPackage();
	}

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(Outer, NAME_None, RF_Transient);
	Wave->SetSampleRate(GPCM.SampleRate);
	Wave->NumChannels = GPCM.NumChannels;
	Wave->Duration = GPCM.Duration;
	Wave->SoundGroup = SOUNDGROUP_Voice;
	Wave->bLooping = false;
	Wave->bProcedural = true;
	Wave->QueueAudio(GPCM.Samples.GetData(), GPCM.Samples.Num());
	return Wave;
}

void SBCastShout::PlayAt(UWorld* World, const FVector& Location)
{
	if (!World)
	{
		return;
	}

	if (USoundBase* Shout = GetOrCreateShout(World))
	{
		UGameplayStatics::PlaySoundAtLocation(
			World,
			Shout,
			Location,
			1.2f,
			0.92f + FMath::FRandRange(-0.05f, 0.05f));
	}
}
