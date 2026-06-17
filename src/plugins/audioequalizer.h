/******************************************************************************\
* Audio Dynamic Equalizer                                                      *
\******************************************************************************/

#pragma once

#include "util.h"

class CAudioEqualizer
{
public:
    static constexpr int NUM_BANDS = 8;

    CAudioEqualizer();

    void Init ( const int iSampleRateHz );
    void SetBypass ( const bool bNBypass )
    {
        bBypass       = bNBypass;
        fWetMixTarget = bNBypass ? 0.0f : 1.0f;
    }
    bool GetBypass() const { return bBypass; }

    // Static band gain (user-set node position on curve, ±12 dB)
    void  SetBandGainDb ( const int iBandIndex, const float fGainDb );
    float GetBandGainDb ( const int iBandIndex ) const
    {
        if ( ( iBandIndex < 0 ) || ( iBandIndex >= NUM_BANDS ) )
        {
            return 0.0f;
        }

        return afBandTargetGainDb[iBandIndex];
    }

    // Per-band dynamics setters
    void SetBandDynEnabled ( const int iBand, const bool bEnabled );
    void SetBandDynThresholdDb ( const int iBand, const float fDb );
    void SetBandDynRatio ( const int iBand, const float fRatio );
    void SetBandDynAttackMs ( const int iBand, const float fMs );
    void SetBandDynReleaseMs ( const int iBand, const float fMs );

    // Per-band dynamics getters
    bool  GetBandDynEnabled ( const int iBand ) const;
    float GetBandDynThresholdDb ( const int iBand ) const;
    float GetBandDynRatio ( const int iBand ) const;
    float GetBandDynAttackMs ( const int iBand ) const;
    float GetBandDynReleaseMs ( const int iBand ) const;

    // Real-time gain reduction readback for the GUI curve display
    float GetBandGainReductionDb ( const int iBand ) const;

    // Per-band Q (quality factor / bandwidth)
    void  SetBandQ ( const int iBand, const float fQ );
    float GetBandQ ( const int iBand ) const;

    // Band frequency information (dynamic, for curve widget)
    void         SetBandFrequency ( const int iBand, const float fFreqHz );
    float        GetBandFrequency ( const int iBand ) const;
    static float GetDefaultBandFrequency ( const int iBand );

    void Reset();
    void Process ( CVector<int16_t>& vecsStereoInOut, const int iStereoBlockSizeSam );

private:
    struct SCoeff
    {
        float b0;
        float b1;
        float b2;
        float a1;
        float a2;
    };

    struct SState
    {
        float x1[2];
        float x2[2];
        float y1[2];
        float y2[2];
    };

    struct SDynParams
    {
        bool  bEnabled;
        float fThresholdDb;
        float fRatio;
        float fAttackMs;
        float fReleaseMs;
    };

    float afBandFrequencies[NUM_BANDS];
    float afBandQ[NUM_BANDS];

    void UpdateBandCoeff ( const int iBandIndex, const float fGainDb, const float fQ );
    void UpdateDetCoeff ( const int iBandIndex, const float fQ );
    void ClearFilterState();

    bool  bBypass;
    float fWetMixCurrent;
    float fWetMixTarget;
    int   iSampleRateHz;

    // Static gain (user-set)
    float afBandTargetGainDb[NUM_BANDS];
    float afBandSmoothedGainDb[NUM_BANDS];

    // EQ peaking biquad
    SCoeff aBandCoeff[NUM_BANDS];
    SState aBandState[NUM_BANDS];

    // Detector bandpass biquad (for frequency-selective envelope detection)
    SCoeff aDetCoeff[NUM_BANDS];
    SState aDetState[NUM_BANDS];

    // Per-band dynamics
    SDynParams aBandDynParams[NUM_BANDS];
    float      afDetEnvelope[NUM_BANDS];
    float      afBandGainReductionDb[NUM_BANDS];
    float      afBandEffectiveGainDb[NUM_BANDS];
};
