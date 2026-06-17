/******************************************************************************\
* Audio Dynamic Equalizer                                                      *
\******************************************************************************/

#include "audioequalizer.h"
#include <cmath>

CAudioEqualizer::CAudioEqualizer() : bBypass ( true ), fWetMixCurrent ( 0.0f ), fWetMixTarget ( 0.0f ), iSampleRateHz ( SYSTEM_SAMPLE_RATE_HZ )
{
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        afBandFrequencies[iBand]     = GetDefaultBandFrequency ( iBand );
        afBandQ[iBand]               = 1.0f;
        afBandTargetGainDb[iBand]    = 0.0f;
        afBandSmoothedGainDb[iBand]  = 0.0f;
        afBandEffectiveGainDb[iBand] = 0.0f;
        aBandCoeff[iBand]            = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        aDetCoeff[iBand]             = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

        aBandDynParams[iBand].bEnabled     = false;
        aBandDynParams[iBand].fThresholdDb = -20.0f;
        aBandDynParams[iBand].fRatio       = 4.0f;
        aBandDynParams[iBand].fAttackMs    = 5.0f;
        aBandDynParams[iBand].fReleaseMs   = 80.0f;

        afDetEnvelope[iBand]         = 0.0f;
        afBandGainReductionDb[iBand] = 0.0f;
    }

    ClearFilterState();
}

