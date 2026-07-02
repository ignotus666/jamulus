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
#include <QLabel>
#include <QScreen>
#include <QWheelEvent>
#include <QResizeEvent>
#include <cmath>

namespace
{
QString MakeBandTooltipText ( const float fFreqHz, const float fGainDb, const float fQ )
{
    const QString strFreq = ( fFreqHz >= 1000.0f ) ? QString::number ( fFreqHz / 1000.0f, 'f', 2 ) + QObject::tr ( " kHz" )
                                                   : QString::number ( fFreqHz, 'f', 1 ) + QObject::tr ( " Hz" );
    const QString strGain = ( fGainDb >= 0.0f ) ? QString ( "+%1 dB" ).arg ( QString::number ( fGainDb, 'f', 1 ) )
                                                : QString ( "%1 dB" ).arg ( QString::number ( fGainDb, 'f', 1 ) );
    return QObject::tr ( "%1 | %2 | Q: %3" ).arg ( strFreq ).arg ( strGain ).arg ( QString::number ( fQ, 'f', 1 ) );
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
CEQCurveWidget::CEQCurveWidget ( QWidget* parent ) :
    QWidget ( parent ),
    iSampleRateHz ( 48000 ),
    iSelectedBand ( 0 ),
    bDragging ( false ),
    bDarkTheme ( true ),
    bStaticCurveDirty ( true ),
    bEffectiveCurveDirty ( true ),
    pBandTooltip ( nullptr ),
    iTooltipBand ( -1 )
{
    for ( int i = 0; i < kNumBands; ++i )
    {
        afBandGainDb[i]          = 0.0f;
        afBandGainReductionDb[i] = 0.0f;
        afBandFrequencies[i]     = CAudioEqualizer::GetDefaultBandFrequency ( i );
        afBandQ[i]               = 1.0f;
    }
    for ( int i = 0; i < kNumSpectrumBands; ++i )
    {
        afSpectrumLevels[i] = 0.0f;
    }

    setMinimumSize ( 300, 120 );
    setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setMouseTracking ( true );
    setFocusPolicy ( Qt::ClickFocus );

    pBandTooltip = nullptr;

    UpdateBandTooltipStyle();
}

void CEQCurveWidget::UpdateBandTooltipStyle()
{
    if ( !pBandTooltip )
    {
        return;
    }

    pBandTooltip->setStyleSheet ( QStringLiteral ( "QLabel { font-size: 9px; color: #eef1f5; background-color: #202328; border: 1px solid "
                                                   "#4a4f57; border-radius: 3px; padding: 2px 4px; }" ) );
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
    bool bChanged = false;
    for ( int iBand = 0; iBand < kNumSpectrumBands; ++iBand )
    {
        const float fOldVal = afSpectrumLevels[iBand];
        float       fNewVal = 0.0f;
        if ( iBand < vecLevels.size() )
        {
            fNewVal = qBound ( 0.0f, vecLevels[iBand], 1.0f );
        }

        if ( std::fabs ( fOldVal - fNewVal ) > 0.02f )
        {
            afSpectrumLevels[iBand] = fNewVal;
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
        // Clear real-time analyzer data when bypassed
        for ( int i = 0; i < kNumSpectrumBands; ++i )
        {
            afSpectrumLevels[i] = 0.0f;
        }
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
}

void CEQCurveWidget::SetDarkTheme ( const bool bEnable )
{
    if ( bDarkTheme != bEnable )
    {
        bDarkTheme = bEnable;
        UpdateBandTooltipStyle();
        update();
    }
}

// ---------------------------------------------------------------------------
// Coordinate transforms
// ---------------------------------------------------------------------------
QRectF CEQCurveWidget::PlotRect() const
{
    return QRectF ( kMarginLeft, kMarginTop, width() - kMarginLeft - kMarginRight, height() - kMarginTop - kMarginBottom );
}

float CEQCurveWidget::FreqToXf ( const float fHz ) const
{
    const QRectF r     = PlotRect();
    const float  fNorm = std::log ( fHz / kFreqMin ) / std::log ( kFreqMax / kFreqMin );
    return r.left() + fNorm * r.width();
}

int CEQCurveWidget::FreqToX ( const float fHz ) const { return static_cast<int> ( FreqToXf ( fHz ) + 0.5f ); }

float CEQCurveWidget::DbToYf ( const float fDb ) const
{
    const QRectF r     = PlotRect();
    const float  fNorm = ( kDisplayMax - fDb ) / ( kDisplayMax - kDisplayMin );
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
// Biquad transfer-function evaluation (analytical, no FFT)
// ---------------------------------------------------------------------------
float CEQCurveWidget::EvalBandMagnitudeDb ( const int iBand, const float fGainDb, const float fFreqHz ) const
{
    // Evaluate |H(e^{jω})| for a peaking EQ biquad at the given frequency.
    // We recompute coefficients here to be independent of the DSP engine's state.
    constexpr float fPi = 3.14159265358979323846f;
    const float     fQ  = afBandQ[iBand];

    const float fBandFreq = afBandFrequencies[iBand];

    if ( fBandFreq <= 0.0f || iSampleRateHz <= 0 )
    {
        return 0.0f;
    }

    const float fA     = std::pow ( 10.0f, fGainDb / 40.0f );
    const float fW0    = 2.0f * fPi * fBandFreq / iSampleRateHz;
    const float fAlpha = std::sin ( fW0 ) / ( 2.0f * fQ );
    const float fCosW0 = std::cos ( fW0 );

    const float b0 = 1.0f + fAlpha * fA;
    const float b1 = -2.0f * fCosW0;
    const float b2 = 1.0f - fAlpha * fA;
    const float a0 = 1.0f + fAlpha / fA;
    const float a1 = -2.0f * fCosW0;
    const float a2 = 1.0f - fAlpha / fA;

    // Normalise coefficients
    const float nb0 = b0 / a0;
    const float nb1 = b1 / a0;
    const float nb2 = b2 / a0;
    const float na1 = a1 / a0;
    const float na2 = a2 / a0;

    // Evaluate H(e^{jω}) at the plot frequency
    const float fW    = 2.0f * fPi * fFreqHz / iSampleRateHz;
    const float cosW  = std::cos ( fW );
    const float sinW  = std::sin ( fW );
    const float cos2W = std::cos ( 2.0f * fW );
    const float sin2W = std::sin ( 2.0f * fW );

    const float numRe = nb0 + nb1 * cosW + nb2 * cos2W;
    const float numIm = -( nb1 * sinW + nb2 * sin2W );
    const float denRe = 1.0f + na1 * cosW + na2 * cos2W;
    const float denIm = -( na1 * sinW + na2 * sin2W );

    const float numMagSq = numRe * numRe + numIm * numIm;
    const float denMagSq = denRe * denRe + denIm * denIm;

    if ( denMagSq < 1e-18f )
    {
        return 0.0f;
    }

    return 10.0f * std::log10 ( numMagSq / denMagSq );
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

    // Draw spectrum overlay in the background
    bool bAnySpectrum = false;
    for ( int i = 0; i < kNumSpectrumBands; ++i )
    {
        if ( afSpectrumLevels[i] > 0.001f )
        {
            bAnySpectrum = true;
            break;
        }
    }

    if ( bAnySpectrum )
    {
        QPainterPath spectrumPath;
        // Start at bottom-left corner of plot area (20 Hz)
        spectrumPath.moveTo ( FreqToXf ( kFreqMin ), r.bottom() );

        // Add each band level point
        for ( int iBand = 0; iBand < kNumSpectrumBands; ++iBand )
        {
            const float fFreq = 30.0f * std::pow ( 16000.0f / 30.0f, static_cast<float> ( iBand ) / ( kNumSpectrumBands - 1 ) );
            const float fNx   = FreqToXf ( fFreq );
            const float fNy   = r.bottom() - afSpectrumLevels[iBand] * r.height();
            spectrumPath.lineTo ( fNx, fNy );
        }

        // End at bottom-right corner of plot area (20000 Hz)
        spectrumPath.lineTo ( FreqToXf ( kFreqMax ), r.bottom() );
        spectrumPath.closeSubpath();

        // Create a semi-transparent glowing gradient matching the level colors
        QLinearGradient spectrumGrad ( r.left(), r.bottom(), r.left(), r.top() );
        spectrumGrad.setColorAt ( 0.0, QColor ( 0, 200, 255, 0 ) );   // transparent at bottom
        spectrumGrad.setColorAt ( 0.4, QColor ( 48, 230, 75, 45 ) );  // glowing green
        spectrumGrad.setColorAt ( 0.7, QColor ( 245, 155, 40, 60 ) ); // orange near top
        spectrumGrad.setColorAt ( 1.0, QColor ( 235, 60, 55, 80 ) );  // red at top

        painter.fillPath ( spectrumPath, spectrumGrad );

        // Draw thin outline
        const QColor colSpectrumLine = QColor ( 48, 230, 75, 120 );
        painter.setPen ( QPen ( colSpectrumLine, 1.0, Qt::SolidLine ) );
        painter.setBrush ( Qt::NoBrush );

        QPainterPath linePath;
        linePath.moveTo ( FreqToXf ( kFreqMin ), r.bottom() );
        for ( int iBand = 0; iBand < kNumSpectrumBands; ++iBand )
        {
            const float fFreq = 30.0f * std::pow ( 16000.0f / 30.0f, static_cast<float> ( iBand ) / ( kNumSpectrumBands - 1 ) );
            linePath.lineTo ( FreqToXf ( fFreq ), r.bottom() - afSpectrumLevels[iBand] * r.height() );
        }
        linePath.lineTo ( FreqToXf ( kFreqMax ), r.bottom() );
        painter.drawPath ( linePath );
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

        painter.setPen ( Qt::NoPen );
        painter.setBrush ( colNode );
        painter.drawEllipse ( QPointF ( fNx, fNy ), iRad, iRad );
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

        const QString strText = MakeBandTooltipText ( afBandFrequencies[iTooltipBand], afBandGainDb[iTooltipBand], afBandQ[iTooltipBand] );

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

    if ( pEvent->button() != Qt::LeftButton )
    {
        QWidget::mousePressEvent ( pEvent );
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
    bDragging = false;
}

void CEQCurveWidget::leaveEvent ( QEvent* pEvent )
{
    Q_UNUSED ( pEvent )
    UpdateBandTooltip ( -1, false );
    QWidget::leaveEvent ( pEvent );
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
    if ( iSelectedBand < 0 )
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

    if ( iB != iSelectedBand || fDist > kHitRadius * 3 )
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
        // Scroll horizontally -> adjust frequency
        // We move the frequency dot by a uniform visual pixel step (e.g. 2 pixels)
        float fX = FreqToXf ( afBandFrequencies[iSelectedBand] );
        fX += ( deltaX > 0 ) ? 2.0f : -2.0f;
        const float fFreqHz = XToFreq ( fX );

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
        // Scroll vertically -> adjust gain
        const float fDelta = ( deltaY > 0 ) ? 0.1f : -0.1f;
        const float fNewDb = std::max ( kGainMinDb, std::min ( kGainMaxDb, std::round ( ( afBandGainDb[iSelectedBand] + fDelta ) * 10.0f ) / 10.0f ) );

        if ( std::abs ( afBandGainDb[iSelectedBand] - fNewDb ) > 0.001f )
        {
            afBandGainDb[iSelectedBand] = fNewDb;
            bStaticCurveDirty           = true;
            bEffectiveCurveDirty        = true;
            emit bandGainChanged ( iSelectedBand, fNewDb );
            bChanged = true;
        }
    }

    if ( bChanged )
    {
        UpdateBandTooltip ( iSelectedBand );
        update();
    }
}
