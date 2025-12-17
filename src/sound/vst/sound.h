#pragma once

#include "../soundbase.h"
#include "../../global.h"
#include <vector>
#include <mutex>

class CSound : public CSoundBase
{
    Q_OBJECT

public:
    CSound ( void ( *fpNewProcessCallback ) ( CVector<int16_t>& psData, void* arg ),
             void*          arg,
             const QString& strMIDISetup,
             const bool     bNoAutoJackConnect,
             const QString& strClientName );

    virtual ~CSound();

    virtual int  Init ( const int iNewPrefMonoBufferSize ) override;
    virtual void Start() override;
    virtual void Stop() override;
    virtual float GetInOutLatencyMs() override { return 0.0f; }

    void ProcessAudioBlockVST ( const float* const* inputChannelData,
                                int numInputChannels,
                                float* const* outputChannelData,
                                int numOutputChannels,
                                int numSamples );

private:
    int iBlockSize;
    bool bInitialized;

    std::vector<float> inputFifo;
    std::vector<float> outputFifo;
    size_t inReadIndex;
    size_t inWriteIndex;
    size_t inCount;

    size_t outReadIndex;
    size_t outWriteIndex;
    size_t outCount;

    size_t fifoSize;

    CVector<int16_t> conversionBuffer;

    std::mutex audioMutex;
};
