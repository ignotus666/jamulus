/******************************************************************************\
* Audio Compressor                                                            *
\******************************************************************************/

#include "audiocompressor.h"
#include <cmath>

CAudioCompressor::CAudioCompressor() :
    bBypass ( true ),
    bLimiterEnabled ( true ),
    iSampleRateHz ( SYSTEM_SAMPLE_RATE_HZ ),
    fThresholdDb ( -12.0f ),
    fRatio ( 3.0f ),
    fAttackMs ( 5.0f ),
    fReleaseMs ( 120.0f ),
    fMakeupDb ( 3.0f ),
    fEnvelope ( 0.0f ),
    fKneeDb ( 6.0f ),
    fLimiterCeilDb ( -1.0f )
{}

void CAudioCompressor::Init ( const int iNSampleRateHz )
{
    iSampleRateHz = iNSampleRateHz;
    fEnvelope     = 0.0f;
}

float CAudioCompressor::DbToLinear ( const float fDb ) const { return std::pow ( 10.0f, fDb / 20.0f ); }

float CAudioCompressor::LinearToDb ( const float fValue ) const
{
    const float fSafe = std::max ( fValue, 1.0e-8f );
    return 20.0f * std::log10 ( fSafe );
}

float CAudioCompressor::ComputeGainDb ( const float fInputDb ) const
{
    const float fKneeHalf = fKneeDb * 0.5f;

    if ( fInputDb <= ( fThresholdDb - fKneeHalf ) )
    {
        return 0.0f;
    }

    if ( fInputDb >= ( fThresholdDb + fKneeHalf ) )
    {
        const float fOverDb       = fInputDb - fThresholdDb;
        const float fCompressedDb = fOverDb / fRatio;
        return fCompressedDb - fOverDb;
    }

    // Soft-knee interpolation within the knee region.
    const float fKneeInput    = fInputDb - ( fThresholdDb - fKneeHalf );
    const float fKneeRatio    = fKneeInput / fKneeDb;
    const float fOverDb       = fInputDb - fThresholdDb;
    const float fCompressedDb = fOverDb / fRatio;
    const float fGainFull     = fCompressedDb - fOverDb;

    return fGainFull * ( fKneeRatio * fKneeRatio );
}

void CAudioCompressor::Process ( CVector<int16_t>& vecsStereoInOut, const int iStereoBlockSizeSam )
{
    if ( bBypass )
    {
        return;
    }

    const float fAttackCoeff    = std::exp ( -1.0f / ( 0.001f * fAttackMs * iSampleRateHz ) );
    const float fReleaseCoeff   = std::exp ( -1.0f / ( 0.001f * fReleaseMs * iSampleRateHz ) );
    const float fMakeupLin      = DbToLinear ( fMakeupDb );
    const float fLimiterCeilLin = DbToLinear ( fLimiterCeilDb );

    for ( int iSample = 0; iSample < iStereoBlockSizeSam; iSample += 2 )
    {
        const float fL   = vecsStereoInOut[iSample];
        const float fR   = vecsStereoInOut[iSample + 1];
        const float fAbs = std::max ( std::fabs ( fL ), std::fabs ( fR ) );

        if ( fAbs > fEnvelope )
        {
            fEnvelope = fAttackCoeff * fEnvelope + ( 1.0f - fAttackCoeff ) * fAbs;
        }
        else
        {
            fEnvelope = fReleaseCoeff * fEnvelope + ( 1.0f - fReleaseCoeff ) * fAbs;
        }

        const float fInputDb = LinearToDb ( fEnvelope / 32768.0f );
        const float fGainDb  = ComputeGainDb ( fInputDb ) + fMakeupDb;
        float       fGainLin = DbToLinear ( fGainDb );

        float fOutL = fL * fGainLin;
        float fOutR = fR * fGainLin;

        if ( bLimiterEnabled )
        {
            fOutL = std::max ( -fLimiterCeilLin * 32768.0f, std::min ( fLimiterCeilLin * 32768.0f, fOutL ) );
            fOutR = std::max ( -fLimiterCeilLin * 32768.0f, std::min ( fLimiterCeilLin * 32768.0f, fOutR ) );
        }

        vecsStereoInOut[iSample]     = Float2Short ( fOutL );
        vecsStereoInOut[iSample + 1] = Float2Short ( fOutR );
    }
}
