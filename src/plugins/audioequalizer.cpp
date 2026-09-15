/******************************************************************************\
* Audio Dynamic Equalizer                                                      *
\******************************************************************************/

#include "audioequalizer.h"
#include <cmath>
#include <algorithm>

CAudioEqualizer::CAudioEqualizer() :
    bBypass ( true ),
    fWetMixCurrent ( 0.0f ),
    fWetMixTarget ( 0.0f ),
    iSampleRateHz ( SYSTEM_SAMPLE_RATE_HZ ),
    iSoloBand ( -1 )
{
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        afBandFrequencies[iBand]     = GetDefaultBandFrequency ( iBand );
        afBandQ[iBand]               = 1.0f;
        aeBandFilterType[iBand]      = EFilterType::Peak;
        afBandTargetGainDb[iBand]    = 0.0f;
        afBandSmoothedGainDb[iBand]  = 0.0f;
        afBandEffectiveGainDb[iBand] = 0.0f;
        aBandCoeff[iBand]            = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
        aDetCoeff[iBand]             = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };

        aBandDynParams[iBand].bEnabled     = false;
        aBandDynParams[iBand].eMode        = EDynMode::Compress;
        aBandDynParams[iBand].fThresholdDb = -20.0f;
        aBandDynParams[iBand].fRatio       = 4.0f;
        aBandDynParams[iBand].fAttackMs    = 5.0f;
        aBandDynParams[iBand].fReleaseMs   = 80.0f;

        afDetEnvelope[iBand]         = 0.0f;
        afBandGainReductionDb[iBand] = 0.0f;
        abBandMute[iBand]            = false;
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

void CAudioEqualizer::SetBandFilterType ( const int iBand, const EFilterType eType )
{
    if ( ( iBand >= 0 ) && ( iBand < NUM_BANDS ) )
    {
        aeBandFilterType[iBand] = eType;
        UpdateBandCoeff ( iBand, afBandEffectiveGainDb[iBand], afBandQ[iBand] );
    }
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

void CAudioEqualizer::SetBandDynMode ( const int iBand, const EDynMode eMode )
{
    if ( ( iBand >= 0 ) && ( iBand < NUM_BANDS ) )
    {
        aBandDynParams[iBand].eMode = eMode;
    }
}

CAudioEqualizer::EDynMode CAudioEqualizer::GetBandDynMode ( const int iBand ) const
{
    return ( iBand >= 0 && iBand < NUM_BANDS ) ? aBandDynParams[iBand].eMode : EDynMode::Compress;
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
        afBandQ[iBand] = std::max ( 0.1f, std::min ( 20.0f, fQ ) );
        UpdateBandCoeff ( iBand, afBandSmoothedGainDb[iBand], afBandQ[iBand] );
        UpdateDetCoeff ( iBand, afBandQ[iBand] );
    }
}

float CAudioEqualizer::GetBandQ ( const int iBand ) const { return ( iBand >= 0 && iBand < NUM_BANDS ) ? afBandQ[iBand] : 1.0f; }

void CAudioEqualizer::SetSoloBand ( const int iBand ) { iSoloBand = ( iBand >= 0 && iBand < NUM_BANDS ) ? iBand : -1; }

void CAudioEqualizer::Reset()
{
    iSoloBand = -1;

    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        afBandFrequencies[iBand]     = GetDefaultBandFrequency ( iBand );
        afBandQ[iBand]               = 1.0f;
        aeBandFilterType[iBand]      = EFilterType::Peak;
        afBandTargetGainDb[iBand]    = 0.0f;
        afBandSmoothedGainDb[iBand]  = 0.0f;
        afBandEffectiveGainDb[iBand] = 0.0f;
        UpdateBandCoeff ( iBand, 0.0f, afBandQ[iBand] );
        UpdateDetCoeff ( iBand, afBandQ[iBand] );

        aBandDynParams[iBand].bEnabled     = false;
        aBandDynParams[iBand].eMode        = EDynMode::Compress;
        aBandDynParams[iBand].fThresholdDb = -20.0f;
        aBandDynParams[iBand].fRatio       = 4.0f;
        aBandDynParams[iBand].fAttackMs    = 5.0f;
        aBandDynParams[iBand].fReleaseMs   = 80.0f;

        afDetEnvelope[iBand]         = 0.0f;
        afBandGainReductionDb[iBand] = 0.0f;
        abBandMute[iBand]            = false;
    }

    ClearFilterState();
}

