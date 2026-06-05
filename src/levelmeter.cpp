/******************************************************************************\
 * Copyright (c) 2004-2026
 *
 * Author(s):
 *  Volker Fischer
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
 *
 * Description:
 *  Implements a multi color LED bar
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
\******************************************************************************/

#include "levelmeter.h"
#include <QPainter>

class CLevelMeter::CGradientLevelBar : public QWidget
{
public:
    CGradientLevelBar ( QWidget* parent = nullptr ) :
        QWidget ( parent ),
        iValue ( 0 ),
        iTargetValue ( 0 ),
        iMaxValue ( 100 * NUM_STEPS_LED_BAR ),
        bClip ( false ),
        bDarkTheme ( true )
    {
        setMinimumSize ( QSize ( 1, 1 ) );

        QObject::connect ( &TimerDecay, &QTimer::timeout, this, [this]() {
            if ( iValue > iTargetValue )
            {
                iValue = static_cast<int> ( iValue * 0.82 + iTargetValue * 0.18 );
                if ( iValue - iTargetValue < 10 )
                {
                    iValue = iTargetValue;
                }
                update();
            }
            if ( iValue <= iTargetValue )
            {
                TimerDecay.stop();
            }
        } );
    }

    void SetDarkTheme ( const bool bEnable )
    {
        if ( bDarkTheme != bEnable )
        {
            bDarkTheme = bEnable;
            update();
        }
    }

    void SetRange ( const int iMin, const int iMax )
    {
        Q_UNUSED ( iMin );
        iMaxValue = std::max ( 1, iMax );
    }

    void SetValue ( const int iNewValue )
    {
        const int iClamped = std::max ( 0, std::min ( iMaxValue, iNewValue ) );
        iTargetValue = iClamped;

        if ( iTargetValue > iValue )
        {
            iValue = iTargetValue;
            update();
        }

        if ( iValue > iTargetValue )
        {
            if ( !TimerDecay.isActive() )
            {
                TimerDecay.start ( 30 );
            }
        }
    }

    void SetClip ( const bool bNewClip )
    {
        if ( bClip != bNewClip )
        {
            bClip = bNewClip;
            update();
        }
    }

protected:
    void paintEvent ( QPaintEvent* event ) override
    {
        Q_UNUSED ( event );

        QPainter painter ( this );
        painter.setRenderHint ( QPainter::Antialiasing, true );

        const QRect r = rect();
        if ( !r.isValid() )
            return;

        painter.fillRect ( r, bDarkTheme ? QColor ( 18, 24, 31 ) : QColor ( 240, 242, 245 ) );

        const auto ColorAt = [] ( const double dPos ) {
            struct TStop
            {
                double dPos;
                QColor c;
            };

            const TStop aStops[] = { { 0.00, QColor ( 48, 230, 75 ) },
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

        // Enhanced mode uses discrete meter blocks to emulate hardware segmented meters.
        const int    iSegmentCount = std::max ( 1, NUM_STEPS_LED_BAR );
        const double dNormValue    = std::max ( 0.0, std::min ( 1.0, static_cast<double> ( iValue ) / std::max ( 1, iMaxValue ) ) );
        const int    iGapPx        = ( r.height() > 60 ) ? 1 : 0;

        const int iTotalGapPx = std::max ( 0, ( iSegmentCount - 1 ) * iGapPx );
        const int iUsableH    = std::max ( iSegmentCount, r.height() - iTotalGapPx );

        const double dBottomWeight = 1.35;
        const double dTopWeight    = 0.65;

        double dWeightSum = 0.0;
        for ( int iSegment = 0; iSegment < iSegmentCount; ++iSegment )
        {
            const double dPos    = static_cast<double> ( iSegment ) / std::max ( 1, iSegmentCount - 1 );
            const double dWeight = dBottomWeight + ( dTopWeight - dBottomWeight ) * dPos;
            dWeightSum += dWeight;
        }

        int    iYCursor   = r.bottom() + 1;
        double dCumWeight = 0.0;

        for ( int iSegment = 0; iSegment < iSegmentCount; ++iSegment )
        {
            const double dPos    = static_cast<double> ( iSegment ) / std::max ( 1, iSegmentCount - 1 );
            const double dWeight = dBottomWeight + ( dTopWeight - dBottomWeight ) * dPos;

            const int iStartPx = static_cast<int> ( ( dCumWeight * iUsableH ) / std::max ( 1e-9, dWeightSum ) + 0.5 );
            dCumWeight += dWeight;
            const int iEndPx = static_cast<int> ( ( dCumWeight * iUsableH ) / std::max ( 1e-9, dWeightSum ) + 0.5 );

            const int iSegH = std::max ( 1, iEndPx - iStartPx );
            iYCursor -= iSegH;

            QRect segRect ( r.left(), iYCursor, r.width(), iSegH );
            if ( !segRect.isValid() )
            {
                continue;
            }

            const double dSegStart = static_cast<double> ( iSegment ) / iSegmentCount;
            bool         bActive   = dNormValue > dSegStart;

            if ( iSegment == iSegmentCount - 1 && bClip )
            {
                bActive = true;
            }

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
                painter.fillRect ( segRect, bDarkTheme ? QColor ( 28, 35, 45 ) : QColor ( 214, 220, 228 ) );
            }

            if ( iSegment + 1 < iSegmentCount )
            {
                iYCursor -= iGapPx;
            }
        }
    }

private:
    int    iValue;
    int    iTargetValue;
    int    iMaxValue;
    bool   bClip;
    bool   bDarkTheme;
    QTimer TimerDecay;
};

/* Implementation *************************************************************/
CLevelMeter::CLevelMeter ( QWidget* parent ) : QWidget ( parent ), eLevelMeterType ( MT_BAR_WIDE ), bNormalModeStyle ( false ), bDarkTheme ( true )
{
    // Enhanced-mode fixed-gradient reveal bar.
    pGradientBar = new CGradientLevelBar();
    pGradientBar->SetRange ( 0, 100 * NUM_STEPS_LED_BAR );

    // setup stacked layout for meter type switching mechanism
    pMinStackedLayout = new CMinimumStackedLayout ( this );
    pMinStackedLayout->setContentsMargins ( 0, 0, 0, 0 );
    pMinStackedLayout->addWidget ( pGradientBar );
    pMinStackedLayout->setAlignment ( pGradientBar, Qt::AlignHCenter );

    // according to QScrollArea description: "When using a scroll area to display the
    // contents of a custom widget, it is important to ensure that the size hint of
    // the child widget is set to a suitable value."
    pGradientBar->setMinimumSize ( QSize ( 1, 1 ) );

    // update the meter type (using the default value of the meter type)
    SetLevelMeterType ( eLevelMeterType );

    // setup clip indicator timer
    TimerClip.setSingleShot ( true );
    TimerClip.setInterval ( CLIP_IND_TIME_OUT_MS );

    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerClip, &QTimer::timeout, this, &CLevelMeter::ClipReset );
}

