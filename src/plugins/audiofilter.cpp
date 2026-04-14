/******************************************************************************\
* Audio Filters                                                               *
\******************************************************************************/

#include "audiofilter.h"
#include <cmath>
#include <algorithm>

CAudioFilter::CAudioFilter() :
    bBypass ( true ),
    bHighPassEnabled ( false ),
    bLowPassEnabled ( false ),
    iSampleRateHz ( SYSTEM_SAMPLE_RATE_HZ ),
    iHighPassCutoffHz ( 80 ),
    iLowPassCutoffHz ( 12000 ),
    fQ ( 0.7071f )
{
    ClearState ( sHighPassState );
    ClearState ( sLowPassState );
    UpdateHighPassCoeff();
    UpdateLowPassCoeff();
}

void CAudioFilter::Init ( const int iNSampleRateHz )
{
    iSampleRateHz = iNSampleRateHz;
    ClearState ( sHighPassState );
    ClearState ( sLowPassState );
    UpdateHighPassCoeff();
    UpdateLowPassCoeff();
}

void CAudioFilter::SetHighPassEnabled ( const bool bEnabled )
{
    bHighPassEnabled = bEnabled;
    ClearState ( sHighPassState );
}

void CAudioFilter::SetLowPassEnabled ( const bool bEnabled )
{
    bLowPassEnabled = bEnabled;
    ClearState ( sLowPassState );
}

void CAudioFilter::SetHighPassCutoffHz ( const int iHz )
{
    iHighPassCutoffHz = iHz;
    UpdateHighPassCoeff();
}

void CAudioFilter::SetLowPassCutoffHz ( const int iHz )
{
    iLowPassCutoffHz = iHz;
    UpdateLowPassCoeff();
}

void CAudioFilter::UpdateHighPassCoeff()
{
    const float fCutoff = static_cast<float> ( std::max ( 20, iHighPassCutoffHz ) );
    const float fW0 = 2.0f * static_cast<float> ( M_PI ) * fCutoff / iSampleRateHz;
    const float fCosW0 = std::cos ( fW0 );
    const float fSinW0 = std::sin ( fW0 );
    const float fAlpha = fSinW0 / ( 2.0f * fQ );

    const float b0 = ( 1.0f + fCosW0 ) * 0.5f;
    const float b1 = -( 1.0f + fCosW0 );
    const float b2 = ( 1.0f + fCosW0 ) * 0.5f;
    const float a0 = 1.0f + fAlpha;
    const float a1 = -2.0f * fCosW0;
    const float a2 = 1.0f - fAlpha;

    sHighPassCoeff.b0 = b0 / a0;
    sHighPassCoeff.b1 = b1 / a0;
    sHighPassCoeff.b2 = b2 / a0;
    sHighPassCoeff.a1 = a1 / a0;
    sHighPassCoeff.a2 = a2 / a0;
}

void CAudioFilter::UpdateLowPassCoeff()
{
    const float fCutoff = static_cast<float> ( std::max ( 20, iLowPassCutoffHz ) );
    const float fW0 = 2.0f * static_cast<float> ( M_PI ) * fCutoff / iSampleRateHz;
    const float fCosW0 = std::cos ( fW0 );
    const float fSinW0 = std::sin ( fW0 );
    const float fAlpha = fSinW0 / ( 2.0f * fQ );

    const float b0 = ( 1.0f - fCosW0 ) * 0.5f;
    const float b1 = 1.0f - fCosW0;
    const float b2 = ( 1.0f - fCosW0 ) * 0.5f;
    const float a0 = 1.0f + fAlpha;
    const float a1 = -2.0f * fCosW0;
    const float a2 = 1.0f - fAlpha;

    sLowPassCoeff.b0 = b0 / a0;
    sLowPassCoeff.b1 = b1 / a0;
    sLowPassCoeff.b2 = b2 / a0;
    sLowPassCoeff.a1 = a1 / a0;
    sLowPassCoeff.a2 = a2 / a0;
}

void CAudioFilter::ClearState ( SState& state )
{
    for ( int iChannel = 0; iChannel < 2; ++iChannel )
    {
        state.x1[iChannel] = 0.0f;
        state.x2[iChannel] = 0.0f;
        state.y1[iChannel] = 0.0f;
        state.y2[iChannel] = 0.0f;
    }
}

void CAudioFilter::Process ( CVector<int16_t>& vecsStereoInOut, const int iStereoBlockSizeSam )
{
    if ( bBypass || ( !bHighPassEnabled && !bLowPassEnabled ) )
    {
        return;
    }

    for ( int iSample = 0; iSample < iStereoBlockSizeSam; iSample += 2 )
    {
        for ( int iChannel = 0; iChannel < 2; ++iChannel )
        {
            float fSample = vecsStereoInOut[iSample + iChannel];

            if ( bHighPassEnabled )
            {
                const SCoeff& c = sHighPassCoeff;
                SState& s = sHighPassState;
                const float fOut = c.b0 * fSample + c.b1 * s.x1[iChannel] + c.b2 * s.x2[iChannel] - c.a1 * s.y1[iChannel] - c.a2 * s.y2[iChannel];

                s.x2[iChannel] = s.x1[iChannel];
                s.x1[iChannel] = fSample;
                s.y2[iChannel] = s.y1[iChannel];
                s.y1[iChannel] = fOut;

                fSample = fOut;
            }

            if ( bLowPassEnabled )
            {
                const SCoeff& c = sLowPassCoeff;
                SState& s = sLowPassState;
                const float fOut = c.b0 * fSample + c.b1 * s.x1[iChannel] + c.b2 * s.x2[iChannel] - c.a1 * s.y1[iChannel] - c.a2 * s.y2[iChannel];

                s.x2[iChannel] = s.x1[iChannel];
                s.x1[iChannel] = fSample;
                s.y2[iChannel] = s.y1[iChannel];
                s.y1[iChannel] = fOut;

                fSample = fOut;
            }

            vecsStereoInOut[iSample + iChannel] = Float2Short ( fSample );
        }
    }
}
