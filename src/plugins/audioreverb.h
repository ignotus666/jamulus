/******************************************************************************\
* Audio Reverberation                                                          *
\******************************************************************************/
/*
    The following code is based on "JCRev: John Chowning's reverberator class"
    by Perry R. Cook and Gary P. Scavone, 1995 - 2004
    which is in "The Synthesis ToolKit in C++ (STK)"
    http://ccrma.stanford.edu/software/stk

    Original description:
    This class is derived from the CLM JCRev function, which is based on the use
    of networks of simple allpass and comb delay filters. This class implements
    three series allpass units, followed by four parallel comb filters, and two
    decorrelation delay lines in parallel at the output.
*/

#pragma once
#include "util.h"

struct SReverbParams
{
    float fWet;
    float fDry;
    float fEarlyLevel;
    float fWidth;
    float fT60;
    float fDamping;
    int   iPreDelayMs;
    bool  bEarlyEnabled;
    bool  bFreeze;
};

class CAudioReverb
{
public:
    CAudioReverb() {}

    void Init ( const EAudChanConf eNAudioChannelConf, const int iNStereoBlockSizeSam, const int iNSampleRate, const float fT60 = 1.1f );

    void Clear();
    void Process ( CVector<int16_t>& vecsStereoInOut, const bool bReverbOnLeftChan, const SReverbParams& sParams );

protected:
    void setT60 ( const float fT60, const int iSampleRate );
    bool isPrime ( const int number );
    void UpdatePreDelayBuffer ( const int iPreDelayMs );
    void UpdateEarlyTapSamples();
    int  WrapIndex ( const int iIndex, const int iSize ) const;

    class COnePole
    {
    public:
        COnePole() : fA ( 0 ), fB ( 0 ) { Reset(); }
        void  setPole ( const float fPole );
        float Calc ( const float fIn );
        void  Reset() { fLastSample = 0; }

    protected:
        float fA;
        float fB;
        float fLastSample;
    };

    EAudChanConf eAudioChannelConf;
    int          iStereoBlockSizeSam;
    int          iSampleRate;
    CFIFO<float> allpassDelays[3];
    CFIFO<float> combDelays[4];
    COnePole     combFilters[4];
    CFIFO<float> outLeftDelay;
    CFIFO<float> outRightDelay;
    float        allpassCoefficient;
    float        combCoefficient[4];
    CVector<float> vecPreDelayBuffer;
    int            iPreDelaySamples;
    int            iPreDelayWriteIndex;
    float          fLastT60;
    float          fLastDamping;
    float          fWet;
    float          fDry;
    float          fEarlyLevel;
    float          fWidth;
    bool           bEarlyEnabled;
    bool           bFreeze;
    int            aiEarlyTapSamples[4];
    float          afEarlyTapGains[4];
    int            iEarlyTapCount;
};
