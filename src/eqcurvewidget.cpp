/******************************************************************************\
 * Copyright (c) 2026
 *
 * Author(s):
 *  Daryl Hanlon
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "eqcurvewidget.h"
#include "uicolors.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <cmath>

namespace
{
QString FilterTypeToString ( const CAudioEqualizer::EFilterType eType )
{
    switch ( eType )
    {
    case CAudioEqualizer::EFilterType::Peak:
        return QObject::tr ( "Peak" );
    case CAudioEqualizer::EFilterType::LowShelf:
        return QObject::tr ( "Low Shelf" );
    case CAudioEqualizer::EFilterType::HighShelf:
        return QObject::tr ( "High Shelf" );
    case CAudioEqualizer::EFilterType::LowPass:
        return QObject::tr ( "Low Pass" );
    case CAudioEqualizer::EFilterType::HighPass:
        return QObject::tr ( "High Pass" );
    case CAudioEqualizer::EFilterType::Notch:
        return QObject::tr ( "Notch" );
    }
    return QObject::tr ( "Peak" );
}

QString MakeBandTooltipText ( const int                          iBand,
                              const float                        fFreqHz,
                              const float                        fGainDb,
                              const float                        fQ,
                              const CAudioEqualizer::EFilterType eType,
                              const bool                         bSolo )
{
    const QString strType = FilterTypeToString ( eType );
    const QString strFreq = ( fFreqHz >= 1000.0f ) ? QString::number ( fFreqHz / 1000.0f, 'f', 2 ) + QObject::tr ( " kHz" )
                                                   : QString::number ( fFreqHz, 'f', 1 ) + QObject::tr ( " Hz" );
    const QString strGain = ( fGainDb >= 0.0f ) ? QString ( "+%1 dB" ).arg ( QString::number ( fGainDb, 'f', 1 ) )
                                                : QString ( "%1 dB" ).arg ( QString::number ( fGainDb, 'f', 1 ) );
    QString       strRes  = QObject::tr ( "Band %1 (%2) | %3 | %4 | Q: %5" )
                         .arg ( iBand + 1 )
                         .arg ( strType )
                         .arg ( strFreq )
                         .arg ( strGain )
                         .arg ( QString::number ( fQ, 'f', 1 ) );
    if ( bSolo )
    {
        strRes += QObject::tr ( " [SOLO]" );
    }
    return strRes;
}

} // namespace

// ---------------------------------------------------------------------------
// CFft Implementation (Cooley-Tukey Radix-2 Real FFT)
// ---------------------------------------------------------------------------
CFft::CFft()
{
    constexpr float fPi = 3.14159265358979323846f;

    // Precompute Hann window
    for ( int i = 0; i < N; ++i )
    {
        afWindow[i] = 0.5f * ( 1.0f - std::cos ( 2.0f * fPi * i / ( N - 1 ) ) );
    }

    // Precompute bit reversal table
    for ( int i = 0; i < N; ++i )
    {
        int rev  = 0;
        int temp = i;
        for ( int j = 0; j < 11; ++j ) // 2^11 = 2048
        {
            rev = ( rev << 1 ) | ( temp & 1 );
            temp >>= 1;
        }
        aiBitRev[i] = rev;
    }

    // Precompute twiddle factor tables
    for ( int i = 0; i < N / 2; ++i )
    {
        const float fAngle = -2.0f * fPi * i / N;
        afCosTable[i]      = std::cos ( fAngle );
        afSinTable[i]      = std::sin ( fAngle );
    }
}

void CFft::Compute ( const float* pInput, float* pMagnitudesOut )
{
    float real[N];
    float imag[N];

    for ( int i = 0; i < N; ++i )
    {
        const int iRev = aiBitRev[i];
        real[iRev]     = pInput[i] * afWindow[i];
        imag[iRev]     = 0.0f;
    }

    for ( int len = 2; len <= N; len <<= 1 )
    {
        const int halfLen = len >> 1;
        const int step    = N / len;

        for ( int i = 0; i < N; i += len )
        {
            for ( int j = 0; j < halfLen; ++j )
            {
                const int   idx = j * step;
                const float uR  = real[i + j];
                const float uI  = imag[i + j];
                const float vR  = real[i + j + halfLen] * afCosTable[idx] - imag[i + j + halfLen] * afSinTable[idx];
                const float vI  = real[i + j + halfLen] * afSinTable[idx] + imag[i + j + halfLen] * afCosTable[idx];

                real[i + j]           = uR + vR;
                imag[i + j]           = uI + vI;
                real[i + j + halfLen] = uR - vR;
                imag[i + j + halfLen] = uI - vI;
            }
        }
    }

    const float norm = 2.0f / N;
    for ( int i = 0; i < N / 2; ++i )
    {
        pMagnitudesOut[i] = std::sqrt ( real[i] * real[i] + imag[i] * imag[i] ) * norm;
    }
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
CEQCurveWidget::CEQCurveWidget ( QWidget* parent ) :
    QWidget ( parent ),
    iSampleRateHz ( 48000 ),
    iSelectedBand ( 0 ),
    iSoloBand ( -1 ),
    bDragging ( false ),
    bDrawingCurve ( false ),
    bDarkTheme ( true ),
    bStaticCurveDirty ( true ),
    bEffectiveCurveDirty ( true ),
    iTooltipBand ( -1 )
{
    for ( int i = 0; i < kNumBands; ++i )
    {
        afBandGainDb[i]          = 0.0f;
        afBandGainReductionDb[i] = 0.0f;
        afBandFrequencies[i]     = CAudioEqualizer::GetDefaultBandFrequency ( i );
        afBandQ[i]               = 1.0f;
        abBandMuted[i]           = false;
        aeBandFilterType[i]      = CAudioEqualizer::EFilterType::Peak;
    }
    for ( int i = 0; i < kNumVisualBands; ++i )
    {
        afSpectrumLevels[i] = 0.0f;
        afSpectrumPeaks[i]  = 0.0f;
    }
    for ( int p = 0; p < kNumVisualCurvePoints; ++p )
    {
        const float fNorm = static_cast<float> ( p ) / ( kNumVisualCurvePoints - 1 );
        const float fHz   = kFreqMin * std::pow ( kFreqMax / kFreqMin, fNorm );
        afVisualFreqs[p]  = fHz;
        // +3.0 dB/octave tilt relative to 1000 Hz for natural musical energy balance
        afVisualTiltDb[p]       = 3.0f * ( std::log ( fHz / 1000.0f ) / 0.693147f );
        afSpectrumPreLevels[p]  = 0.0f;
        afSpectrumPostLevels[p] = 0.0f;
    }

    setMinimumSize ( 300, 120 );
    setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setMouseTracking ( true );
    setFocusPolicy ( Qt::ClickFocus );
}

void CEQCurveWidget::UpdateBandTooltip ( const int iBand, const bool bVisible )
{
    const int iNewBand = ( bVisible && iBand >= 0 && iBand < kNumBands ) ? iBand : -1;
    if ( iTooltipBand != iNewBand )
    {
        iTooltipBand = iNewBand;
        update();
    }
}

// ---------------------------------------------------------------------------
// Public setters
// ---------------------------------------------------------------------------
void CEQCurveWidget::SetBandGain ( const int iBand, const float fGainDb )
{
    if ( iBand >= 0 && iBand < kNumBands )
    {
        if ( std::fabs ( afBandGainDb[iBand] - fGainDb ) > 0.01f )
        {
            afBandGainDb[iBand]  = fGainDb;
            bStaticCurveDirty    = true;
            bEffectiveCurveDirty = true;
            update();
        }
    }
}

void CEQCurveWidget::SetBandFrequency ( const int iBand, const float fFreqHz )
{
    if ( iBand >= 0 && iBand < kNumBands )
    {
        if ( std::fabs ( afBandFrequencies[iBand] - fFreqHz ) > 0.1f )
        {
            afBandFrequencies[iBand] = fFreqHz;
            bStaticCurveDirty        = true;
            bEffectiveCurveDirty     = true;
            update();
        }
    }
}

void CEQCurveWidget::SetBandQ ( const int iBand, const float fQ )
{
    if ( iBand >= 0 && iBand < kNumBands )
    {
        if ( std::fabs ( afBandQ[iBand] - fQ ) > 0.01f )
        {
            afBandQ[iBand]       = fQ;
            bStaticCurveDirty    = true;
            bEffectiveCurveDirty = true;
            update();
        }
    }
}

void CEQCurveWidget::SetBandFilterType ( const int iBand, const CAudioEqualizer::EFilterType eType )
{
    if ( iBand >= 0 && iBand < kNumBands )
    {
        if ( aeBandFilterType[iBand] != eType )
        {
            aeBandFilterType[iBand] = eType;
            bStaticCurveDirty       = true;
            bEffectiveCurveDirty    = true;
            update();
        }
    }
}

CAudioEqualizer::EFilterType CEQCurveWidget::GetBandFilterType ( const int iBand ) const
{
    return ( iBand >= 0 && iBand < kNumBands ) ? aeBandFilterType[iBand] : CAudioEqualizer::EFilterType::Peak;
}

void CEQCurveWidget::SetSoloBand ( const int iBand )
{
    const int iNewSolo = ( iBand >= 0 && iBand < kNumBands ) ? iBand : -1;
    if ( iSoloBand != iNewSolo )
    {
        iSoloBand = iNewSolo;
        update();
    }
}

void CEQCurveWidget::SetBandMuted ( const int iBand, const bool bMuted )
{
    if ( iBand >= 0 && iBand < kNumBands )
    {
        if ( abBandMuted[iBand] != bMuted )
        {
            abBandMuted[iBand]   = bMuted;
            bStaticCurveDirty    = true;
            bEffectiveCurveDirty = true;
            update();
        }
    }
}

bool CEQCurveWidget::GetBandMuted ( const int iBand ) const { return ( iBand >= 0 && iBand < kNumBands ) ? abBandMuted[iBand] : false; }

float CEQCurveWidget::GetBandFrequency ( const int iBand ) const { return ( iBand >= 0 && iBand < kNumBands ) ? afBandFrequencies[iBand] : 0.0f; }

void CEQCurveWidget::SetBandGainReduction ( const int iBand, const float fReductionDb )
{
    if ( iBand >= 0 && iBand < kNumBands )
    {
        if ( std::fabs ( afBandGainReductionDb[iBand] - fReductionDb ) > 0.2f )
        {
            afBandGainReductionDb[iBand] = fReductionDb;
            bEffectiveCurveDirty         = true;
            update();
        }
    }
}

void CEQCurveWidget::SetSpectrumLevels ( const QVector<float>& vecLevels )
{
    const int iInputSize = vecLevels.size();
    bool      bChanged   = false;

    for ( int iBand = 0; iBand < kNumVisualBands; ++iBand )
    {
        float fNewVal = 0.0f;
        if ( iBand < iInputSize )
        {
            fNewVal = vecLevels[iBand];
        }

        fNewVal = std::max ( 0.0f, std::min ( 1.0f, fNewVal ) );

        const float fOldLevel = afSpectrumLevels[iBand];
        float       fTargetLevel;

        if ( fNewVal >= fOldLevel )
        {
            fTargetLevel = fNewVal;
        }
        else
        {
            fTargetLevel = fOldLevel * 0.88f + fNewVal * 0.12f;
        }

        if ( std::fabs ( fOldLevel - fTargetLevel ) > 0.005f )
        {
            afSpectrumLevels[iBand] = fTargetLevel;
            bChanged                = true;
        }

        const float fOldPeak = afSpectrumPeaks[iBand];
        float       fTargetPeak;

        if ( fNewVal >= fOldPeak )
        {
            fTargetPeak = fNewVal;
        }
        else
        {
            fTargetPeak = std::max ( fNewVal, fOldPeak * 0.97f - 0.002f );
        }

        if ( std::fabs ( fOldPeak - fTargetPeak ) > 0.005f )
        {
            afSpectrumPeaks[iBand] = fTargetPeak;
            bChanged               = true;
        }
    }

    if ( bChanged )
    {
        update();
    }
}

void CEQCurveWidget::ResetSpectrum()
{
    for ( int p = 0; p < kNumVisualCurvePoints; ++p )
    {
        afSpectrumPreLevels[p]  = 0.0f;
        afSpectrumPostLevels[p] = 0.0f;
    }
    for ( int i = 0; i < kNumVisualBands; ++i )
    {
        afSpectrumLevels[i] = 0.0f;
        afSpectrumPeaks[i]  = 0.0f;
    }
    update();
}

void CEQCurveWidget::ProcessSpectrum ( const float* pfPreSamples, const float* pfPostSamples, const int iNumSamples )
{
    if ( !pfPreSamples || !pfPostSamples || iNumSamples < CFft::N )
    {
        return;
    }

    float afPreMag[CFft::N / 2];
    float afPostMag[CFft::N / 2];
    FftEngine.Compute ( pfPreSamples, afPreMag );
    FftEngine.Compute ( pfPostSamples, afPostMag );

    const float fBinHz   = ( iSampleRateHz > 0 ? static_cast<float> ( iSampleRateHz ) : 48000.0f ) / static_cast<float> ( CFft::N );
    const float fMinDb   = -68.0f;
    const float fMaxDb   = 6.0f;
    const float fDbRange = fMaxDb - fMinDb;

    bool bChanged = false;

    for ( int p = 0; p < kNumVisualCurvePoints; ++p )
    {
        const float fHz   = afVisualFreqs[p];
        const float fTilt = afVisualTiltDb[p];

        const float fBin  = fHz / fBinHz;
        const int   iBin0 = std::max ( 0, std::min ( CFft::N / 2 - 2, static_cast<int> ( fBin ) ) );
        const float fFrac = fBin - static_cast<float> ( iBin0 );

        const float fBandwidthHz = fHz * ( std::pow ( kFreqMax / kFreqMin, 1.0f / ( kNumVisualCurvePoints - 1 ) ) - 1.0f );
        const int   iSpanBins    = std::max ( 1, static_cast<int> ( std::ceil ( fBandwidthHz / fBinHz ) ) );
        const int   iBinEnd      = std::min ( CFft::N / 2 - 1, iBin0 + iSpanBins );

        float fMagPre  = ( 1.0f - fFrac ) * afPreMag[iBin0] + fFrac * afPreMag[iBin0 + 1];
        float fMagPost = ( 1.0f - fFrac ) * afPostMag[iBin0] + fFrac * afPostMag[iBin0 + 1];

        for ( int b = iBin0 + 1; b <= iBinEnd; ++b )
        {
            fMagPre  = std::max ( fMagPre, afPreMag[b] );
            fMagPost = std::max ( fMagPost, afPostMag[b] );
        }

        const float fDbPre   = ( fMagPre > 1e-6f ) ? ( 20.0f * std::log10 ( fMagPre ) + fTilt ) : -100.0f;
        float       fNormPre = ( fDbPre - fMinDb ) / fDbRange;
        fNormPre             = std::max ( 0.0f, std::min ( 1.0f, fNormPre ) );

        const float fDbPost   = ( fMagPost > 1e-6f ) ? ( 20.0f * std::log10 ( fMagPost ) + fTilt ) : -100.0f;
        float       fNormPost = ( fDbPost - fMinDb ) / fDbRange;
        fNormPost             = std::max ( 0.0f, std::min ( 1.0f, fNormPost ) );

        // Ballistics (fast attack, smooth exponential decay)
        const float fOldPre = afSpectrumPreLevels[p];
        float       fTargetPre;
        if ( fNormPre >= fOldPre )
        {
            fTargetPre = fOldPre * 0.35f + fNormPre * 0.65f;
        }
        else
        {
            fTargetPre = fOldPre * 0.88f + fNormPre * 0.12f;
        }

        if ( std::fabs ( fOldPre - fTargetPre ) > 0.003f )
        {
            afSpectrumPreLevels[p] = fTargetPre;
            bChanged               = true;
        }

        const float fOldPost = afSpectrumPostLevels[p];
        float       fTargetPost;
        if ( fNormPost >= fOldPost )
        {
            fTargetPost = fOldPost * 0.35f + fNormPost * 0.65f;
        }
        else
        {
            fTargetPost = fOldPost * 0.88f + fNormPost * 0.12f;
        }

        if ( std::fabs ( fOldPost - fTargetPost ) > 0.003f )
        {
            afSpectrumPostLevels[p] = fTargetPost;
            bChanged                = true;
        }
    }

    if ( bChanged )
    {
        update();
    }
}

void CEQCurveWidget::SetSampleRate ( const int iRate )
{
    if ( iSampleRateHz != iRate )
    {
        iSampleRateHz        = iRate;
        bStaticCurveDirty    = true;
        bEffectiveCurveDirty = true;
        update();
    }
}

void CEQCurveWidget::SetBypassed ( const bool bBypassed )
{
    if ( bBypassed )
    {
        ResetSpectrum();
        for ( int i = 0; i < kNumBands; ++i )
        {
            afBandGainReductionDb[i] = 0.0f;
        }
        update();
    }
}

void CEQCurveWidget::resizeEvent ( QResizeEvent* pEvent )
{
    QWidget::resizeEvent ( pEvent );
    bStaticCurveDirty    = true;
    bEffectiveCurveDirty = true;
    update();
}

void CEQCurveWidget::leaveEvent ( QEvent* pEvent )
{
    QWidget::leaveEvent ( pEvent );
    UpdateBandTooltip ( -1, false );
}

void CEQCurveWidget::SetDarkTheme ( const bool bEnable )
{
    if ( bDarkTheme != bEnable )
    {
        bDarkTheme           = bEnable;
        bStaticCurveDirty    = true;
        bEffectiveCurveDirty = true;
        update();
    }
}

// ---------------------------------------------------------------------------
// Coordinate mapping
// ---------------------------------------------------------------------------
QRectF CEQCurveWidget::PlotRect() const
{
    return QRectF ( kMarginLeft, kMarginTop, width() - kMarginLeft - kMarginRight, height() - kMarginTop - kMarginBottom );
}

float CEQCurveWidget::FreqToXf ( const float fHz ) const
{
    const QRectF r     = PlotRect();
    const float  fClmp = std::max ( kFreqMin, std::min ( kFreqMax, fHz ) );
    const float  fNorm = std::log ( fClmp / kFreqMin ) / std::log ( kFreqMax / kFreqMin );
    return r.left() + fNorm * r.width();
}

int CEQCurveWidget::FreqToX ( const float fHz ) const { return static_cast<int> ( std::round ( FreqToXf ( fHz ) ) ); }

float CEQCurveWidget::DbToYf ( const float fDb ) const
{
    const QRectF r     = PlotRect();
    const float  fClmp = std::max ( kDisplayMin, std::min ( kDisplayMax, fDb ) );
    const float  fNorm = ( kDisplayMax - fClmp ) / ( kDisplayMax - kDisplayMin );
    return r.top() + fNorm * r.height();
}

float CEQCurveWidget::XToFreq ( const float fX ) const
{
    const QRectF r     = PlotRect();
    const float  fNorm = ( fX - r.left() ) / r.width();
    return kFreqMin * std::pow ( kFreqMax / kFreqMin, fNorm );
}

float CEQCurveWidget::YToDb ( const float fY ) const
{
    const QRectF r     = PlotRect();
    const float  fNorm = ( fY - r.top() ) / r.height();
    return kDisplayMax - fNorm * ( kDisplayMax - kDisplayMin );
}

// ---------------------------------------------------------------------------
// SVF transfer-function evaluation (analytical, no FFT)
// ---------------------------------------------------------------------------
float CEQCurveWidget::EvalBandMagnitudeDb ( const int iBand, const float fGainDb, const float fFreqHz ) const
{
    if ( iBand < 0 || iBand >= kNumBands || iSampleRateHz <= 0 || fFreqHz <= 0.0f || abBandMuted[iBand] )
    {
        return 0.0f;
    }

    const CAudioEqualizer::EFilterType eType = aeBandFilterType[iBand];

    // For Peak or Shelves, if gain is 0 dB, response is flat 0 dB
    if ( ( eType == CAudioEqualizer::EFilterType::Peak || eType == CAudioEqualizer::EFilterType::LowShelf ||
           eType == CAudioEqualizer::EFilterType::HighShelf ) &&
         std::fabs ( fGainDb ) < 0.001f )
    {
        return 0.0f;
    }

    constexpr float fPi       = 3.14159265358979323846f;
    const float     fBandFreq = std::max ( 10.0f, std::min ( afBandFrequencies[iBand], iSampleRateHz * 0.49f ) );
    const float     fQ        = std::max ( 0.1f, std::min ( 20.0f, afBandQ[iBand] ) );
    const double    A         = std::pow ( 10.0, static_cast<double> ( fGainDb ) / 40.0 );

    double g  = std::tan ( static_cast<double> ( fPi * fBandFreq / iSampleRateHz ) );
    double k  = 1.0 / static_cast<double> ( fQ );
    double m0 = 1.0;
    double m1 = 0.0;
    double m2 = 0.0;

    switch ( eType )
    {
    case CAudioEqualizer::EFilterType::Peak:
        k  = 1.0 / ( static_cast<double> ( fQ ) * A );
        m0 = 1.0;
        m1 = k * ( A * A - 1.0 );
        m2 = 0.0;
        break;
    case CAudioEqualizer::EFilterType::LowShelf:
        g /= std::sqrt ( A );
        k  = 1.0 / static_cast<double> ( fQ );
        m0 = 1.0;
        m1 = k * ( A - 1.0 );
        m2 = A * A - 1.0;
        break;
    case CAudioEqualizer::EFilterType::HighShelf:
        g *= std::sqrt ( A );
        k  = 1.0 / static_cast<double> ( fQ );
        m0 = A * A;
        m1 = k * ( 1.0 - A ) * A;
        m2 = 1.0 - A * A;
        break;
    case CAudioEqualizer::EFilterType::LowPass:
        k  = 1.0 / static_cast<double> ( fQ );
        m0 = 0.0;
        m1 = 0.0;
        m2 = 1.0;
        break;
    case CAudioEqualizer::EFilterType::HighPass:
        k  = 1.0 / static_cast<double> ( fQ );
        m0 = 1.0;
        m1 = -k;
        m2 = -1.0;
        break;
    case CAudioEqualizer::EFilterType::Notch:
        k  = 1.0 / static_cast<double> ( fQ );
        m0 = 1.0;
        m1 = -k;
        m2 = 0.0;
        break;
    }

    if ( g < 1e-7 )
    {
        return 0.0f;
    }

    const double wPlot   = std::tan ( static_cast<double> ( fPi * std::min ( fFreqHz, iSampleRateHz * 0.499f ) / iSampleRateHz ) );
    const double omega   = wPlot / g;
    const double omegaSq = omega * omega;

    const double numRe    = m0 * ( 1.0 - omegaSq ) + m2;
    const double numIm    = omega * ( m0 * k + m1 );
    const double numMagSq = numRe * numRe + numIm * numIm;

    const double denRe    = 1.0 - omegaSq;
    const double denIm    = k * omega;
    const double denMagSq = denRe * denRe + denIm * denIm;

    if ( denMagSq < 1e-18 || numMagSq < 1e-18 )
    {
        return ( denMagSq < 1e-18 ) ? 0.0f : -100.0f;
    }

    return static_cast<float> ( 10.0 * std::log10 ( numMagSq / denMagSq ) );
}

void CEQCurveWidget::ComputeResponseCurve ( const float* afGains, QVector<QPointF>& vecPoints ) const
{
    const QRectF  r         = PlotRect();
    const int     iWidth    = std::max ( 1, static_cast<int> ( r.width() ) );
    constexpr int kStepSize = 6;

    vecPoints.clear();
    vecPoints.reserve ( ( iWidth / kStepSize ) + 2 );

    for ( int iX = 0; iX < iWidth; iX += kStepSize )
    {
        const float fX       = r.left() + static_cast<float> ( iX );
        const float fFreqHz  = XToFreq ( fX );
        float       fTotalDb = 0.0f;

        for ( int iBand = 0; iBand < kNumBands; ++iBand )
        {
            fTotalDb += EvalBandMagnitudeDb ( iBand, afGains[iBand], fFreqHz );
        }

        const float fY = DbToYf ( fTotalDb );
        vecPoints.append ( QPointF ( fX, fY ) );
    }

    // Always include the very last point to ensure the curve reaches the right margin
    const float fX       = r.right();
    const float fFreqHz  = XToFreq ( fX );
    float       fTotalDb = 0.0f;
    for ( int iBand = 0; iBand < kNumBands; ++iBand )
    {
        fTotalDb += EvalBandMagnitudeDb ( iBand, afGains[iBand], fFreqHz );
    }
    vecPoints.append ( QPointF ( fX, DbToYf ( fTotalDb ) ) );
}

// ---------------------------------------------------------------------------
// Hit testing
// ---------------------------------------------------------------------------
int CEQCurveWidget::FindNearestBand ( const QPointF& pos, float* pfDistOut ) const
{
    int   iBest  = -1;
    float fBestD = 1e9f;

    for ( int iBand = 0; iBand < kNumBands; ++iBand )
    {
        const float fNx = FreqToXf ( afBandFrequencies[iBand] );
        const float fNy = DbToYf ( afBandGainDb[iBand] );
        const float dx  = pos.x() - fNx;
        const float dy  = pos.y() - fNy;
        const float d   = std::sqrt ( dx * dx + dy * dy );

        if ( d < fBestD )
        {
            fBestD = d;
            iBest  = iBand;
        }
    }

    if ( pfDistOut )
    {
        *pfDistOut = fBestD;
    }

    return iBest;
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------
void CEQCurveWidget::paintEvent ( QPaintEvent* pEvent )
{
    Q_UNUSED ( pEvent )

    QPainter painter ( this );
    painter.setRenderHint ( QPainter::Antialiasing, true );

    const QRectF r = PlotRect();

    // Background
    const QColor colBg   = GetControlPalette ( bDarkTheme ).background;
    const QColor colGrid = QColor ( 48, 52, 58 );
    const QColor colZero = QColor ( 72, 78, 86 );
    const QColor colText = bDarkTheme ? QColor ( 150, 158, 168 ) : QColor ( 100, 110, 120 );

    painter.fillRect ( rect(), colBg );

    // Plot area background
    const QColor colPlotBg = QColor ( 20, 20, 22 );
    painter.fillRect ( r.toRect(), colPlotBg );

    // Grid: horizontal dB lines
    painter.setPen ( QPen ( colGrid, 1 ) );
    const QFont fontSmall = QFont ( "Inter", 8 );
    painter.setFont ( fontSmall );

    for ( int iDb = static_cast<int> ( kDisplayMin ); iDb <= static_cast<int> ( kDisplayMax ); iDb += 3 )
    {
        const float fY = DbToYf ( static_cast<float> ( iDb ) );

        if ( iDb == 0 )
        {
            painter.setPen ( QPen ( colZero, 1.2 ) );
        }
        else
        {
            painter.setPen ( QPen ( colGrid, 1 ) );
        }

        painter.drawLine ( QPointF ( r.left(), fY ), QPointF ( r.right(), fY ) );

        // dB label
        if ( iDb % 6 == 0 )
        {
            painter.setPen ( colText );
            const QString strDb = ( iDb > 0 ) ? QString ( "+%1" ).arg ( iDb ) : QString::number ( iDb );
            painter.drawText ( QRectF ( 0, fY - 8, kMarginLeft - 4, 16 ), Qt::AlignRight | Qt::AlignVCenter, strDb );
        }
    }

    // Grid: vertical frequency lines
    const float afGridFreqs[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    const int   iGridCount    = sizeof ( afGridFreqs ) / sizeof ( afGridFreqs[0] );

    for ( int i = 0; i < iGridCount; ++i )
    {
        const float fX = FreqToXf ( afGridFreqs[i] );
        painter.setPen ( QPen ( colGrid, 1 ) );
        painter.drawLine ( QPointF ( fX, r.top() ), QPointF ( fX, r.bottom() ) );

        // Frequency label
        painter.setPen ( colText );
        QString strLabel;

        if ( afGridFreqs[i] >= 1000.0f )
        {
            strLabel = QString ( "%1k" ).arg ( static_cast<int> ( afGridFreqs[i] / 1000.0f ) );
        }
        else
        {
            strLabel = QString::number ( static_cast<int> ( afGridFreqs[i] ) );
        }

        painter.drawText ( QRectF ( fX - 20, r.bottom() + 2, 40, 20 ), Qt::AlignHCenter | Qt::AlignTop, strLabel );
    }

    // Draw real-time FFT spectrum overlay in the background (Pre-EQ and Post-EQ smooth curves)
    bool bAnyPreSpectrum  = false;
    bool bAnyPostSpectrum = false;
    for ( int p = 0; p < kNumVisualCurvePoints; ++p )
    {
        if ( afSpectrumPreLevels[p] > 0.002f )
        {
            bAnyPreSpectrum = true;
        }
        if ( afSpectrumPostLevels[p] > 0.002f )
        {
            bAnyPostSpectrum = true;
        }
    }

    if ( bAnyPreSpectrum || bAnyPostSpectrum )
    {
        painter.setRenderHint ( QPainter::Antialiasing, true );

        // 1. Draw Pre-EQ curve (subtle, semi-transparent slate-grey)
        if ( bAnyPreSpectrum )
        {
            QPainterPath pathPreArea;
            QPainterPath pathPreLine;

            const float fStartX = FreqToXf ( afVisualFreqs[0] );
            const float fStartY = r.bottom() - afSpectrumPreLevels[0] * r.height();
            pathPreArea.moveTo ( fStartX, r.bottom() );
            pathPreArea.lineTo ( fStartX, fStartY );
            pathPreLine.moveTo ( fStartX, fStartY );

            for ( int p = 1; p < kNumVisualCurvePoints; ++p )
            {
                const float fX = FreqToXf ( afVisualFreqs[p] );
                const float fY = r.bottom() - afSpectrumPreLevels[p] * r.height();
                pathPreArea.lineTo ( fX, fY );
                pathPreLine.lineTo ( fX, fY );
            }

            const float fEndX = FreqToXf ( afVisualFreqs[kNumVisualCurvePoints - 1] );
            pathPreArea.lineTo ( fEndX, r.bottom() );
            pathPreArea.closeSubpath();

            // Fill Pre-EQ area
            const QColor colPreFill = bDarkTheme ? QColor ( 150, 160, 175, 25 ) : QColor ( 120, 130, 145, 20 );
            painter.fillPath ( pathPreArea, colPreFill );

            // Stroke Pre-EQ line
            const QColor colPreLine = bDarkTheme ? QColor ( 175, 185, 200, 160 ) : QColor ( 110, 120, 135, 160 );
            painter.strokePath ( pathPreLine, QPen ( colPreLine, 1.25, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        }

        // 2. Draw Post-EQ curve (vibrant cyan/teal)
        if ( bAnyPostSpectrum )
        {
            QPainterPath pathPostArea;
            QPainterPath pathPostLine;

            const float fStartX = FreqToXf ( afVisualFreqs[0] );
            const float fStartY = r.bottom() - afSpectrumPostLevels[0] * r.height();
            pathPostArea.moveTo ( fStartX, r.bottom() );
            pathPostArea.lineTo ( fStartX, fStartY );
            pathPostLine.moveTo ( fStartX, fStartY );

            for ( int p = 1; p < kNumVisualCurvePoints; ++p )
            {
                const float fX = FreqToXf ( afVisualFreqs[p] );
                const float fY = r.bottom() - afSpectrumPostLevels[p] * r.height();
                pathPostArea.lineTo ( fX, fY );
                pathPostLine.lineTo ( fX, fY );
            }

            const float fEndX = FreqToXf ( afVisualFreqs[kNumVisualCurvePoints - 1] );
            pathPostArea.lineTo ( fEndX, r.bottom() );
            pathPostArea.closeSubpath();

            // Gradient fill for Post-EQ area
            QLinearGradient gradPost ( 0, r.top(), 0, r.bottom() );
            if ( bDarkTheme )
            {
                gradPost.setColorAt ( 0.0, QColor ( 45, 215, 205, 55 ) );
                gradPost.setColorAt ( 0.5, QColor ( 35, 180, 175, 35 ) );
                gradPost.setColorAt ( 1.0, QColor ( 20, 120, 115, 12 ) );
            }
            else
            {
                gradPost.setColorAt ( 0.0, QColor ( 0, 180, 170, 50 ) );
                gradPost.setColorAt ( 0.5, QColor ( 0, 150, 140, 30 ) );
                gradPost.setColorAt ( 1.0, QColor ( 0, 110, 100, 10 ) );
            }
            painter.fillPath ( pathPostArea, gradPost );

            // Stroke Post-EQ line
            const QColor colPostLine = bDarkTheme ? QColor ( 55, 235, 225, 225 ) : QColor ( 0, 165, 155, 225 );
            painter.strokePath ( pathPostLine, QPen ( colPostLine, 1.75, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        }

        // 3. Draw Pre/Post legend badge in top-right corner of the plot
        {
            const int    iBadgeW = 118;
            const int    iBadgeH = 17;
            const QRectF badgeRect ( r.right() - iBadgeW - 6, r.top() + 6, iBadgeW, iBadgeH );

            painter.save();
            painter.setRenderHint ( QPainter::Antialiasing, true );

            // Subtle background pill
            const QColor colBadgeBg     = bDarkTheme ? QColor ( 20, 22, 28, 175 ) : QColor ( 245, 247, 250, 185 );
            const QColor colBadgeBorder = bDarkTheme ? QColor ( 255, 255, 255, 25 ) : QColor ( 0, 0, 0, 25 );
            painter.setPen ( QPen ( colBadgeBorder, 1 ) );
            painter.setBrush ( colBadgeBg );
            painter.drawRoundedRect ( badgeRect, 3.0, 3.0 );

            QFont fontLegend = painter.font();
            fontLegend.setPixelSize ( 9 );
            fontLegend.setBold ( false );
            painter.setFont ( fontLegend );

            const QColor colPreColor   = bDarkTheme ? QColor ( 175, 185, 200 ) : QColor ( 110, 120, 135 );
            const QColor colPostColor  = bDarkTheme ? QColor ( 55, 235, 225 ) : QColor ( 0, 165, 155 );
            const QColor colLegendText = bDarkTheme ? QColor ( 190, 195, 205 ) : QColor ( 60, 65, 75 );

            // Pre label + box
            float       fCurX = badgeRect.left() + 7;
            const float fMidY = badgeRect.center().y();

            painter.setPen ( colLegendText );
            painter.drawText ( QPointF ( fCurX, fMidY + 3.0f ), "Pre:" );
            fCurX += 24;

            painter.setPen ( Qt::NoPen );
            painter.setBrush ( colPreColor );
            painter.drawRoundedRect ( QRectF ( fCurX, fMidY - 3.5f, 7, 7 ), 1.5, 1.5 );
            fCurX += 16;

            // Post label + box
            painter.setPen ( colLegendText );
            painter.drawText ( QPointF ( fCurX, fMidY + 3.0f ), "Post:" );
            fCurX += 28;

            painter.setPen ( Qt::NoPen );
            painter.setBrush ( colPostColor );
            painter.drawRoundedRect ( QRectF ( fCurX, fMidY - 3.5f, 7, 7 ), 1.5, 1.5 );

            painter.restore();
        }
    }

    // Compute and draw response curves
    if ( bStaticCurveDirty )
    {
        ComputeResponseCurve ( afBandGainDb, vecStaticCurveCache );
        bStaticCurveDirty = false;
    }

    bool bAnyReduction = false;
    for ( int iBand = 0; iBand < kNumBands; ++iBand )
    {
        if ( afBandGainReductionDb[iBand] > 0.05f )
        {
            bAnyReduction = true;
            break;
        }
    }

    if ( bAnyReduction )
    {
        if ( bEffectiveCurveDirty )
        {
            float afEffectiveGains[kNumBands];
            for ( int iBand = 0; iBand < kNumBands; ++iBand )
            {
                afEffectiveGains[iBand] = afBandGainDb[iBand] - afBandGainReductionDb[iBand];
            }
            ComputeResponseCurve ( afEffectiveGains, vecEffectiveCurveCache );
            bEffectiveCurveDirty = false;
        }
    }
    else
    {
        vecEffectiveCurveCache = vecStaticCurveCache;
        bEffectiveCurveDirty   = false;
    }

    // Individual band curves
    {
        const int     iWidth    = std::max ( 1, static_cast<int> ( r.width() ) );
        constexpr int kStepSize = 6;
        const float   fZeroY    = DbToYf ( 0.0f );

        for ( int iBand = 0; iBand < kNumBands; ++iBand )
        {
            if ( std::abs ( afBandGainDb[iBand] ) < 0.05f )
            {
                continue;
            }

            const QColor colBand = GetBandColor ( iBand );

            // Build filled path (starts and ends at the 0 dB center line)
            QPainterPath fillPath;
            fillPath.moveTo ( r.left(), fZeroY );

            // Build stroke outline path
            QPainterPath strokePath;
            const float  fStartDb = EvalBandMagnitudeDb ( iBand, afBandGainDb[iBand], XToFreq ( r.left() ) );
            strokePath.moveTo ( r.left(), DbToYf ( fStartDb ) );

            for ( int iX = 0; iX < iWidth; iX += kStepSize )
            {
                const float fX      = r.left() + static_cast<float> ( iX );
                const float fFreqHz = XToFreq ( fX );
                const float fDb     = EvalBandMagnitudeDb ( iBand, afBandGainDb[iBand], fFreqHz );
                const float fY      = DbToYf ( fDb );

                fillPath.lineTo ( fX, fY );
                if ( iX > 0 )
                {
                    strokePath.lineTo ( fX, fY );
                }
            }

            const float fEndDb = EvalBandMagnitudeDb ( iBand, afBandGainDb[iBand], XToFreq ( r.right() ) );
            const float fEndY  = DbToYf ( fEndDb );
            fillPath.lineTo ( r.right(), fEndY );
            fillPath.lineTo ( r.right(), fZeroY );
            fillPath.closeSubpath();

            strokePath.lineTo ( r.right(), fEndY );

            // Render fill with soft opacity (e.g. ~10% for dark/light themes)
            const QColor colFill = QColor ( colBand.red(), colBand.green(), colBand.blue(), 25 );
            painter.fillPath ( fillPath, colFill );

            // Render outline with medium opacity (e.g. ~35%)
            const QColor colOutline = QColor ( colBand.red(), colBand.green(), colBand.blue(), 90 );
            painter.setPen ( QPen ( colOutline, 1.0, Qt::SolidLine ) );
            painter.setBrush ( Qt::NoBrush );
            painter.drawPath ( strokePath );
        }
    }

    const QVector<QPointF>& vecStaticCurve    = vecStaticCurveCache;
    const QVector<QPointF>& vecEffectiveCurve = vecEffectiveCurveCache;

    // Fill between the two curves to show gain reduction
    if ( bAnyReduction && vecStaticCurve.size() == vecEffectiveCurve.size() && vecStaticCurve.size() > 1 )
    {
        QPainterPath fillPath;
        fillPath.moveTo ( vecStaticCurve.first() );

        for ( int i = 1; i < vecStaticCurve.size(); ++i )
        {
            fillPath.lineTo ( vecStaticCurve[i] );
        }

        for ( int i = vecEffectiveCurve.size() - 1; i >= 0; --i )
        {
            fillPath.lineTo ( vecEffectiveCurve[i] );
        }

        fillPath.closeSubpath();

        const QColor colFill = QColor ( 255, 140, 40, 50 );
        painter.fillPath ( fillPath, colFill );
    }

    // Draw static curve
    if ( bAnyReduction && vecStaticCurve.size() > 1 )
    {
        QPainterPath staticPath;
        staticPath.moveTo ( vecStaticCurve.first() );

        for ( int i = 1; i < vecStaticCurve.size(); ++i )
        {
            staticPath.lineTo ( vecStaticCurve[i] );
        }

        const QColor colStatic = QColor ( 54, 207, 255, 70 );
        painter.setPen ( QPen ( colStatic, 1.2 ) );
        painter.setBrush ( Qt::NoBrush );
        painter.drawPath ( staticPath );
    }

    // Draw effective curve (main, bold)
    if ( vecEffectiveCurve.size() > 1 )
    {
        // Gradient fill under the curve
        const float fZeroY = DbToYf ( 0.0f );
        {
            QPainterPath areaPath;
            areaPath.moveTo ( vecEffectiveCurve.first().x(), fZeroY );
            for ( int i = 0; i < vecEffectiveCurve.size(); ++i )
            {
                areaPath.lineTo ( vecEffectiveCurve[i] );
            }
            areaPath.lineTo ( vecEffectiveCurve.last().x(), fZeroY );
            areaPath.closeSubpath();

            const QColor colAreaAbove = QColor ( 54, 207, 255, 22 );
            painter.fillPath ( areaPath, colAreaAbove );
        }

        QPainterPath effectivePath;
        effectivePath.moveTo ( vecEffectiveCurve.first() );

        for ( int i = 1; i < vecEffectiveCurve.size(); ++i )
        {
            effectivePath.lineTo ( vecEffectiveCurve[i] );
        }

        QColor colCurve = QColor ( 54, 207, 255 );
        painter.setPen ( QPen ( colCurve, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
        painter.setBrush ( Qt::NoBrush );
        painter.drawPath ( effectivePath );
    }

    // Band nodes
    for ( int iBand = 0; iBand < kNumBands; ++iBand )
    {
        const float fNx = FreqToXf ( afBandFrequencies[iBand] );
        const float fNy = DbToYf ( afBandGainDb[iBand] );

        const bool bSelected = ( iBand == iSelectedBand );
        const bool bHovered  = ( iBand == iTooltipBand );
        const int  iRad      = ( bSelected || bHovered ) ? kNodeRadius + 2 : kNodeRadius;

        // Per-band color
        const QColor colBand = GetBandColor ( iBand );

        // Glow for selected node
        if ( bSelected )
        {
            const QColor colGlow ( colBand.red(), colBand.green(), colBand.blue(), 80 );
            painter.setPen ( Qt::NoPen );
            painter.setBrush ( colGlow );
            painter.drawEllipse ( QPointF ( fNx, fNy ), iRad + 3, iRad + 3 );

            // Bandwidth (Q) visual capsule
            const float fQ = afBandQ[iBand];
            if ( fQ > 0.0f )
            {
                const float fFreq  = afBandFrequencies[iBand];
                const float fFreqL = std::max ( kFreqMin, fFreq * std::pow ( 2.0f, -0.5f / fQ ) );
                const float fFreqR = std::min ( kFreqMax, fFreq * std::pow ( 2.0f, 0.5f / fQ ) );
                const float fX1    = FreqToXf ( fFreqL );
                const float fX2    = FreqToXf ( fFreqR );

                const QColor colBandwidth ( colBand.red(), colBand.green(), colBand.blue(), 30 );
                painter.setPen ( QPen ( QColor ( colBand.red(), colBand.green(), colBand.blue(), 150 ), 1.0, Qt::DashLine ) );
                painter.setBrush ( colBandwidth );
                painter.drawRoundedRect ( QRectF ( fX1, fNy - 5, fX2 - fX1, 10 ), 5, 5 );
            }
        }
        else if ( bHovered )
        {
            // Subtle glow for hover
            const QColor colGlow ( colBand.red(), colBand.green(), colBand.blue(), 40 );
            painter.setPen ( Qt::NoPen );
            painter.setBrush ( colGlow );
            painter.drawEllipse ( QPointF ( fNx, fNy ), iRad + 2, iRad + 2 );
        }

        // Gain reduction indicator: vertical line from static to effective
        if ( afBandGainReductionDb[iBand] > 0.05f )
        {
            const float  fEy   = DbToYf ( afBandGainDb[iBand] - afBandGainReductionDb[iBand] );
            const QColor colGR = QColor ( 255, 140, 40, 180 );
            painter.setPen ( QPen ( colGR, 2.5, Qt::SolidLine, Qt::RoundCap ) );
            painter.drawLine ( QPointF ( fNx, fNy ), QPointF ( fNx, fEy ) );

            // Small effective-position dot
            painter.setPen ( Qt::NoPen );
            painter.setBrush ( colGR );
            painter.drawEllipse ( QPointF ( fNx, fEy ), 3, 3 );
        }

        // Node circle — per-band color
        QColor colNode = colBand;

        // Solo highlight ring
        if ( iBand == iSoloBand )
        {
            painter.setPen ( QPen ( QColor ( 255, 190, 0, 220 ), 2.0 ) );
            painter.setBrush ( Qt::NoBrush );
            painter.drawEllipse ( QPointF ( fNx, fNy ), iRad + 4, iRad + 4 );
        }

        painter.setPen ( Qt::NoPen );
        painter.setBrush ( colNode );
        painter.drawEllipse ( QPointF ( fNx, fNy ), iRad, iRad );

        // Filter type badge for non-Peak filters
        const CAudioEqualizer::EFilterType eType = aeBandFilterType[iBand];
        if ( eType != CAudioEqualizer::EFilterType::Peak )
        {
            QString strTypeBadge;
            switch ( eType )
            {
            case CAudioEqualizer::EFilterType::LowShelf:
                strTypeBadge = "LS";
                break;
            case CAudioEqualizer::EFilterType::HighShelf:
                strTypeBadge = "HS";
                break;
            case CAudioEqualizer::EFilterType::LowPass:
                strTypeBadge = "LP";
                break;
            case CAudioEqualizer::EFilterType::HighPass:
                strTypeBadge = "HP";
                break;
            case CAudioEqualizer::EFilterType::Notch:
                strTypeBadge = "NT";
                break;
            default:
                break;
            }
            if ( !strTypeBadge.isEmpty() )
            {
                painter.setFont ( QFont ( "Inter", 6, QFont::Bold ) );
                painter.setPen ( colBand );
                painter.drawText ( QRectF ( fNx - 12, fNy + iRad + 1, 24, 10 ), Qt::AlignCenter, strTypeBadge );
            }
        }
    }

    // Solo mode canvas indicator
    if ( iSoloBand >= 0 && iSoloBand < kNumBands )
    {
        const QString strSolo = tr ( "SOLO BAND %1" ).arg ( iSoloBand + 1 );
        painter.setFont ( QFont ( "Inter", 8, QFont::Bold ) );
        const QFontMetrics fm = painter.fontMetrics();
#if QT_VERSION >= QT_VERSION_CHECK( 5, 11, 0 )
        const int iW = fm.horizontalAdvance ( strSolo ) + 12;
#else
        const int iW = fm.width ( strSolo ) + 12;
#endif
        const int   iH = 18;
        const QRect rectSolo ( static_cast<int> ( r.right() - iW - 6 ), static_cast<int> ( r.top() + 6 ), iW, iH );
        painter.setPen ( QPen ( QColor ( 255, 180, 0 ), 1.5 ) );
        painter.setBrush ( QColor ( 40, 30, 10, 220 ) );
        painter.drawRoundedRect ( rectSolo, 3, 3 );
        painter.setPen ( QColor ( 255, 200, 50 ) );
        painter.drawText ( rectSolo, Qt::AlignCenter, strSolo );
    }

    // Plot area border
    painter.setPen ( QPen ( colGrid, 1 ) );
    painter.setBrush ( Qt::NoBrush );
    painter.drawRect ( r );

    // Draw custom canvas tooltip
    if ( iTooltipBand >= 0 && iTooltipBand < kNumBands )
    {
        const float fNx = FreqToXf ( afBandFrequencies[iTooltipBand] );
        const float fNy = DbToYf ( afBandGainDb[iTooltipBand] );

        const QString strText = MakeBandTooltipText ( iTooltipBand,
                                                      afBandFrequencies[iTooltipBand],
                                                      afBandGainDb[iTooltipBand],
                                                      afBandQ[iTooltipBand],
                                                      aeBandFilterType[iTooltipBand],
                                                      iTooltipBand == iSoloBand );

        painter.setFont ( QFont ( "Inter", 8 ) );
        const QFontMetrics fm = painter.fontMetrics();
#if QT_VERSION >= QT_VERSION_CHECK( 5, 11, 0 )
        const int iTextW = fm.horizontalAdvance ( strText );
#else
        const int iTextW = fm.width ( strText );
#endif
        const int iTextH = fm.height();

        const int iPaddingX = 6;
        const int iPaddingY = 3;
        const int iBoxW     = iTextW + 2 * iPaddingX;
        const int iBoxH     = iTextH + 2 * iPaddingY;

        // Position box above the node by default
        int iBoxX = static_cast<int> ( fNx - iBoxW / 2 );
        int iBoxY = static_cast<int> ( fNy - iBoxH - 12 );

        // Clamp horizontal position to plot margins
        iBoxX = std::max ( kMarginLeft + 4, std::min ( static_cast<int> ( r.right() - iBoxW - 4 ), iBoxX ) );

        // If too close to the top margin, position below the node
        if ( iBoxY < r.top() + 4 )
        {
            iBoxY = static_cast<int> ( fNy + 12 );
        }

        const QRect rectBox ( iBoxX, iBoxY, iBoxW, iBoxH );

        // Render box background and border based on theme
        const QColor colBg     = QColor ( 32, 35, 40 );
        const QColor colBorder = QColor ( 74, 79, 87 );
        const QColor colText   = QColor ( 238, 241, 245 );

        painter.setPen ( QPen ( colBorder, 1 ) );
        painter.setBrush ( colBg );
        painter.drawRoundedRect ( rectBox, 3.0, 3.0 );

        painter.setPen ( colText );
        painter.drawText ( rectBox, Qt::AlignCenter, strText );
    }
}

// ---------------------------------------------------------------------------
// Mouse interaction
// ---------------------------------------------------------------------------
void CEQCurveWidget::mousePressEvent ( QMouseEvent* pEvent )
{
    if ( pEvent->button() == Qt::RightButton )
    {
        float     fDist = 0.0f;
        const int iB    = FindNearestBand ( pEvent->pos(), &fDist );
        if ( iB >= 0 && fDist <= kHitRadius )
        {
            emit bandSoloToggled ( iB );
            return;
        }
    }

    if ( pEvent->button() != Qt::LeftButton )
    {
        QWidget::mousePressEvent ( pEvent );
        return;
    }

    if ( pEvent->modifiers() & Qt::ControlModifier )
    {
        bDrawingCurve          = true;
        const float fDb        = YToDb ( pEvent->pos().y() );
        const float fClampedDb = std::max ( kGainMinDb, std::min ( kGainMaxDb, std::round ( fDb * 10.0f ) / 10.0f ) );
        const float fMouseX    = pEvent->pos().x();

        int   iNearestX = 0;
        float fMinDx    = 1e9f;
        for ( int i = 0; i < kNumBands; ++i )
        {
            const float dx = std::fabs ( fMouseX - FreqToXf ( afBandFrequencies[i] ) );
            if ( dx < fMinDx )
            {
                fMinDx    = dx;
                iNearestX = i;
            }
        }
        if ( iNearestX >= 0 && iNearestX < kNumBands )
        {
            afBandGainDb[iNearestX] = fClampedDb;
            bStaticCurveDirty       = true;
            bEffectiveCurveDirty    = true;
            iSelectedBand           = iNearestX;
            emit bandGainChanged ( iNearestX, fClampedDb );
            emit bandSelected ( iNearestX );
            update();
        }
        return;
    }

    float     fDist = 0.0f;
    const int iB    = FindNearestBand ( pEvent->pos(), &fDist );

    if ( iB >= 0 && fDist <= kHitRadius )
    {
        iSelectedBand = iB;
        bDragging     = true;
        emit bandSelected ( iB );
        UpdateBandTooltip ( iB );
        update();
    }
    else if ( iB >= 0 )
    {
        // Click in plot area but not on a node – still select nearest
        iSelectedBand = iB;
        emit bandSelected ( iB );
        UpdateBandTooltip ( iB );
        update();
    }
}

void CEQCurveWidget::mouseMoveEvent ( QMouseEvent* pEvent )
{
    if ( bDrawingCurve )
    {
        const float fDb        = YToDb ( pEvent->pos().y() );
        const float fClampedDb = std::max ( kGainMinDb, std::min ( kGainMaxDb, std::round ( fDb * 10.0f ) / 10.0f ) );
        const float fMouseX    = pEvent->pos().x();

        int   iNearestX = 0;
        float fMinDx    = 1e9f;
        for ( int i = 0; i < kNumBands; ++i )
        {
            const float dx = std::fabs ( fMouseX - FreqToXf ( afBandFrequencies[i] ) );
            if ( dx < fMinDx )
            {
                fMinDx    = dx;
                iNearestX = i;
            }
        }
        if ( iNearestX >= 0 && iNearestX < kNumBands )
        {
            if ( std::fabs ( afBandGainDb[iNearestX] - fClampedDb ) > 0.05f )
            {
                afBandGainDb[iNearestX] = fClampedDb;
                bStaticCurveDirty       = true;
                bEffectiveCurveDirty    = true;
                iSelectedBand           = iNearestX;
                emit bandGainChanged ( iNearestX, fClampedDb );
                emit bandSelected ( iNearestX );
                update();
            }
        }
        return;
    }

    if ( !bDragging )
    {
        float     fDist = 0.0f;
        const int iB    = FindNearestBand ( pEvent->pos(), &fDist );

        if ( iB >= 0 && fDist <= kHitRadius )
        {
            UpdateBandTooltip ( iB );
        }
        else
        {
            UpdateBandTooltip ( -1, false );
        }

        return;
    }

    if ( bDragging && iSelectedBand >= 0 )
    {
        bool bChanged = false;

        // Handle Gain Dragging (Vertical)
        const float fDb        = YToDb ( pEvent->pos().y() );
        const float fClampedDb = std::max ( kGainMinDb, std::min ( kGainMaxDb, std::round ( fDb * 10.0f ) / 10.0f ) );

        if ( std::abs ( afBandGainDb[iSelectedBand] - fClampedDb ) > 0.001f )
        {
            afBandGainDb[iSelectedBand] = fClampedDb;
            bStaticCurveDirty           = true;
            bEffectiveCurveDirty        = true;
            emit bandGainChanged ( iSelectedBand, fClampedDb );
            bChanged = true;
        }

        // Handle Frequency Dragging (Horizontal)
        const float fFreqHz = XToFreq ( pEvent->pos().x() );

        // Prevent band crossover with a 10% dynamic safety margin:
        float fMinFreq = ( iSelectedBand > 0 ) ? afBandFrequencies[iSelectedBand - 1] * 1.10f : kFreqMin;
        float fMaxFreq = ( iSelectedBand < kNumBands - 1 ) ? afBandFrequencies[iSelectedBand + 1] * 0.90f : kFreqMax;

        fMinFreq = std::max ( kFreqMin, fMinFreq );
        fMaxFreq = std::min ( kFreqMax, fMaxFreq );

        const float fClampedFreq = std::max ( fMinFreq, std::min ( fMaxFreq, fFreqHz ) );

        if ( std::fabs ( afBandFrequencies[iSelectedBand] - fClampedFreq ) > 0.1f )
        {
            afBandFrequencies[iSelectedBand] = fClampedFreq;
            bStaticCurveDirty                = true;
            bEffectiveCurveDirty             = true;
            emit bandFrequencyChanged ( iSelectedBand, fClampedFreq );
            bChanged = true;
        }

        UpdateBandTooltip ( iSelectedBand );

        if ( bChanged )
        {
            update();
        }
    }
}

void CEQCurveWidget::mouseReleaseEvent ( QMouseEvent* pEvent )
{
    Q_UNUSED ( pEvent )
    bDragging     = false;
    bDrawingCurve = false;
}

void CEQCurveWidget::mouseDoubleClickEvent ( QMouseEvent* pEvent )
{
    float     fDist = 0.0f;
    const int iB    = FindNearestBand ( pEvent->pos(), &fDist );

    if ( iB >= 0 && fDist <= kHitRadius )
    {
        afBandGainDb[iB]     = 0.0f;
        bStaticCurveDirty    = true;
        bEffectiveCurveDirty = true;
        iSelectedBand        = iB;
        emit bandGainReset ( iB );
        emit bandSelected ( iB );
        update();
    }
}

void CEQCurveWidget::wheelEvent ( QWheelEvent* pEvent )
{
    if ( iSelectedBand < 0 || iSelectedBand >= kNumBands )
    {
        return;
    }

    // Check if mouse is reasonably near the selected node
    float fDist = 0.0f;
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    const int iB = FindNearestBand ( pEvent->position(), &fDist );
#else
    const int iB = FindNearestBand ( pEvent->posF(), &fDist );
#endif

    if ( iB != iSelectedBand && fDist > kHitRadius * 3 )
    {
        return;
    }

    int deltaX = pEvent->angleDelta().x();
    int deltaY = pEvent->angleDelta().y();

    // Map Shift + vertical scroll to horizontal scroll if the OS/Qt doesn't do it automatically
    if ( pEvent->modifiers() & Qt::ShiftModifier )
    {
        if ( deltaX == 0 && deltaY != 0 )
        {
            deltaX = deltaY;
            deltaY = 0;
        }
    }

    bool bChanged = false;

    if ( deltaX != 0 )
    {
        // Scroll horizontally -> adjust frequency by 0.1 semitones per detent (120 units of delta)
        const float fFactor = std::pow ( 2.0f, static_cast<float> ( deltaX ) / 14400.0f );
        const float fFreqHz = afBandFrequencies[iSelectedBand] * fFactor;

        // Prevent band crossover with a 10% dynamic safety margin:
        float fMinFreq = ( iSelectedBand > 0 ) ? afBandFrequencies[iSelectedBand - 1] * 1.10f : kFreqMin;
        float fMaxFreq = ( iSelectedBand < kNumBands - 1 ) ? afBandFrequencies[iSelectedBand + 1] * 0.90f : kFreqMax;

        fMinFreq = std::max ( kFreqMin, fMinFreq );
        fMaxFreq = std::min ( kFreqMax, fMaxFreq );

        const float fClampedFreq = std::max ( fMinFreq, std::min ( fMaxFreq, fFreqHz ) );

        if ( std::fabs ( afBandFrequencies[iSelectedBand] - fClampedFreq ) > 0.1f )
        {
            afBandFrequencies[iSelectedBand] = fClampedFreq;
            bStaticCurveDirty                = true;
            bEffectiveCurveDirty             = true;
            emit bandFrequencyChanged ( iSelectedBand, fClampedFreq );
            bChanged = true;
        }
    }

    if ( deltaY != 0 )
    {
        if ( pEvent->modifiers() & Qt::ControlModifier )
        {
            // Ctrl + Wheel -> adjust gain
            const float fDelta = ( deltaY > 0 ) ? 0.1f : -0.1f;
            const float fNewDb =
                std::max ( kGainMinDb, std::min ( kGainMaxDb, std::round ( ( afBandGainDb[iSelectedBand] + fDelta ) * 10.0f ) / 10.0f ) );

            if ( std::abs ( afBandGainDb[iSelectedBand] - fNewDb ) > 0.001f )
            {
                afBandGainDb[iSelectedBand] = fNewDb;
                bStaticCurveDirty           = true;
                bEffectiveCurveDirty        = true;
                emit bandGainChanged ( iSelectedBand, fNewDb );
                bChanged = true;
            }
        }
        else
        {
            // Vertical wheel -> adjust Q (bandwidth)
            const float fFactor = std::pow ( 2.0f, static_cast<float> ( deltaY ) / 1200.0f );
            const float fNewQ   = std::max ( 0.1f, std::min ( 20.0f, std::round ( ( afBandQ[iSelectedBand] * fFactor ) * 100.0f ) / 100.0f ) );

            if ( std::fabs ( afBandQ[iSelectedBand] - fNewQ ) > 0.01f )
            {
                afBandQ[iSelectedBand] = fNewQ;
                bStaticCurveDirty      = true;
                bEffectiveCurveDirty   = true;
                emit bandQChanged ( iSelectedBand, fNewQ );
                bChanged = true;
            }
        }
    }

    if ( bChanged )
    {
        UpdateBandTooltip ( iSelectedBand );
        update();
    }
}
