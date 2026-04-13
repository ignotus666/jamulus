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

#include "audioreverb.h"
#include <algorithm>

void CAudioReverb::Init ( const EAudChanConf eNAudioChannelConf, const int iNStereoBlockSizeSam, const int iNSampleRate, const float fT60 )
{
    // store parameters
    eAudioChannelConf   = eNAudioChannelConf;
    iStereoBlockSizeSam = iNStereoBlockSizeSam;
    iSampleRate         = iNSampleRate;
    iPreDelaySamples    = 0;
    iPreDelayWriteIndex = 0;
    fLastT60            = -1.0f;
    fLastDamping        = -1.0f;
    fWet                = 0.0f;
    fDry                = 1.0f;
    fEarlyLevel         = 0.0f;
    fWidth              = 1.0f;
    bEarlyEnabled       = false;
    bFreeze             = false;
    iEarlyTapCount      = 4;
    afEarlyTapGains[0]  = 0.6f;
    afEarlyTapGains[1]  = 0.5f;
    afEarlyTapGains[2]  = 0.4f;
    afEarlyTapGains[3]  = 0.3f;
    UpdateEarlyTapSamples();
    UpdatePreDelayBuffer ( 0 );

    // delay lengths for 44100 Hz sample rate
    int         lengths[9] = { 1116, 1356, 1422, 1617, 225, 341, 441, 211, 179 };
    const float scaler     = static_cast<float> ( iNSampleRate ) / 44100.0f;

    if ( scaler != 1.0f )
    {
        for ( int i = 0; i < 9; i++ )
        {
            int delay = static_cast<int> ( floorf ( scaler * lengths[i] ) );

            if ( ( delay & 1 ) == 0 )
            {
                delay++;
            }

            while ( !isPrime ( delay ) )
            {
                delay += 2;
            }

            lengths[i] = delay;
        }
    }

    for ( int i = 0; i < 3; i++ )
    {
        allpassDelays[i].Init ( lengths[i + 4] );
    }

    for ( int i = 0; i < 4; i++ )
    {
        combDelays[i].Init ( lengths[i] );
        combFilters[i].setPole ( 0.2f );
    }

    setT60 ( fT60, iNSampleRate );
    fLastT60 = fT60;
    outLeftDelay.Init ( lengths[7] );
    outRightDelay.Init ( lengths[8] );
    allpassCoefficient = 0.7f;
    Clear();
}

bool CAudioReverb::isPrime ( const int number )
{
    /*
        Returns true if argument value is prime. Taken from "class Effect" in
        "STK abstract effects parent class".
    */
    if ( number == 2 )
    {
        return true;
    }

    if ( number & 1 )
    {
        for ( int i = 3; i < static_cast<int> ( sqrtf ( static_cast<float> ( number ) ) ) + 1; i += 2 )
        {
            if ( ( number % i ) == 0 )
            {
                return false;
            }
        }

        return true; // prime
    }
    else
    {
        return false; // even
    }
}

void CAudioReverb::Clear()
{
    // reset and clear all internal state
    allpassDelays[0].Reset ( 0 );
    allpassDelays[1].Reset ( 0 );
    allpassDelays[2].Reset ( 0 );
    combDelays[0].Reset ( 0 );
    combDelays[1].Reset ( 0 );
    combDelays[2].Reset ( 0 );
    combDelays[3].Reset ( 0 );
    combFilters[0].Reset();
    combFilters[1].Reset();
    combFilters[2].Reset();
    combFilters[3].Reset();
    outRightDelay.Reset ( 0 );
    outLeftDelay.Reset ( 0 );
    if ( vecPreDelayBuffer.Size() > 0 )
    {
        for ( int i = 0; i < vecPreDelayBuffer.Size(); ++i )
        {
            vecPreDelayBuffer[i] = 0.0f;
        }
    }
    iPreDelayWriteIndex = 0;
}

void CAudioReverb::setT60 ( const float fT60, const int iSampleRate )
{
    // set the reverberation T60 decay time
    for ( int i = 0; i < 4; i++ )
    {
        combCoefficient[i] = powf ( 10.0f, static_cast<float> ( -3.0f * combDelays[i].Size() / ( fT60 * iSampleRate ) ) );
    }
}

