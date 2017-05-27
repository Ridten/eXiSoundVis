// Fill out your copyright notice in the Description page of Project Settings.
#include "AudioDecompressWorker.h"
#include "SoundVisComponent.h"

#include "AudioDevice.h"
#include "Developer/TargetPlatform/Public/Interfaces/IAudioFormat.h"

#include "eXiSoundVisPrivatePCH.h"

FAudioDecompressWorker* FAudioDecompressWorker::Runnable = NULL;
int32 FAudioDecompressWorker::ThreadCounter = 0;

FAudioDecompressWorker::FAudioDecompressWorker(class USoundWave* InSoundWaveRef)
	: SoundWaveRef(InSoundWaveRef)
	, CurrentTime(0)
	, DecompressDuration(0)
	, PCMBuffer(NULL)	
	, AudioInfo(NULL)
	, Thread(NULL)
{
	if (GEngine && GEngine->GetMainAudioDevice())
	{
		AudioInfo = GEngine->GetMainAudioDevice()->CreateCompressedAudioInfo(SoundWaveRef);
	}

	// Higher overall ThreadCounter to avoid duplicated names
	FAudioDecompressWorker::ThreadCounter++;

	Thread = FRunnableThread::Create(this, *FString::Printf(TEXT("FAudioDecompressWorker%d"), FAudioDecompressWorker::ThreadCounter), 0, EThreadPriority::TPri_Normal);
}

FAudioDecompressWorker::FAudioDecompressWorker(class USoundWave* InSoundWaveRef, uint8* OutPCMBuffer, float StartTime, float Duration)
	: SoundWaveRef(InSoundWaveRef)
	, CurrentTime(StartTime)
	, DecompressDuration(Duration)
	, PCMBuffer(OutPCMBuffer)	
	, AudioInfo(NULL)
	, Thread(NULL)	
{
	if (GEngine && GEngine->GetMainAudioDevice())
	{
		AudioInfo = GEngine->GetMainAudioDevice()->CreateCompressedAudioInfo(SoundWaveRef);
	}

	// Higher overall ThreadCounter to avoid duplicated names
	FAudioDecompressWorker::ThreadCounter++;

	Thread = FRunnableThread::Create(this, *FString::Printf(TEXT("FAudioDecompressWorker%d"), FAudioDecompressWorker::ThreadCounter), 0, EThreadPriority::TPri_Normal);
}

FAudioDecompressWorker::~FAudioDecompressWorker()
{
	delete Thread;
	Thread = NULL;
}

FAudioDecompressWorker* FAudioDecompressWorker::InitializeWorker(class USoundWave* InSoundWaveRef)
{
	Runnable = new FAudioDecompressWorker(InSoundWaveRef);

	return Runnable;
}

FAudioDecompressWorker* FAudioDecompressWorker::InitializeWorker(class USoundWave* InSoundWaveRef, uint8* PCMBuffer, float StartTime, float Duration)
{
	Runnable = new FAudioDecompressWorker(InSoundWaveRef, PCMBuffer, StartTime, Duration);

	return Runnable;
}

void FAudioDecompressWorker::ShutdownWorker()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = NULL;
	}
}

bool FAudioDecompressWorker::Init()
{
	// Make sure the Worker is marked is not finished
	bIsFinished = false;

	return true;
}

uint32 FAudioDecompressWorker::Run()
{
	if (!SoundWaveRef)
	{
		return 0;
	}

	if (AudioInfo != NULL)
	{
		FSoundQualityInfo QualityInfo = { 0 };

		// Parse the audio header for the relevant information
		if (!(SoundWaveRef->ResourceData))
		{
			return 0;
		}

		if (AudioInfo->ReadCompressedInfo(SoundWaveRef->ResourceData, SoundWaveRef->ResourceSize, &QualityInfo))
		{
			FScopeCycleCounterUObject WaveObject(SoundWaveRef);

			// Extract the data
			SoundWaveRef->SampleRate = QualityInfo.SampleRate;
			SoundWaveRef->NumChannels = QualityInfo.NumChannels;

			if (QualityInfo.Duration > 0.0f)
			{
				SoundWaveRef->Duration = QualityInfo.Duration;
			}

			bool partialDecompress = false;
			uint32 PCMBufferSize = 0;
			if (DecompressDuration != 0)
				partialDecompress = true;

			if (partialDecompress)
				PCMBufferSize = DecompressDuration * SoundWaveRef->SampleRate * SoundWaveRef->NumChannels;
			else
				PCMBufferSize = SoundWaveRef->Duration * SoundWaveRef->SampleRate * SoundWaveRef->NumChannels;

			SoundWaveRef->CachedRealtimeFirstBuffer = new uint8[PCMBufferSize * 2];

			if (partialDecompress)
				AudioInfo->SeekToTime(CurrentTime);
			else
				AudioInfo->SeekToTime(0.0f);

			if (partialDecompress)
				AudioInfo->ReadCompressedData(PCMBuffer, false, PCMBufferSize * 2);
			else	
				AudioInfo->ReadCompressedData(SoundWaveRef->CachedRealtimeFirstBuffer, false, PCMBufferSize * 2);
		}
		else if (SoundWaveRef->DecompressionType == DTYPE_RealTime || SoundWaveRef->DecompressionType == DTYPE_Native)
		{
			SoundWaveRef->RemoveAudioResource();
		}

		delete AudioInfo;
	}

	return 0;
}

void FAudioDecompressWorker::Stop()
{
	StopTaskCounter.Increment();
}

void FAudioDecompressWorker::Exit()
{
	// Make sure to mark Thread as finished
	bIsFinished = true;
}

void FAudioDecompressWorker::EnsureCompletion()
{
	Stop();

	if (Thread != NULL) {

		Thread->WaitForCompletion();
	}		
}
