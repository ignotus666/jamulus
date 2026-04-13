/******************************************************************************\
* Audio Equalizer                                                             *
\******************************************************************************/

#include "audioequalizer.h"
#include <cmath>

const float CAudioEqualizer::afBandFrequencies[NUM_BANDS] = { 63.0f, 89.0f, 125.0f, 177.0f, 250.0f, 354.0f, 500.0f, 707.0f,
                                                             1000.0f, 1400.0f, 2000.0f, 2800.0f, 4000.0f, 5600.0f, 8000.0f, 11200.0f };

CAudioEqualizer::CAudioEqualizer() :
    bBypass ( false ),
    fWetMixCurrent ( 1.0f ),
    fWetMixTarget ( 1.0f ),
    iSampleRateHz ( SYSTEM_SAMPLE_RATE_HZ )
{
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        afBandTargetGainDb[iBand]  = 0.0f;
        afBandCurrentGainDb[iBand] = 0.0f;
        aBandCoeff[iBand]   = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    }

    ClearFilterState();
}

void CAudioEqualizer::Init ( const int iNsampleRateHz )
{
    iSampleRateHz = iNsampleRateHz;
    fWetMixCurrent = bBypass ? 0.0f : 1.0f;
    fWetMixTarget  = fWetMixCurrent;

    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        UpdateBandCoeff ( iBand, afBandCurrentGainDb[iBand] );
    }

    ClearFilterState();
}

void CAudioEqualizer::SetBandGainDb ( const int iBandIndex, const float fGainDb )
{
    if ( ( iBandIndex < 0 ) || ( iBandIndex >= NUM_BANDS ) )
    {
        return;
    }

    afBandTargetGainDb[iBandIndex] = fGainDb;
}

void CAudioEqualizer::Reset()
{
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        afBandTargetGainDb[iBand]  = 0.0f;
        afBandCurrentGainDb[iBand] = 0.0f;
        UpdateBandCoeff ( iBand, 0.0f );
    }

    ClearFilterState();
}

void CAudioEqualizer::Process ( CVector<int16_t>& vecsStereoInOut, const int iStereoBlockSizeSam )
{
    // Smooth target gain changes once per block to avoid zipper noise.
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        const float fDiff = afBandTargetGainDb[iBand] - afBandCurrentGainDb[iBand];

        if ( std::fabs ( fDiff ) > 0.001f )
        {
            afBandCurrentGainDb[iBand] += fDiff * 0.2f;

            if ( std::fabs ( afBandTargetGainDb[iBand] - afBandCurrentGainDb[iBand] ) < 0.001f )
            {
                afBandCurrentGainDb[iBand] = afBandTargetGainDb[iBand];
            }

            UpdateBandCoeff ( iBand, afBandCurrentGainDb[iBand] );
        }
    }

    const int iFrameCount = iStereoBlockSizeSam / 2;
    const float fWetStep  = ( iFrameCount > 0 ) ? ( fWetMixTarget - fWetMixCurrent ) / iFrameCount : 0.0f;

    for ( int iSample = 0; iSample < iStereoBlockSizeSam; iSample += 2 )
    {
        for ( int iChannel = 0; iChannel < 2; ++iChannel )
        {
            const float fDry = vecsStereoInOut[iSample + iChannel];
            float       fSample = fDry;

            for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
            {
                const SCoeff& c = aBandCoeff[iBand];
                SState&       s = aBandState[iBand];

                const float fOut = c.b0 * fSample + c.b1 * s.x1[iChannel] + c.b2 * s.x2[iChannel] - c.a1 * s.y1[iChannel] - c.a2 * s.y2[iChannel];

                s.x2[iChannel] = s.x1[iChannel];
                s.x1[iChannel] = fSample;
                s.y2[iChannel] = s.y1[iChannel];
                s.y1[iChannel] = fOut;

                fSample = fOut;
            }

            const float fMixed = fDry + ( fSample - fDry ) * fWetMixCurrent;
            vecsStereoInOut[iSample + iChannel] = Float2Short ( fMixed );
        }

        fWetMixCurrent += fWetStep;
    }

    if ( std::fabs ( fWetMixTarget - fWetMixCurrent ) < 0.0001f )
    {
        fWetMixCurrent = fWetMixTarget;
    }
}

void CAudioEqualizer::UpdateBandCoeff ( const int iBandIndex, const float fGainDb )
{
    // RBJ peaking EQ biquad.
    constexpr float fPi = 3.14159265358979323846f;
    constexpr float fQ = 1.0f;

    const float fA     = std::pow ( 10.0f, fGainDb / 40.0f );
    const float fW0    = 2.0f * fPi * afBandFrequencies[iBandIndex] / iSampleRateHz;
    const float fAlpha = std::sin ( fW0 ) / ( 2.0f * fQ );
    const float fCosW0 = std::cos ( fW0 );

    const float b0 = 1.0f + fAlpha * fA;
    const float b1 = -2.0f * fCosW0;
    const float b2 = 1.0f - fAlpha * fA;
    const float a0 = 1.0f + fAlpha / fA;
    const float a1 = -2.0f * fCosW0;
    const float a2 = 1.0f - fAlpha / fA;

    aBandCoeff[iBandIndex].b0 = b0 / a0;
    aBandCoeff[iBandIndex].b1 = b1 / a0;
    aBandCoeff[iBandIndex].b2 = b2 / a0;
    aBandCoeff[iBandIndex].a1 = a1 / a0;
    aBandCoeff[iBandIndex].a2 = a2 / a0;
}

void CAudioEqualizer::ClearFilterState()
{
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        for ( int iChannel = 0; iChannel < 2; ++iChannel )
        {
            aBandState[iBand].x1[iChannel] = 0.0f;
            aBandState[iBand].x2[iChannel] = 0.0f;
            aBandState[iBand].y1[iChannel] = 0.0f;
            aBandState[iBand].y2[iChannel] = 0.0f;
        }
    }
}