void CAudioReverb::UpdatePreDelayBuffer ( const int iPreDelayMs )
{
    const int iNewPreDelaySamples = std::max ( 0, static_cast<int> ( ( static_cast<float> ( iPreDelayMs ) * iSampleRate ) / 1000.0f ) );
    int       iMaxTapSamples      = 0;

    for ( int iTap = 0; iTap < iEarlyTapCount; ++iTap )
    {
        iMaxTapSamples = std::max ( iMaxTapSamples, aiEarlyTapSamples[iTap] );
    }

    const int iNewBufferSize = std::max ( iNewPreDelaySamples, iMaxTapSamples ) + 1;

    if ( iNewBufferSize != vecPreDelayBuffer.Size() )
    {
        vecPreDelayBuffer.Init ( iNewBufferSize );
        for ( int i = 0; i < vecPreDelayBuffer.Size(); ++i )
        {
            vecPreDelayBuffer[i] = 0.0f;
        }
        iPreDelayWriteIndex = 0;
    }

    iPreDelaySamples = iNewPreDelaySamples;
}

void CAudioReverb::UpdateEarlyTapSamples()
{
    const int aiTapMs[4] = { 7, 11, 17, 29 };

    for ( int iTap = 0; iTap < iEarlyTapCount; ++iTap )
    {
        aiEarlyTapSamples[iTap] = std::max ( 1, static_cast<int> ( ( static_cast<float> ( aiTapMs[iTap] ) * iSampleRate ) / 1000.0f ) );
    }
}

int CAudioReverb::WrapIndex ( const int iIndex, const int iSize ) const
{
    if ( iSize <= 0 )
    {
        return 0;
    }

    int iWrapped = iIndex % iSize;
    if ( iWrapped < 0 )
    {
        iWrapped += iSize;
    }

    return iWrapped;
}

void CAudioReverb::COnePole::setPole ( const float fPole )
{
    // calculate IIR filter coefficients based on the pole value
    fA = -fPole;
    fB = 1.0f - fPole;
}

float CAudioReverb::COnePole::Calc ( const float fIn )
{
    // calculate IIR filter
    fLastSample = fB * fIn - fA * fLastSample;

    return fLastSample;
}