void CLevelMeter::SetNormalModeStyle ( const bool bEnable )
{
    if ( bNormalModeStyle != bEnable )
    {
        bNormalModeStyle = bEnable;
        SetLevelMeterType ( eLevelMeterType );
    }
}

void CLevelMeter::SetDarkTheme ( const bool bEnable )
{
    if ( bDarkTheme != bEnable )
    {
        bDarkTheme = bEnable;
        pGradientBar->SetDarkTheme ( bEnable );
        SetBarMeterStyleAndClipStatus ( eLevelMeterType, false );
    }
}

CLevelMeter::~CLevelMeter() {}

void CLevelMeter::SetLevelMeterType ( const ELevelMeterType eNType )
{
    eLevelMeterType = eNType;

    // update bar meter style and reset clip state
    SetBarMeterStyleAndClipStatus ( eNType, false );
}

void CLevelMeter::SetBarMeterStyleAndClipStatus ( const ELevelMeterType eNType, const bool bIsClip )
{
    const bool bNarrow = ( eNType == MT_BAR_NARROW );
    const int  iWidth  = bNarrow ? 6 : 15;

    pMinStackedLayout->setContentsMargins ( 0, 0, 0, 0 );
    pGradientBar->setFixedWidth ( iWidth );
    pGradientBar->SetClip ( bIsClip );
}

void CLevelMeter::SetValue ( const double dValue )
{
    pGradientBar->SetValue ( static_cast<int> ( 100 * dValue ) );

    // clip indicator management (note that in case of clipping, i.e. full
    // scale level, the value is above NUM_STEPS_LED_BAR since the minimum
    // value of int16 is -32768 but we normalize with 32767 -> therefore
    // we really only show the clipping indicator, if actually the largest
    // value of int16 is used)
    if ( dValue >= NUM_STEPS_LED_BAR - 0.01 )
    {
        SetBarMeterStyleAndClipStatus ( eLevelMeterType, true );

        TimerClip.start();
    }
}

void CLevelMeter::ClipReset()
{
    // we manually want to reset the clipping indicator: stop timer and reset
    // clipping indicator GUI element
    TimerClip.stop();
    SetBarMeterStyleAndClipStatus ( eLevelMeterType, false );
}