void CAudioEqualizer::Init ( const int iNsampleRateHz )
{
    iSampleRateHz  = iNsampleRateHz;
    fWetMixCurrent = bBypass ? 0.0f : 1.0f;
    fWetMixTarget  = fWetMixCurrent;

    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        UpdateBandCoeff ( iBand, afBandSmoothedGainDb[iBand], afBandQ[iBand] );
        UpdateDetCoeff ( iBand, afBandQ[iBand] );
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

void CAudioEqualizer::SetBandDynEnabled ( const int iBand, const bool bEnabled )
{
    if ( ( iBand >= 0 ) && ( iBand < NUM_BANDS ) )
    {
        aBandDynParams[iBand].bEnabled = bEnabled;

        if ( !bEnabled )
        {
            afDetEnvelope[iBand]         = 0.0f;
            afBandGainReductionDb[iBand] = 0.0f;
        }
    }
}

void CAudioEqualizer::SetBandDynThresholdDb ( const int iBand, const float fDb )
{
    if ( ( iBand >= 0 ) && ( iBand < NUM_BANDS ) )
    {
        aBandDynParams[iBand].fThresholdDb = fDb;
    }
}

void CAudioEqualizer::SetBandDynRatio ( const int iBand, const float fRatio )
{
    if ( ( iBand >= 0 ) && ( iBand < NUM_BANDS ) )
    {
        aBandDynParams[iBand].fRatio = std::max ( 1.0f, fRatio );
    }
}

void CAudioEqualizer::SetBandDynAttackMs ( const int iBand, const float fMs )
{
    if ( ( iBand >= 0 ) && ( iBand < NUM_BANDS ) )
    {
        aBandDynParams[iBand].fAttackMs = std::max ( 0.1f, fMs );
    }
}

void CAudioEqualizer::SetBandDynReleaseMs ( const int iBand, const float fMs )
{
    if ( ( iBand >= 0 ) && ( iBand < NUM_BANDS ) )
    {
        aBandDynParams[iBand].fReleaseMs = std::max ( 1.0f, fMs );
    }
}

bool CAudioEqualizer::GetBandDynEnabled ( const int iBand ) const
{
    return ( iBand >= 0 && iBand < NUM_BANDS ) ? aBandDynParams[iBand].bEnabled : false;
}

float CAudioEqualizer::GetBandDynThresholdDb ( const int iBand ) const
{
    return ( iBand >= 0 && iBand < NUM_BANDS ) ? aBandDynParams[iBand].fThresholdDb : -20.0f;
}

float CAudioEqualizer::GetBandDynRatio ( const int iBand ) const { return ( iBand >= 0 && iBand < NUM_BANDS ) ? aBandDynParams[iBand].fRatio : 4.0f; }

float CAudioEqualizer::GetBandDynAttackMs ( const int iBand ) const
{
    return ( iBand >= 0 && iBand < NUM_BANDS ) ? aBandDynParams[iBand].fAttackMs : 5.0f;
}

float CAudioEqualizer::GetBandDynReleaseMs ( const int iBand ) const
{
    return ( iBand >= 0 && iBand < NUM_BANDS ) ? aBandDynParams[iBand].fReleaseMs : 80.0f;
}

float CAudioEqualizer::GetBandGainReductionDb ( const int iBand ) const
{
    return ( iBand >= 0 && iBand < NUM_BANDS ) ? afBandGainReductionDb[iBand] : 0.0f;
}

float CAudioEqualizer::GetBandFrequency ( const int iBand ) const { return ( iBand >= 0 && iBand < NUM_BANDS ) ? afBandFrequencies[iBand] : 0.0f; }

float CAudioEqualizer::GetDefaultBandFrequency ( const int iBand )
{
    static const float afDefaultBandFrequencies[NUM_BANDS] = { 63.0f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f };
    return ( iBand >= 0 && iBand < NUM_BANDS ) ? afDefaultBandFrequencies[iBand] : 0.0f;
}

void CAudioEqualizer::SetBandFrequency ( const int iBand, const float fFreqHz )
{
    if ( ( iBand >= 0 ) && ( iBand < NUM_BANDS ) )
    {
        afBandFrequencies[iBand] = std::max ( 20.0f, std::min ( 20000.0f, fFreqHz ) );
        UpdateBandCoeff ( iBand, afBandSmoothedGainDb[iBand], afBandQ[iBand] );
        UpdateDetCoeff ( iBand, afBandQ[iBand] );
    }
}

void CAudioEqualizer::SetBandQ ( const int iBand, const float fQ )
{
    if ( ( iBand >= 0 ) && ( iBand < NUM_BANDS ) )
    {
        afBandQ[iBand] = std::max ( 0.3f, std::min ( 10.0f, fQ ) );
        UpdateBandCoeff ( iBand, afBandSmoothedGainDb[iBand], afBandQ[iBand] );
        UpdateDetCoeff ( iBand, afBandQ[iBand] );
    }
}

float CAudioEqualizer::GetBandQ ( const int iBand ) const { return ( iBand >= 0 && iBand < NUM_BANDS ) ? afBandQ[iBand] : 1.0f; }

void CAudioEqualizer::Reset()
{
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        afBandFrequencies[iBand]     = GetDefaultBandFrequency ( iBand );
        afBandQ[iBand]               = 1.0f;
        afBandTargetGainDb[iBand]    = 0.0f;
        afBandSmoothedGainDb[iBand]  = 0.0f;
        afBandEffectiveGainDb[iBand] = 0.0f;
        UpdateBandCoeff ( iBand, 0.0f, afBandQ[iBand] );
        UpdateDetCoeff ( iBand, afBandQ[iBand] );

        aBandDynParams[iBand].bEnabled     = false;
        aBandDynParams[iBand].fThresholdDb = -20.0f;
        aBandDynParams[iBand].fRatio       = 4.0f;
        aBandDynParams[iBand].fAttackMs    = 5.0f;
        aBandDynParams[iBand].fReleaseMs   = 80.0f;

        afDetEnvelope[iBand]         = 0.0f;
        afBandGainReductionDb[iBand] = 0.0f;
    }

    ClearFilterState();
}

