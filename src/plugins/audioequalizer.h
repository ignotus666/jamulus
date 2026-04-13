/******************************************************************************\
* Audio Equalizer                                                             *
\******************************************************************************/

#pragma once

#include "util.h"

class CAudioEqualizer
{
public:
    static constexpr int NUM_BANDS = 16;

    CAudioEqualizer();

    void Init ( const int iSampleRateHz );
    void SetBypass ( const bool bNBypass )
    {
        bBypass       = bNBypass;
        fWetMixTarget = bNBypass ? 0.0f : 1.0f;
    }
    bool GetBypass() const { return bBypass; }
    void SetBandGainDb ( const int iBandIndex, const float fGainDb );
    float GetBandGainDb ( const int iBandIndex ) const
    {
        if ( ( iBandIndex < 0 ) || ( iBandIndex >= NUM_BANDS ) )
        {
            return 0.0f;
        }

        return afBandTargetGainDb[iBandIndex];
    }
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

    static const float afBandFrequencies[NUM_BANDS];

    void UpdateBandCoeff ( const int iBandIndex, const float fGainDb );
    void ClearFilterState();

    bool   bBypass;
    float  fWetMixCurrent;
    float  fWetMixTarget;
    int    iSampleRateHz;
    float  afBandTargetGainDb[NUM_BANDS];
    float  afBandCurrentGainDb[NUM_BANDS];
    SCoeff aBandCoeff[NUM_BANDS];
    SState aBandState[NUM_BANDS];
};
