#include "sound.h"
#include <algorithm>
#include <cmath>

CSound::CSound ( void ( *fpNewProcessCallback ) ( CVector<int16_t>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool     bNoAutoJackConnect,
                 const QString& strClientName ) :
    CSoundBase ( "VST", fpNewProcessCallback, arg, strMIDISetup ),
    iBlockSize ( 128 ),
    bInitialized ( false ),
    inReadIndex ( 0 ),
    inWriteIndex ( 0 ),
    inCount ( 0 ),
    outReadIndex ( 0 ),
    outWriteIndex ( 0 ),
    outCount ( 0 ),
    fifoSize ( 65536 )
{
    inputFifo.resize ( fifoSize * 2, 0.0f );
    outputFifo.resize ( fifoSize * 2, 0.0f );
}

CSound::~CSound()
{
    Stop();
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    std::lock_guard<std::mutex> lock(audioMutex);
    iBlockSize = iNewPrefMonoBufferSize;
    conversionBuffer.Init ( iBlockSize * 2 );
    bInitialized = true;
    return iBlockSize;
}

void CSound::Start()
{
    std::lock_guard<std::mutex> lock(audioMutex);
    CSoundBase::Start();
    inReadIndex = 0;
    inWriteIndex = 0;
    inCount = 0;
    outReadIndex = 0;
    outWriteIndex = 0;
    outCount = 0;
    std::fill(inputFifo.begin(), inputFifo.end(), 0.0f);
    std::fill(outputFifo.begin(), outputFifo.end(), 0.0f);
}

void CSound::Stop()
{
    std::lock_guard<std::mutex> lock(audioMutex);
    CSoundBase::Stop();
}

void CSound::ProcessAudioBlockVST ( const float* const* inputChannelData,
                                    int numInputChannels,
                                    float* const* outputChannelData,
                                    int numOutputChannels,
                                    int numSamples )
{
    if ( !bRun || !bInitialized )
    {
        for ( int ch = 0; ch < numOutputChannels; ++ch )
        {
            if ( outputChannelData[ch] )
            {
                std::fill ( outputChannelData[ch], outputChannelData[ch] + numSamples, 0.0f );
            }
        }
        return;
    }

    // 1. Push Input
    for ( int i = 0; i < numSamples; ++i )
    {
        float left = ( numInputChannels > 0 ) ? inputChannelData[0][i] : 0.0f;
        float right = ( numInputChannels > 1 ) ? inputChannelData[1][i] : left;

        size_t idx = inWriteIndex * 2;
        inputFifo[idx] = left;
        inputFifo[idx + 1] = right;

        inWriteIndex = ( inWriteIndex + 1 ) & ( fifoSize - 1 );
        inCount++;
    }

    // 2. Process chunks
    while ( inCount >= (size_t)iBlockSize )
    {
        // Read from Input
        for ( int i = 0; i < iBlockSize; ++i )
        {
            size_t idx = inReadIndex * 2;
            float fl = inputFifo[idx];
            float fr = inputFifo[idx + 1];

            auto f2s = [](float f) -> int16_t {
                if (f > 1.0f) f = 1.0f;
                if (f < -1.0f) f = -1.0f;
                return static_cast<int16_t>(f * 32767.0f);
            };

            conversionBuffer[i * 2] = f2s(fl);
            conversionBuffer[i * 2 + 1] = f2s(fr);

            inReadIndex = ( inReadIndex + 1 ) & ( fifoSize - 1 );
        }
        inCount -= iBlockSize;

        // Process
        ProcessCallback ( conversionBuffer );

        // Write to Output FIFO
        for ( int i = 0; i < iBlockSize; ++i )
        {
            int16_t sl = conversionBuffer[i * 2];
            int16_t sr = conversionBuffer[i * 2 + 1];

            auto s2f = [](int16_t s) -> float {
                return static_cast<float>(s) / 32768.0f;
            };

            size_t idx = outWriteIndex * 2;
            outputFifo[idx] = s2f(sl);
            outputFifo[idx + 1] = s2f(sr);

            outWriteIndex = ( outWriteIndex + 1 ) & ( fifoSize - 1 );
        }
        outCount += iBlockSize;
    }

    // 3. Pop Output
    for ( int i = 0; i < numSamples; ++i )
    {
        float left = 0.0f;
        float right = 0.0f;

        if ( outCount > 0 )
        {
            size_t idx = outReadIndex * 2;
            left = outputFifo[idx];
            right = outputFifo[idx + 1];

            outReadIndex = ( outReadIndex + 1 ) & ( fifoSize - 1 );
            outCount--;
        }

        if ( numOutputChannels > 0 && outputChannelData[0] ) outputChannelData[0][i] = left;
        if ( numOutputChannels > 1 && outputChannelData[1] ) outputChannelData[1][i] = right;
    }
}