void CAudioEqualizer::Process ( CVector<int16_t>& vecsStereoInOut, const int iStereoBlockSizeSam )
{
    // --- Step 1: Compute per-band gain reduction from previous block's detector envelopes ---
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        if ( aBandDynParams[iBand].bEnabled && afDetEnvelope[iBand] > 0.0f )
        {
            // Convert envelope to dB (envelope is in sample-value units, normalise to 0 dBFS = 32768)
            const float fInputDb = 20.0f * std::log10 ( std::max ( afDetEnvelope[iBand] / 32768.0f, 1e-8f ) );

            if ( fInputDb > aBandDynParams[iBand].fThresholdDb )
            {
                const float fOverDb          = fInputDb - aBandDynParams[iBand].fThresholdDb;
                afBandGainReductionDb[iBand] = fOverDb * ( 1.0f - 1.0f / aBandDynParams[iBand].fRatio );
            }
            else
            {
                afBandGainReductionDb[iBand] = 0.0f;
            }
        }
        else
        {
            afBandGainReductionDb[iBand] = 0.0f;
        }
    }

    // --- Step 2: Smooth static gain and compute effective biquad gain ---
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        // Smooth user-set static gain toward target (anti-zipper)
        const float fStaticDiff = afBandTargetGainDb[iBand] - afBandSmoothedGainDb[iBand];

        if ( std::fabs ( fStaticDiff ) > 0.001f )
        {
            afBandSmoothedGainDb[iBand] += fStaticDiff * 0.2f;

            if ( std::fabs ( afBandTargetGainDb[iBand] - afBandSmoothedGainDb[iBand] ) < 0.001f )
            {
                afBandSmoothedGainDb[iBand] = afBandTargetGainDb[iBand];
            }
        }

        // Effective gain = smoothed static gain minus dynamics gain reduction
        const float fEffective = afBandSmoothedGainDb[iBand] - afBandGainReductionDb[iBand];

        // Update biquad coefficients only if effective gain actually changed
        if ( std::fabs ( fEffective - afBandEffectiveGainDb[iBand] ) > 0.001f )
        {
            afBandEffectiveGainDb[iBand] = fEffective;
            UpdateBandCoeff ( iBand, fEffective, afBandQ[iBand] );
        }
    }

    // --- Step 3: Pre-compute per-band attack/release coefficients ---
    float afAttackCoeff[NUM_BANDS];
    float afReleaseCoeff[NUM_BANDS];

    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        if ( aBandDynParams[iBand].bEnabled )
        {
            afAttackCoeff[iBand]  = std::exp ( -1.0f / ( 0.001f * aBandDynParams[iBand].fAttackMs * iSampleRateHz ) );
            afReleaseCoeff[iBand] = std::exp ( -1.0f / ( 0.001f * aBandDynParams[iBand].fReleaseMs * iSampleRateHz ) );
        }
    }

    // --- Step 4: Process audio samples ---
    const int   iFrameCount = iStereoBlockSizeSam / 2;
    const float fWetStep    = ( iFrameCount > 0 ) ? ( fWetMixTarget - fWetMixCurrent ) / iFrameCount : 0.0f;

    for ( int iSample = 0; iSample < iStereoBlockSizeSam; iSample += 2 )
    {
        // Temporary per-frame peak from detector output for envelope update
        float afDetPeak[NUM_BANDS] = {};

        for ( int iChannel = 0; iChannel < 2; ++iChannel )
        {
            const float fDry    = vecsStereoInOut[iSample + iChannel];
            float       fSample = fDry;

            for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
            {
                // --- Detector path: bandpass filter on dry (pre-EQ) input ---
                if ( aBandDynParams[iBand].bEnabled )
                {
                    const SCoeff& d  = aDetCoeff[iBand];
                    SState&       ds = aDetState[iBand];

                    const float fDetOut =
                        d.b0 * fDry + d.b1 * ds.x1[iChannel] + d.b2 * ds.x2[iChannel] - d.a1 * ds.y1[iChannel] - d.a2 * ds.y2[iChannel];

                    ds.x2[iChannel] = ds.x1[iChannel];
                    ds.x1[iChannel] = fDry;
                    ds.y2[iChannel] = ds.y1[iChannel];
                    ds.y1[iChannel] = fDetOut;

                    // Track peak across both channels for this frame
                    const float fAbs = std::fabs ( fDetOut );

                    if ( fAbs > afDetPeak[iBand] )
                    {
                        afDetPeak[iBand] = fAbs;
                    }
                }

                // --- EQ path: cascaded peaking biquad ---
                const SCoeff& c = aBandCoeff[iBand];
                SState&       s = aBandState[iBand];

                const float fOut = c.b0 * fSample + c.b1 * s.x1[iChannel] + c.b2 * s.x2[iChannel] - c.a1 * s.y1[iChannel] - c.a2 * s.y2[iChannel];

                s.x2[iChannel] = s.x1[iChannel];
                s.x1[iChannel] = fSample;
                s.y2[iChannel] = s.y1[iChannel];
                s.y1[iChannel] = fOut;

                fSample = fOut;
            }

            const float fMixed                  = fDry + ( fSample - fDry ) * fWetMixCurrent;
            vecsStereoInOut[iSample + iChannel] = Float2Short ( fMixed );
        }

        // Update per-band envelopes after processing both channels of this frame
        for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
        {
            if ( aBandDynParams[iBand].bEnabled )
            {
                if ( afDetPeak[iBand] > afDetEnvelope[iBand] )
                {
                    afDetEnvelope[iBand] = afAttackCoeff[iBand] * afDetEnvelope[iBand] + ( 1.0f - afAttackCoeff[iBand] ) * afDetPeak[iBand];
                }
                else
                {
                    afDetEnvelope[iBand] = afReleaseCoeff[iBand] * afDetEnvelope[iBand] + ( 1.0f - afReleaseCoeff[iBand] ) * afDetPeak[iBand];
                }
            }
        }

        fWetMixCurrent += fWetStep;
    }

    if ( std::fabs ( fWetMixTarget - fWetMixCurrent ) < 0.0001f )
    {
        fWetMixCurrent = fWetMixTarget;
    }
}

