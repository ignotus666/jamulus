/******************************************************************************\
* Audio Compressor                                                            *
\******************************************************************************/

#pragma once

#include "util.h"

#include <atomic>

class CAudioCompressor
{
public:
    CAudioCompressor();

    void Init ( const int iNSampleRateHz );
    void SetBypass ( const bool bNBypass ) { bBypass = bNBypass; }
    bool GetBypass() const { return bBypass; }

    void SetThresholdDb ( const float fDb ) { fThresholdDb = fDb; }
    void SetRatio ( const float fValue ) { fRatio = fValue; }
    void SetAttackMs ( const float fMs ) { fAttackMs = fMs; }
    void SetReleaseMs ( const float fMs ) { fReleaseMs = fMs; }
    void SetMakeupDb ( const float fDb ) { fMakeupDb = fDb; }
    void SetLimiterEnabled ( const bool bEnabled ) { bLimiterEnabled = bEnabled; }

    float GetThresholdDb() const { return fThresholdDb; }
    float GetRatio() const { return fRatio; }
    float GetAttackMs() const { return fAttackMs; }
    float GetReleaseMs() const { return fReleaseMs; }
    float GetMakeupDb() const { return fMakeupDb; }
    bool  GetLimiterEnabled() const { return bLimiterEnabled; }
    float GetGainReductionDb();

    void Process ( CVector<int16_t>& vecsStereoInOut, const int iStereoBlockSizeSam );

private:
    float DbToLinear ( const float fDb ) const;
    float LinearToDb ( const float fValue ) const;
    float ComputeGainDb ( const float fInputDb ) const;

    bool               bBypass;
    bool               bLimiterEnabled;
    int                iSampleRateHz;
    float              fThresholdDb;
    float              fRatio;
    float              fAttackMs;
    float              fReleaseMs;
    float              fMakeupDb;
    float              fEnvelope;
    float              fKneeDb;
    float              fLimiterCeilDb;
    std::atomic<float> fGainReductionDb;
};
