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
QString MakeBandTooltipText ( const int iBand, const float fFreqHz, const float fGainDb )
{
    const QString strFreq = ( fFreqHz >= 1000.0f ) ? QString::number ( fFreqHz / 1000.0f, 'f', 2 ) + QObject::tr ( " kHz" )
                                                   : QString::number ( fFreqHz, 'f', 1 ) + QObject::tr ( " Hz" );
    const QString strGain = ( fGainDb >= 0.0f ) ? QString ( "+%1 dB" ).arg ( QString::number ( fGainDb, 'f', 1 ) )
                                                : QString ( "%1 dB" ).arg ( QString::number ( fGainDb, 'f', 1 ) );
    return QObject::tr ( "Band %1 | %2 | %3" ).arg ( iBand + 1 ).arg ( strFreq ).arg ( strGain );
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
    bEQBypassed ( false ),
    bStaticCurveDirty ( true ),
    bEffectiveCurveDirty ( true ),
    pBandTooltip ( nullptr )
{
    for ( int i = 0; i < kNumBands; ++i )
    {
        afBandGainDb[i]          = 0.0f;
        afBandGainReductionDb[i] = 0.0f;
        afBandFrequencies[i]     = CAudioEqualizer::GetDefaultBandFrequency ( i );
        afBandQ[i]               = 1.0f;
        afSpectrumLevels[i]      = 0.0f;
    }

    setMinimumSize ( 300, 120 );
    setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setMouseTracking ( true );
    setFocusPolicy ( Qt::ClickFocus );

    pBandTooltip = new QLabel ( this );
    pBandTooltip->setWindowFlags ( Qt::ToolTip | Qt::FramelessWindowHint );
    pBandTooltip->setAttribute ( Qt::WA_ShowWithoutActivating, true );
    pBandTooltip->setAttribute ( Qt::WA_TransparentForMouseEvents, true );
    pBandTooltip->setAttribute ( Qt::WA_StyledBackground, true );
    pBandTooltip->setAlignment ( Qt::AlignLeft | Qt::AlignVCenter );
    pBandTooltip->setWordWrap ( false );

    UpdateBandTooltipStyle();
}

void CEQCurveWidget::UpdateBandTooltipStyle()
{
    if ( !pBandTooltip )
    {
        return;
    }

    if ( bDarkTheme )
    {
        pBandTooltip->setStyleSheet ( QStringLiteral ( "QLabel { font-size: 9px; color: #eef1f5; background-color: #202328; border: 1px solid "
                                                       "#4a4f57; border-radius: 3px; padding: 2px 4px; }" ) );
    }
    else
    {
        pBandTooltip->setStyleSheet ( QStringLiteral ( "QLabel { font-size: 9px; color: #1c1e22; background-color: #fafafa; border: 1px solid "
                                                       "#b8bec8; border-radius: 3px; padding: 2px 4px; }" ) );
    }
}

void CEQCurveWidget::UpdateBandTooltip ( const int iBand, const bool bVisible )
{
    if ( !pBandTooltip )
    {
        return;
    }

    if ( !bVisible || iBand < 0 || iBand >= kNumBands )
    {
        pBandTooltip->hide();
        return;
    }

    pBandTooltip->setText ( MakeBandTooltipText ( iBand, afBandFrequencies[iBand], afBandGainDb[iBand] ) );
    pBandTooltip->adjustSize();

    const int iNodeX = static_cast<int> ( std::round ( FreqToXf ( afBandFrequencies[iBand] ) ) );
    const int iNodeY = static_cast<int> ( std::round ( DbToYf ( afBandGainDb[iBand] ) ) );
    QPoint    pos    = mapToGlobal ( QPoint ( iNodeX + 16, iNodeY - pBandTooltip->height() - 10 ) );

    const QRect screenRect = QApplication::primaryScreen() ? QApplication::primaryScreen()->availableGeometry() : QRect ( 0, 0, width(), height() );
    if ( pos.x() + pBandTooltip->width() > screenRect.right() )
    {
        pos.setX ( screenRect.right() - pBandTooltip->width() - 8 );
    }
    if ( pos.y() < screenRect.top() )
    {
        pos.setY ( mapToGlobal ( QPoint ( iNodeX + 16, iNodeY + 18 ) ).y() );
    }
    if ( pos.x() < screenRect.left() )
    {
        pos.setX ( screenRect.left() + 8 );
    }

    pBandTooltip->move ( pos );
    pBandTooltip->show();
    pBandTooltip->raise();
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
    for ( int iBand = 0; iBand < kNumBands; ++iBand )
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
    if ( bEQBypassed != bBypassed )
    {
        bEQBypassed = bBypassed;
        if ( bEQBypassed )
        {
            // Clear real-time analyzer data when bypassed
            for ( int i = 0; i < kNumBands; ++i )
            {
                afSpectrumLevels[i]      = 0.0f;
                afBandGainReductionDb[i] = 0.0f;
            }
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

    // --- Background ---
    const QColor colBg   = GetControlPalette ( bDarkTheme ).background;
    const QColor colGrid = bDarkTheme ? QColor ( 48, 52, 58 ) : QColor ( 215, 218, 224 );
    const QColor colZero = bDarkTheme ? QColor ( 72, 78, 86 ) : QColor ( 180, 185, 195 );
    const QColor colText = bDarkTheme ? QColor ( 150, 158, 168 ) : QColor ( 100, 110, 120 );

    painter.fillRect ( rect(), colBg );

    // --- Plot area background ---
    const QColor colPlotBg = bDarkTheme ? QColor ( 28, 31, 36 ) : QColor ( 255, 255, 255 );
    painter.fillRect ( r.toRect(), colPlotBg );

    // --- Grid: horizontal dB lines ---
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

    // --- Grid: vertical frequency lines ---
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

    // --- Draw spectrum overlay in the background ---
    bool bAnySpectrum = false;
    for ( int i = 0; i < kNumBands; ++i )
    {
        if ( afSpectrumLevels[i] > 0.001f )
        {
            bAnySpectrum = true;
            break;
        }
    }

    if ( bAnySpectrum && !bEQBypassed )
    {
        QPainterPath spectrumPath;
        // Start at bottom-left corner of plot area (20 Hz)
        spectrumPath.moveTo ( FreqToXf ( kFreqMin ), r.bottom() );

        // Add each band level point
        for ( int iBand = 0; iBand < kNumBands; ++iBand )
        {
            const float fNx = FreqToXf ( afBandFrequencies[iBand] );
            const float fNy = r.bottom() - afSpectrumLevels[iBand] * r.height();
            spectrumPath.lineTo ( fNx, fNy );
        }

        // End at bottom-right corner of plot area (20000 Hz)
        spectrumPath.lineTo ( FreqToXf ( kFreqMax ), r.bottom() );
        spectrumPath.closeSubpath();

        // Create a beautiful, semi-transparent glowing gradient matching the level colors
        QLinearGradient spectrumGrad ( r.left(), r.bottom(), r.left(), r.top() );
        if ( bDarkTheme )
        {
            spectrumGrad.setColorAt ( 0.0, QColor ( 0, 200, 255, 0 ) );   // transparent at bottom
            spectrumGrad.setColorAt ( 0.4, QColor ( 48, 230, 75, 45 ) );  // glowing green
            spectrumGrad.setColorAt ( 0.7, QColor ( 245, 155, 40, 60 ) ); // orange near top
            spectrumGrad.setColorAt ( 1.0, QColor ( 235, 60, 55, 80 ) );  // red at top
        }
        else
        {
            spectrumGrad.setColorAt ( 0.0, QColor ( 0, 200, 255, 0 ) );
            spectrumGrad.setColorAt ( 0.4, QColor ( 60, 220, 90, 35 ) );
            spectrumGrad.setColorAt ( 0.7, QColor ( 245, 155, 40, 50 ) );
            spectrumGrad.setColorAt ( 1.0, QColor ( 235, 60, 55, 65 ) );
        }

        painter.fillPath ( spectrumPath, spectrumGrad );

        // Draw a thin outline to make it look extremely sharp and premium
        const QColor colSpectrumLine = bDarkTheme ? QColor ( 48, 230, 75, 120 ) : QColor ( 60, 220, 90, 100 );
        painter.setPen ( QPen ( colSpectrumLine, 1.0, Qt::SolidLine ) );
        painter.setBrush ( Qt::NoBrush );

        QPainterPath linePath;
        linePath.moveTo ( FreqToXf ( kFreqMin ), r.bottom() );
        for ( int iBand = 0; iBand < kNumBands; ++iBand )
        {
            linePath.lineTo ( FreqToXf ( afBandFrequencies[iBand] ), r.bottom() - afSpectrumLevels[iBand] * r.height() );
        }
        linePath.lineTo ( FreqToXf ( kFreqMax ), r.bottom() );
        painter.drawPath ( linePath );
    }

    // --- Compute and draw response curves ---
    if ( bStaticCurveDirty )
    {
        ComputeResponseCurve ( afBandGainDb, vecStaticCurveCache );
        bStaticCurveDirty = false;
    }

    bool bAnyReduction = false;
    if ( !bEQBypassed )
    {
        for ( int iBand = 0; iBand < kNumBands; ++iBand )
        {
            if ( afBandGainReductionDb[iBand] > 0.05f )
            {
                bAnyReduction = true;
                break;
            }
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

        const QColor colFill = bDarkTheme ? QColor ( 255, 140, 40, 50 ) : QColor ( 220, 100, 20, 40 );
        painter.fillPath ( fillPath, colFill );
    }

    // Draw static curve (thin, subdued)
    if ( bAnyReduction && vecStaticCurve.size() > 1 )
    {
        QPainterPath staticPath;
        staticPath.moveTo ( vecStaticCurve.first() );

        for ( int i = 1; i < vecStaticCurve.size(); ++i )
        {
            staticPath.lineTo ( vecStaticCurve[i] );
        }

        const QColor colStatic = bDarkTheme ? QColor ( 54, 207, 255, 70 ) : QColor ( 50, 150, 200, 60 );
        painter.setPen ( QPen ( colStatic, 1.2 ) );
        painter.setBrush ( Qt::NoBrush );
        painter.drawPath ( staticPath );
    }

    // Draw effective curve (main, bold)
    if ( vecEffectiveCurve.size() > 1 )
    {
        // Gradient fill under the curve
        const float fZeroY = DbToYf ( 0.0f );
        if ( !bEQBypassed )
        {
            QPainterPath areaPath;
            areaPath.moveTo ( vecEffectiveCurve.first().x(), fZeroY );
            for ( int i = 0; i < vecEffectiveCurve.size(); ++i )
            {
                areaPath.lineTo ( vecEffectiveCurve[i] );
            }
            areaPath.lineTo ( vecEffectiveCurve.last().x(), fZeroY );
            areaPath.closeSubpath();

            const QColor colAreaAbove = bDarkTheme ? QColor ( 54, 207, 255, 22 ) : QColor ( 50, 150, 200, 18 );
            painter.fillPath ( areaPath, colAreaAbove );
        }

        QPainterPath effectivePath;
        effectivePath.moveTo ( vecEffectiveCurve.first() );

        for ( int i = 1; i < vecEffectiveCurve.size(); ++i )
        {
            effectivePath.lineTo ( vecEffectiveCurve[i] );
        }

        QColor colCurve;
        if ( bEQBypassed )
        {
            colCurve = bDarkTheme ? QColor ( 100, 105, 115 ) : QColor ( 170, 175, 180 );
        }
        else
        {
            colCurve = bDarkTheme ? QColor ( 54, 207, 255 ) : QColor ( 50, 150, 200 );
        }
        painter.setPen ( QPen ( colCurve, 2.0 ) );
        painter.setBrush ( Qt::NoBrush );
        painter.drawPath ( effectivePath );
    }

    // --- Band nodes ---
    for ( int iBand = 0; iBand < kNumBands; ++iBand )
    {
        const float fNx = FreqToXf ( afBandFrequencies[iBand] );
        const float fNy = DbToYf ( afBandGainDb[iBand] );

        const bool bSelected = ( iBand == iSelectedBand );
        const int  iRad      = ( bSelected && !bEQBypassed ) ? kNodeRadius + 2 : kNodeRadius;

        // Glow for selected node
        if ( bSelected && !bEQBypassed )
        {
            const QColor colGlow = bDarkTheme ? QColor ( 54, 207, 255, 60 ) : QColor ( 50, 150, 200, 50 );
            painter.setPen ( Qt::NoPen );
            painter.setBrush ( colGlow );
            painter.drawEllipse ( QPointF ( fNx, fNy ), iRad + 6, iRad + 6 );
        }

        // Gain reduction indicator: vertical line from static to effective
        if ( afBandGainReductionDb[iBand] > 0.05f && !bEQBypassed )
        {
            const float  fEy   = DbToYf ( afBandGainDb[iBand] - afBandGainReductionDb[iBand] );
            const QColor colGR = bDarkTheme ? QColor ( 255, 140, 40, 180 ) : QColor ( 220, 100, 20, 160 );
            painter.setPen ( QPen ( colGR, 2.5, Qt::SolidLine, Qt::RoundCap ) );
            painter.drawLine ( QPointF ( fNx, fNy ), QPointF ( fNx, fEy ) );

            // Small effective-position dot
            painter.setPen ( Qt::NoPen );
            painter.setBrush ( colGR );
            painter.drawEllipse ( QPointF ( fNx, fEy ), 3, 3 );
        }

        // Node circle
        QColor colNode;
        if ( bEQBypassed )
        {
            colNode = bDarkTheme ? QColor ( 80, 85, 95 ) : QColor ( 180, 185, 190 );
        }
        else
        {
            colNode = bDarkTheme ? ( bSelected ? QColor ( 118, 244, 255 ) : QColor ( 54, 207, 255 ) )
                                 : ( bSelected ? QColor ( 30, 120, 180 ) : QColor ( 50, 150, 200 ) );
        }

        const QColor colBorder = bDarkTheme ? QColor ( 20, 22, 26 ) : QColor ( 255, 255, 255 );

        painter.setPen ( QPen ( colBorder, 2 ) );
        painter.setBrush ( colNode );
        painter.drawEllipse ( QPointF ( fNx, fNy ), iRad, iRad );
    }

    // --- Plot area border ---
    painter.setPen ( QPen ( colGrid, 1 ) );
    painter.setBrush ( Qt::NoBrush );
    painter.drawRect ( r );
}

// ---------------------------------------------------------------------------
// Mouse interaction
// ---------------------------------------------------------------------------
void CEQCurveWidget::mousePressEvent ( QMouseEvent* pEvent )
{
    if ( bEQBypassed )
    {
        return;
    }

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
    if ( bEQBypassed )
    {
        return;
    }

    if ( !bDragging )
    {
        float     fDist = 0.0f;
        const int iB    = FindNearestBand ( pEvent->pos(), &fDist );

        if ( iB >= 0 && fDist <= kHitRadius && iB != iSelectedBand )
        {
            iSelectedBand = iB;
            emit bandSelected ( iB );
            update();
        }

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

        // --- 1. Handle Gain Dragging (Vertical) ---
        const float fDb = YToDb ( pEvent->pos().y() );
        const int   iClampedDb =
            std::max ( static_cast<int> ( kGainMinDb ), std::min ( static_cast<int> ( kGainMaxDb ), static_cast<int> ( std::round ( fDb ) ) ) );

        if ( static_cast<int> ( std::round ( afBandGainDb[iSelectedBand] ) ) != iClampedDb )
        {
            afBandGainDb[iSelectedBand] = static_cast<float> ( iClampedDb );
            bStaticCurveDirty           = true;
            bEffectiveCurveDirty        = true;
            emit bandGainChanged ( iSelectedBand, iClampedDb );
            bChanged = true;
        }

        // --- 2. Handle Frequency Dragging (Horizontal) ---
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
    if ( bEQBypassed )
    {
        return;
    }

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
    if ( bEQBypassed )
    {
        return;
    }

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

    const int iDelta = ( pEvent->angleDelta().y() > 0 ) ? 1 : -1;
    const int iNewDb =
        std::max ( static_cast<int> ( kGainMinDb ),
                   std::min ( static_cast<int> ( kGainMaxDb ), static_cast<int> ( std::round ( afBandGainDb[iSelectedBand] ) ) + iDelta ) );

    afBandGainDb[iSelectedBand] = static_cast<float> ( iNewDb );
    bStaticCurveDirty           = true;
    bEffectiveCurveDirty        = true;
    emit bandGainChanged ( iSelectedBand, iNewDb );
    UpdateBandTooltip ( iSelectedBand );
    update();
}
