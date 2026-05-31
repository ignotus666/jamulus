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

#include "outputbandmeter.h"
#include "uicolors.h"
#include <QPainter>

COutputBandMeter::COutputBandMeter ( QWidget* parent ) : QWidget ( parent ), bDarkTheme ( true )
{
    for ( float& fLevel : afLevels )
    {
        fLevel = 0.0f;
    }

    setMinimumSize ( 180, 72 );
    setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

void COutputBandMeter::SetDarkTheme ( const bool bEnable )
{
    if ( bDarkTheme != bEnable )
    {
        bDarkTheme = bEnable;
        update();
    }
}

void COutputBandMeter::SetLevels ( const CVector<float>& vecLevels )
{
    for ( int iBand = 0; iBand < kBandCount; ++iBand )
    {
        if ( iBand < vecLevels.Size() )
        {
            afLevels[iBand] = qBound ( 0.0f, vecLevels[iBand], 1.0f );
        }
        else
        {
            afLevels[iBand] = 0.0f;
        }
    }

    update();
}

void COutputBandMeter::SetBandCenters ( const QVector<int>& vecBandCenters )
{
    vecBandCentersPx = vecBandCenters;
    update();
}

void COutputBandMeter::paintEvent ( QPaintEvent* pEvent )
{
    Q_UNUSED ( pEvent )

    QPainter painter ( this );
    painter.setRenderHint ( QPainter::Antialiasing, true );
    const SControlPalette palette = GetControlPalette ( bDarkTheme );

    const int iMargin = 2;
    const int iGap    = 2;
    const int iCount  = kBandCount;
    const int iH      = qMax ( 1, height() - 2 * iMargin );
    const int iWAvail = qMax ( 1, width() - 2 * iMargin - ( iCount - 1 ) * iGap );
    const int iBarW   = qMax ( 1, iWAvail / iCount );

    // Segmented meter parameters
    constexpr int iSegmentCount = 16; // Match NUM_STEPS_LED_BAR from CLevelMeter
    const int     iGapPx        = ( iH > 60 ) ? 1 : 0;
    const int     iTotalGapPx   = std::max ( 0, ( iSegmentCount - 1 ) * iGapPx );
    const int     iUsableH      = std::max ( iSegmentCount, iH - iTotalGapPx );
    const double  dBottomWeight = 1.35;
    const double  dTopWeight    = 0.65;

    // Gradient stops (copied from CLevelMeter)
    const auto ColorAt = [] ( const double dPos ) {
        struct TStop
        {
            double dPos;
            QColor c;
        };
        const TStop  aStops[]    = { { 0.00, QColor ( 48, 230, 75 ) },
                                     { 0.50, QColor ( 48, 230, 75 ) },
                                     { 0.68, QColor ( 245, 210, 50 ) },
                                     { 0.84, QColor ( 245, 155, 40 ) },
                                     { 1.00, QColor ( 235, 60, 55 ) } };
        const double dClampedPos = std::max ( 0.0, std::min ( 1.0, dPos ) );
        for ( int i = 0; i < 4; ++i )
        {
            const TStop& s0 = aStops[i];
            const TStop& s1 = aStops[i + 1];
            if ( dClampedPos <= s1.dPos )
            {
                const double dDenom = std::max ( 1e-9, s1.dPos - s0.dPos );
                const double t      = ( dClampedPos - s0.dPos ) / dDenom;
                return QColor::fromRgbF ( s0.c.redF() + ( s1.c.redF() - s0.c.redF() ) * t,
                                          s0.c.greenF() + ( s1.c.greenF() - s0.c.greenF() ) * t,
                                          s0.c.blueF() + ( s1.c.blueF() - s0.c.blueF() ) * t,
                                          1.0 );
            }
        }
        return aStops[4].c;
    };

    const int minGapPx = 3; // Minimum gap in pixels between bars
    for ( int iBand = 0; iBand < iCount; ++iBand )
    {
        int barLeft, barRight;
        if ( vecBandCentersPx.size() == kBandCount && vecBandCentersPx[iBand] >= 0 )
        {
            int center = vecBandCentersPx[iBand];
            // Calculate virtual centers for edges
            int prevCenter, nextCenter;
            if ( iBand == 0 )
            {
                int delta  = vecBandCentersPx[1] - vecBandCentersPx[0];
                prevCenter = vecBandCentersPx[0] - delta;
            }
            else
            {
                prevCenter = vecBandCentersPx[iBand - 1];
            }
            if ( iBand == kBandCount - 1 )
            {
                int delta  = vecBandCentersPx[kBandCount - 1] - vecBandCentersPx[kBandCount - 2];
                nextCenter = vecBandCentersPx[kBandCount - 1] + delta;
            }
            else
            {
                nextCenter = vecBandCentersPx[iBand + 1];
            }
            // Compute left and right edge as midpoint to neighbours, minus half the gap
            barLeft  = ( center + prevCenter ) / 2 + ( minGapPx / 2 );
            barRight = ( center + nextCenter ) / 2 - ( minGapPx / 2 );
            // Clamp to widget bounds
            barLeft  = std::max ( iMargin, barLeft );
            barRight = std::min ( width() - iMargin, barRight );
            if ( barRight < barLeft + 2 )
                barRight = barLeft + 2; // Minimum width
        }
        else
        {
            // Fallback: even spacing
            int iX   = iMargin + iBand * ( iBarW + iGap );
            barLeft  = iX;
            barRight = iX + iBarW;
        }
        int          barW = barRight - barLeft;
        const int    iY   = iMargin;
        QRect        barRect ( barLeft, iY, barW, iH );
        const QColor backgroundFill = bDarkTheme ? QColor ( 28, 32, 36, 90 ) : QColor ( 245, 246, 248, 220 );
        painter.fillRect ( barRect, backgroundFill );

        // Segmented rendering
        double dNormValue = std::max ( 0.0, std::min ( 1.0, static_cast<double> ( afLevels[iBand] ) ) );
        double dWeightSum = 0.0;
        for ( int iSegment = 0; iSegment < iSegmentCount; ++iSegment )
        {
            const double dPos    = static_cast<double> ( iSegment ) / std::max ( 1, iSegmentCount - 1 );
            const double dWeight = dBottomWeight + ( dTopWeight - dBottomWeight ) * dPos;
            dWeightSum += dWeight;
        }
        int    iYCursor   = iY + iH;
        double dCumWeight = 0.0;
        for ( int iSegment = 0; iSegment < iSegmentCount; ++iSegment )
        {
            const double dPos     = static_cast<double> ( iSegment ) / std::max ( 1, iSegmentCount - 1 );
            const double dWeight  = dBottomWeight + ( dTopWeight - dBottomWeight ) * dPos;
            const int    iStartPx = static_cast<int> ( ( dCumWeight * iUsableH ) / std::max ( 1e-9, dWeightSum ) + 0.5 );
            dCumWeight += dWeight;
            const int iEndPx = static_cast<int> ( ( dCumWeight * iUsableH ) / std::max ( 1e-9, dWeightSum ) + 0.5 );
            const int iSegH  = std::max ( 1, iEndPx - iStartPx );
            iYCursor -= iSegH;
            QRect segRect ( barLeft, iYCursor, barW, iSegH );
            if ( !segRect.isValid() )
                continue;
            const double dSegStart = static_cast<double> ( iSegment ) / iSegmentCount;
            const bool   bActive   = dNormValue > dSegStart;
            if ( bActive )
            {
                const double dSegMid = std::min ( 1.0, ( iSegment + 0.5 ) / iSegmentCount );
                const QColor cSeg    = ColorAt ( dSegMid );
                painter.setPen ( Qt::NoPen );
                painter.setBrush ( cSeg );
                const qreal dRadius = std::min ( 2.2, std::min ( segRect.width(), segRect.height() ) / 2.0 );
                painter.drawRoundedRect ( QRectF ( segRect ), dRadius, dRadius );
            }
            else
            {
                painter.fillRect ( segRect, bDarkTheme ? QColor ( 28, 35, 45 ) : palette.trackBackground );
            }
            if ( iSegment + 1 < iSegmentCount )
            {
                iYCursor -= iGapPx;
            }
        }
    }
}