void CAudioReverb::Process ( CVector<int16_t>& vecsStereoInOut, const bool bReverbOnLeftChan, const SReverbParams& sParams )
{
    float fMixedInput, temp, temp0, temp1, temp2;

    const float fWetLocal   = std::max ( 0.0f, std::min ( 1.0f, sParams.fWet ) );
    const float fDryLocal   = std::max ( 0.0f, std::min ( 1.0f, sParams.fDry ) );
    const float fEarlyLocal = std::max ( 0.0f, std::min ( 1.0f, sParams.fEarlyLevel ) );
    const float fWidthLocal = std::max ( 0.0f, std::min ( 1.0f, sParams.fWidth ) );
    const float fDamping    = std::max ( 0.0f, std::min ( 1.0f, sParams.fDamping ) );

    if ( sParams.bFreeze != bFreeze )
    {
        bFreeze = sParams.bFreeze;
        if ( bFreeze )
        {
            for ( int i = 0; i < 4; ++i )
            {
                combCoefficient[i] = 0.9995f;
            }
        }
        else
        {
            setT60 ( sParams.fT60, iSampleRate );
            fLastT60 = sParams.fT60;
        }
    }

    if ( !bFreeze && sParams.fT60 != fLastT60 )
    {
        setT60 ( sParams.fT60, iSampleRate );
        fLastT60 = sParams.fT60;
    }

    if ( fDamping != fLastDamping )
    {
        const float fPole = 0.05f + 0.8f * fDamping;
        for ( int i = 0; i < 4; ++i )
        {
            combFilters[i].setPole ( fPole );
        }
        fLastDamping = fDamping;
    }

    if ( sParams.iPreDelayMs >= 0 )
    {
        UpdatePreDelayBuffer ( sParams.iPreDelayMs );
    }

    fWet          = fWetLocal;
    fDry          = fDryLocal;
    fEarlyLevel   = fEarlyLocal;
    fWidth        = fWidthLocal;
    bEarlyEnabled = sParams.bEarlyEnabled;

    for ( int i = 0; i < iStereoBlockSizeSam; i += 2 )
    {
        // we sum up the stereo input channels (in case mono input is used, a zero
        // shall be input for the right channel)
        const float fInL = static_cast<float> ( vecsStereoInOut[i] );
        const float fInR = static_cast<float> ( vecsStereoInOut[i + 1] );

        if ( eAudioChannelConf == CC_STEREO )
        {
            fMixedInput = 0.5f * ( fInL + fInR );
        }
        else
        {
            if ( bReverbOnLeftChan )
            {
                fMixedInput = fInL;
            }
            else
            {
                fMixedInput = fInR;
            }
        }

        float fPreDelayed = fMixedInput;
        float fEarly      = 0.0f;
        if ( vecPreDelayBuffer.Size() > 0 )
        {
            vecPreDelayBuffer[iPreDelayWriteIndex] = fMixedInput;
            const int iReadIndex = WrapIndex ( iPreDelayWriteIndex - iPreDelaySamples, vecPreDelayBuffer.Size() );
            fPreDelayed          = vecPreDelayBuffer[iReadIndex];

            if ( bEarlyEnabled && ( fEarlyLevel > 0.0f ) )
            {
                for ( int iTap = 0; iTap < iEarlyTapCount; ++iTap )
                {
                    const int iTapIndex = WrapIndex ( iPreDelayWriteIndex - aiEarlyTapSamples[iTap], vecPreDelayBuffer.Size() );
                    fEarly += vecPreDelayBuffer[iTapIndex] * afEarlyTapGains[iTap];
                }
            }

            iPreDelayWriteIndex = WrapIndex ( iPreDelayWriteIndex + 1, vecPreDelayBuffer.Size() );
        }

        temp  = allpassDelays[0].Get();
        temp0 = allpassCoefficient * temp;
        temp0 += fPreDelayed;
        allpassDelays[0].Add ( temp0 );
        temp0 = -( allpassCoefficient * temp0 ) + temp;

        temp  = allpassDelays[1].Get();
        temp1 = allpassCoefficient * temp;
        temp1 += temp0;
        allpassDelays[1].Add ( temp1 );
        temp1 = -( allpassCoefficient * temp1 ) + temp;

        temp  = allpassDelays[2].Get();
        temp2 = allpassCoefficient * temp;
        temp2 += temp1;
        allpassDelays[2].Add ( temp2 );
        temp2 = -( allpassCoefficient * temp2 ) + temp;

        const float temp3 = temp2 + combFilters[0].Calc ( combCoefficient[0] * combDelays[0].Get() );
        const float temp4 = temp2 + combFilters[1].Calc ( combCoefficient[1] * combDelays[1].Get() );
        const float temp5 = temp2 + combFilters[2].Calc ( combCoefficient[2] * combDelays[2].Get() );
        const float temp6 = temp2 + combFilters[3].Calc ( combCoefficient[3] * combDelays[3].Get() );

        combDelays[0].Add ( temp3 );
        combDelays[1].Add ( temp4 );
        combDelays[2].Add ( temp5 );
        combDelays[3].Add ( temp6 );

        const float filtout = temp3 + temp4 + temp5 + temp6;

        outLeftDelay.Add ( filtout );
        outRightDelay.Add ( filtout );

        float fWetL = outLeftDelay.Get();
        float fWetR = outRightDelay.Get();
        const float fWetMono = 0.5f * ( fWetL + fWetR );
        fWetL                = fWetMono + ( fWetL - fWetMono ) * fWidth;
        fWetR                = fWetMono + ( fWetR - fWetMono ) * fWidth;

        // inplace apply the attenuated reverb signal (for stereo always apply
        // reverberation effect on both channels)
        if ( ( eAudioChannelConf == CC_STEREO ) || bReverbOnLeftChan )
        {
            const float fOutL = fDry * fInL + fWet * 0.5f * fWetL + fEarlyLevel * fEarly;
            vecsStereoInOut[i] = Float2Short ( fOutL );
        }

        if ( ( eAudioChannelConf == CC_STEREO ) || !bReverbOnLeftChan )
        {
            const float fOutR = fDry * fInR + fWet * 0.5f * fWetR + fEarlyLevel * fEarly;
            vecsStereoInOut[i + 1] = Float2Short ( fOutR );
        }
    }
}