void CAudioEqualizer::UpdateBandCoeff ( const int iBandIndex, const float fGainDb, const float fQ )
{
    // RBJ peaking EQ biquad
    constexpr float fPi = 3.14159265358979323846f;

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

void CAudioEqualizer::UpdateDetCoeff ( const int iBandIndex, const float fQ )
{
    // RBJ bandpass filter (constant 0 dB peak gain) for frequency-selective envelope detection.
    // This isolates the frequency content around the band center for the dynamics sidechain.
    constexpr float fPi = 3.14159265358979323846f;

    const float fW0    = 2.0f * fPi * afBandFrequencies[iBandIndex] / iSampleRateHz;
    const float fAlpha = std::sin ( fW0 ) / ( 2.0f * fQ );
    const float fCosW0 = std::cos ( fW0 );

    const float b0 = fAlpha;
    const float b1 = 0.0f;
    const float b2 = -fAlpha;
    const float a0 = 1.0f + fAlpha;
    const float a1 = -2.0f * fCosW0;
    const float a2 = 1.0f - fAlpha;

    aDetCoeff[iBandIndex].b0 = b0 / a0;
    aDetCoeff[iBandIndex].b1 = b1 / a0;
    aDetCoeff[iBandIndex].b2 = b2 / a0;
    aDetCoeff[iBandIndex].a1 = a1 / a0;
    aDetCoeff[iBandIndex].a2 = a2 / a0;
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

            aDetState[iBand].x1[iChannel] = 0.0f;
            aDetState[iBand].x2[iChannel] = 0.0f;
            aDetState[iBand].y1[iChannel] = 0.0f;
            aDetState[iBand].y2[iChannel] = 0.0f;
        }

        afDetEnvelope[iBand]         = 0.0f;
        afBandGainReductionDb[iBand] = 0.0f;
    }
}