void CAudioEqualizer::Process ( CVector<int16_t>& vecsStereoInOut, const int iStereoBlockSizeSam )
{
    // --- Step 1: Compute per-band dynamic gain reduction/boost from previous block's detector envelopes ---
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        if ( aBandDynParams[iBand].bEnabled && afDetEnvelope[iBand] > 0.0f )
        {
            // Convert envelope to dB (envelope is in sample-value units, normalise to 0 dBFS = 32768)
            const float fInputDb = 20.0f * std::log10 ( std::max ( afDetEnvelope[iBand] / 32768.0f, 1e-8f ) );

            if ( fInputDb > aBandDynParams[iBand].fThresholdDb )
            {
                const float fOverDb      = fInputDb - aBandDynParams[iBand].fThresholdDb;
                const float fDynAmountDb = fOverDb * ( 1.0f - 1.0f / aBandDynParams[iBand].fRatio );

                if ( aBandDynParams[iBand].eMode == EDynMode::Compress )
                {
                    afBandGainReductionDb[iBand] = fDynAmountDb; // Dynamic attenuation
                }
                else
                {
                    afBandGainReductionDb[iBand] = -fDynAmountDb; // Dynamic boost
                }
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

    // --- Step 2: Smooth static gain and compute effective SVF gain ---
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

        // Update SVF coefficients only if effective gain actually changed
        if ( std::fabs ( fEffective - afBandEffectiveGainDb[iBand] ) > 0.001f )
        {
            afBandEffectiveGainDb[iBand] = fEffective;
            UpdateBandCoeff ( iBand, fEffective, afBandQ[iBand] );
        }
    }

    // --- Step 3: Pre-compute per-band attack/release coefficients (frequency-scaled) ---
    float afAttackCoeff[NUM_BANDS];
    float afReleaseCoeff[NUM_BANDS];

    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        if ( aBandDynParams[iBand].bEnabled )
        {
            // Adapt detector time constants relative to center frequency:
            // Prevents low-frequency harmonic ripple distortion in bass while maintaining fast high response.
            const float fMinPeriodMs = ( afBandFrequencies[iBand] > 1.0f ) ? ( ( 1000.0f / afBandFrequencies[iBand] ) * 2.0f ) : 1.0f;
            const float fAttackMs    = std::max ( aBandDynParams[iBand].fAttackMs, fMinPeriodMs * 0.5f );
            const float fReleaseMs   = std::max ( aBandDynParams[iBand].fReleaseMs, fMinPeriodMs * 2.0f );

            afAttackCoeff[iBand]  = std::exp ( -1.0f / ( 0.001f * fAttackMs * iSampleRateHz ) );
            afReleaseCoeff[iBand] = std::exp ( -1.0f / ( 0.001f * fReleaseMs * iSampleRateHz ) );
        }
    }

    // --- Step 4: Process audio samples with Cytomic SVF filters ---
    const int   iFrameCount = iStereoBlockSizeSam / 2;
    const float fWetStep    = ( iFrameCount > 0 ) ? ( fWetMixTarget - fWetMixCurrent ) / iFrameCount : 0.0f;

    for ( int iSample = 0; iSample < iStereoBlockSizeSam; iSample += 2 )
    {
        // Temporary per-frame peak from detector output for envelope update
        float afDetPeak[NUM_BANDS] = {};

        for ( int iChannel = 0; iChannel < 2; ++iChannel )
        {
            const float fDry        = vecsStereoInOut[iSample + iChannel];
            float       fSample     = fDry;
            float       fSoloSample = fDry;

            for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
            {
                // --- Detector path: bandpass filter on dry (pre-EQ) input for dynamics / solo ---
                if ( aBandDynParams[iBand].bEnabled || iSoloBand == iBand )
                {
                    const SSvfCoeff& d  = aDetCoeff[iBand];
                    SSvfState&       ds = aDetState[iBand];

                    const float v1     = d.a1 * ds.ic1eq[iChannel] + d.a2 * ( fDry - ds.ic2eq[iChannel] );
                    const float v2     = ds.ic2eq[iChannel] + d.g * v1;
                    ds.ic1eq[iChannel] = 2.0f * v1 - ds.ic1eq[iChannel];
                    ds.ic2eq[iChannel] = 2.0f * v2 - ds.ic2eq[iChannel];

                    const float fDetOut = d.m0 * fDry + d.m1 * v1 + d.m2 * v2;

                    if ( iSoloBand == iBand )
                    {
                        fSoloSample = fDetOut;
                    }

                    if ( aBandDynParams[iBand].bEnabled )
                    {
                        const float fAbs = std::fabs ( fDetOut );
                        if ( fAbs > afDetPeak[iBand] )
                        {
                            afDetPeak[iBand] = fAbs;
                        }
                    }
                }

                // --- EQ path: cascaded Cytomic SVF section ---
                if ( !abBandMute[iBand] )
                {
                    const SSvfCoeff& c = aBandCoeff[iBand];
                    SSvfState&       s = aBandState[iBand];

                    const float v1    = c.a1 * s.ic1eq[iChannel] + c.a2 * ( fSample - s.ic2eq[iChannel] );
                    const float v2    = s.ic2eq[iChannel] + c.g * v1;
                    s.ic1eq[iChannel] = 2.0f * v1 - s.ic1eq[iChannel];
                    s.ic2eq[iChannel] = 2.0f * v2 - s.ic2eq[iChannel];

                    fSample = c.m0 * fSample + c.m1 * v1 + c.m2 * v2;
                }
            }

            if ( iSoloBand >= 0 )
            {
                // When a band is soloed, audition the isolated bandpass signal
                fSample = fSoloSample;
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
    // Andrew Simper / Cytomic State Variable Filter (SVF) - linear trapezoidal solver
    constexpr float fPi       = 3.14159265358979323846f;
    const float     fFreq     = std::max ( 10.0f, std::min ( afBandFrequencies[iBandIndex], ( iSampleRateHz * 0.49f ) ) );
    const float     fClampedQ = std::max ( 0.1f, std::min ( 20.0f, fQ ) );
    const double    A         = std::pow ( 10.0, static_cast<double> ( fGainDb ) / 40.0 );

    double g  = std::tan ( static_cast<double> ( fPi * fFreq / iSampleRateHz ) );
    double k  = 1.0 / static_cast<double> ( fClampedQ );
    double m0 = 1.0;
    double m1 = 0.0;
    double m2 = 0.0;

    switch ( aeBandFilterType[iBandIndex] )
    {
    case EFilterType::Peak:
    {
        k  = 1.0 / ( static_cast<double> ( fClampedQ ) * A );
        m0 = 1.0;
        m1 = k * ( A * A - 1.0 );
        m2 = 0.0;
        break;
    }
    case EFilterType::LowShelf:
    {
        g /= std::sqrt ( A );
        k  = 1.0 / static_cast<double> ( fClampedQ );
        m0 = 1.0;
        m1 = k * ( A - 1.0 );
        m2 = A * A - 1.0;
        break;
    }
    case EFilterType::HighShelf:
    {
        g *= std::sqrt ( A );
        k  = 1.0 / static_cast<double> ( fClampedQ );
        m0 = A * A;
        m1 = k * ( 1.0 - A ) * A;
        m2 = 1.0 - A * A;
        break;
    }
    case EFilterType::LowPass:
    {
        k  = 1.0 / static_cast<double> ( fClampedQ );
        m0 = 0.0;
        m1 = 0.0;
        m2 = 1.0;
        break;
    }
    case EFilterType::HighPass:
    {
        k  = 1.0 / static_cast<double> ( fClampedQ );
        m0 = 1.0;
        m1 = -k;
        m2 = -1.0;
        break;
    }
    case EFilterType::Notch:
    {
        k  = 1.0 / static_cast<double> ( fClampedQ );
        m0 = 1.0;
        m1 = -k;
        m2 = 0.0;
        break;
    }
    }

    const double a1 = 1.0 / ( 1.0 + g * ( g + k ) );
    const double a2 = g * a1;

    aBandCoeff[iBandIndex].g  = static_cast<float> ( g );
    aBandCoeff[iBandIndex].a1 = static_cast<float> ( a1 );
    aBandCoeff[iBandIndex].a2 = static_cast<float> ( a2 );
    aBandCoeff[iBandIndex].m0 = static_cast<float> ( m0 );
    aBandCoeff[iBandIndex].m1 = static_cast<float> ( m1 );
    aBandCoeff[iBandIndex].m2 = static_cast<float> ( m2 );
}

void CAudioEqualizer::UpdateDetCoeff ( const int iBandIndex, const float fQ )
{
    // Cytomic SVF bandpass filter (m1 = 1.0, m0 = m2 = 0) for frequency-selective envelope detection
    constexpr float fPi       = 3.14159265358979323846f;
    const float     fFreq     = std::max ( 10.0f, std::min ( afBandFrequencies[iBandIndex], ( iSampleRateHz * 0.49f ) ) );
    const float     fClampedQ = std::max ( 0.1f, std::min ( 20.0f, fQ ) );

    const double g = std::tan ( static_cast<double> ( fPi * fFreq / iSampleRateHz ) );
    const double k = 1.0 / static_cast<double> ( fClampedQ );

    const double a1 = 1.0 / ( 1.0 + g * ( g + k ) );
    const double a2 = g * a1;

    aDetCoeff[iBandIndex].g  = static_cast<float> ( g );
    aDetCoeff[iBandIndex].a1 = static_cast<float> ( a1 );
    aDetCoeff[iBandIndex].a2 = static_cast<float> ( a2 );
    aDetCoeff[iBandIndex].m0 = 0.0f;
    aDetCoeff[iBandIndex].m1 = 1.0f;
    aDetCoeff[iBandIndex].m2 = 0.0f;
}

void CAudioEqualizer::ClearFilterState()
{
    for ( int iBand = 0; iBand < NUM_BANDS; ++iBand )
    {
        for ( int iChannel = 0; iChannel < 2; ++iChannel )
        {
            aBandState[iBand].ic1eq[iChannel] = 0.0f;
            aBandState[iBand].ic2eq[iChannel] = 0.0f;

            aDetState[iBand].ic1eq[iChannel] = 0.0f;
            aDetState[iBand].ic2eq[iChannel] = 0.0f;
        }

        afDetEnvelope[iBand]         = 0.0f;
        afBandGainReductionDb[iBand] = 0.0f;
    }
}
