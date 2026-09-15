/******************************************************************************\
* Audio Dynamic Equalizer                                                      *
\******************************************************************************/

#pragma once

#include "util.h"

class CAudioEqualizer
{
public:
    static constexpr int NUM_BANDS = 8;

    enum class EFilterType
    {
        Peak = 0,
        LowShelf,
        HighShelf,
        LowPass,
        HighPass,
        Notch
    };

    enum class EDynMode
    {
        Compress = 0,
        Boost
    };

    CAudioEqualizer();

    void Init ( const int iSampleRateHz );
    void SetBypass ( const bool bNBypass )
    {
        bBypass       = bNBypass;
        fWetMixTarget = bNBypass ? 0.0f : 1.0f;
    }
    bool GetBypass() const { return bBypass; }

    // Filter Type
    void        SetBandFilterType ( const int iBand, const EFilterType eType );
    EFilterType GetBandFilterType ( const int iBand ) const
    {
        return ( iBand >= 0 && iBand < NUM_BANDS ) ? aeBandFilterType[iBand] : EFilterType::Peak;
    }

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
    void SetBandDynMode ( const int iBand, const EDynMode eMode );
    void SetBandDynThresholdDb ( const int iBand, const float fDb );
    void SetBandDynRatio ( const int iBand, const float fRatio );
    void SetBandDynAttackMs ( const int iBand, const float fMs );
    void SetBandDynReleaseMs ( const int iBand, const float fMs );

    // Per-band dynamics getters
    bool     GetBandDynEnabled ( const int iBand ) const;
    EDynMode GetBandDynMode ( const int iBand ) const;
    float    GetBandDynThresholdDb ( const int iBand ) const;
    float    GetBandDynRatio ( const int iBand ) const;
    float    GetBandDynAttackMs ( const int iBand ) const;
    float    GetBandDynReleaseMs ( const int iBand ) const;

    // Real-time gain reduction/boost readback for the GUI curve display
    float GetBandGainReductionDb ( const int iBand ) const;

    // Per-band Q (quality factor / bandwidth)
    void  SetBandQ ( const int iBand, const float fQ );
    float GetBandQ ( const int iBand ) const;

    // Band frequency information (dynamic, for curve widget)
    void         SetBandFrequency ( const int iBand, const float fFreqHz );
    float        GetBandFrequency ( const int iBand ) const;
    static float GetDefaultBandFrequency ( const int iBand );

    // Band Solo / Auditioning (-1 = no solo, 0..NUM_BANDS-1 = solo that band)
    void SetSoloBand ( const int iBand );
    int  GetSoloBand() const { return iSoloBand; }

    // Band Mute / Bypass
    void SetBandMute ( const int iBand, const bool bMute )
    {
        if ( ( iBand >= 0 ) && ( iBand < NUM_BANDS ) )
        {
            abBandMute[iBand] = bMute;
        }
    }
    bool GetBandMute ( const int iBand ) const { return ( iBand >= 0 && iBand < NUM_BANDS ) ? abBandMute[iBand] : false; }

    void Reset();
    void Process ( CVector<int16_t>& vecsStereoInOut, const int iStereoBlockSizeSam );

private:
    struct SSvfCoeff
    {
        float g;
        float a1;
        float a2;
        float m0;
        float m1;
        float m2;
    };

    struct SSvfState
    {
        float ic1eq[2];
        float ic2eq[2];
    };

    struct SDynParams
    {
        bool     bEnabled;
        EDynMode eMode;
        float    fThresholdDb;
        float    fRatio;
        float    fAttackMs;
        float    fReleaseMs;
    };

    float       afBandFrequencies[NUM_BANDS];
    float       afBandQ[NUM_BANDS];
    EFilterType aeBandFilterType[NUM_BANDS];

    void UpdateBandCoeff ( const int iBandIndex, const float fGainDb, const float fQ );
    void UpdateDetCoeff ( const int iBandIndex, const float fQ );
    void ClearFilterState();

    bool  bBypass;
    float fWetMixCurrent;
    float fWetMixTarget;
    int   iSampleRateHz;
    int   iSoloBand;
    bool  abBandMute[NUM_BANDS];

    // Static gain (user-set)
    float afBandTargetGainDb[NUM_BANDS];
    float afBandSmoothedGainDb[NUM_BANDS];

    // Cytomic SVF EQ filter
    SSvfCoeff aBandCoeff[NUM_BANDS];
    SSvfState aBandState[NUM_BANDS];

    // Cytomic SVF Detector bandpass filter
    SSvfCoeff aDetCoeff[NUM_BANDS];
    SSvfState aDetState[NUM_BANDS];

    // Per-band dynamics
    SDynParams aBandDynParams[NUM_BANDS];
    float      afDetEnvelope[NUM_BANDS];
    float      afBandGainReductionDb[NUM_BANDS];
    float      afBandEffectiveGainDb[NUM_BANDS];
};
