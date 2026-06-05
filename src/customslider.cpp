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

#include "customslider.h"
#include "uicolors.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

namespace
{
constexpr int TICK_SPACING_PX = 20;

void FillAccentGradient ( QPainter& painter, const QRect& rect, const bool bVertical, const SControlPalette& palette )
{
    if ( rect.width() <= 0 || rect.height() <= 0 )
    {
        return;
    }

    QLinearGradient gradient ( rect.topLeft(), bVertical ? rect.bottomLeft() : rect.topRight() );
    gradient.setColorAt ( 0, palette.accentMid );
    gradient.setColorAt ( 0.55, palette.accent );
    gradient.setColorAt ( 1, palette.accentDeep );
    painter.fillRect ( rect, gradient );
}

void FillAccentStripe ( QPainter& painter, const QRect& rect, const bool bVertical, const SControlPalette& palette )
{
    if ( rect.width() <= 0 || rect.height() <= 0 )
    {
        return;
    }

    QLinearGradient gradient ( rect.topLeft(), bVertical ? rect.bottomLeft() : rect.topRight() );
    gradient.setColorAt ( 0, palette.accentBright );
    gradient.setColorAt ( 0.5, palette.accentMid );
    gradient.setColorAt ( 1, palette.accent );
    painter.fillRect ( rect, gradient );
}

void DrawHandle ( QPainter& painter, const QRect& rect, const bool bVertical, const bool bHighlighted, const SControlPalette& palette )
{
    QLinearGradient gradient ( rect.topLeft(), bVertical ? rect.bottomLeft() : rect.topRight() );
    gradient.setColorAt ( 0, palette.handleTop );
    gradient.setColorAt ( 0.55, palette.handleMid );
    gradient.setColorAt ( 1, palette.handleBottom );

    QPainterPath handlePath;
    handlePath.addRoundedRect ( rect.adjusted ( 0, 0, -1, -1 ), 8, 8 );
    painter.setPen ( Qt::NoPen );
    painter.setBrush ( gradient );
    painter.drawPath ( handlePath );

    const QColor borderColor = bHighlighted ? palette.accent : palette.handleBorder;
    const qreal  borderWidth = bHighlighted ? 1.6 : 1.2;
    painter.setPen ( QPen ( borderColor, borderWidth ) );
    painter.setBrush ( Qt::NoBrush );
    painter.drawPath ( handlePath );
}

void DrawTickMarks ( QPainter&                   painter,
                     const int                   startPos,
                     const int                   endPos,
                     const bool                  bVertical,
                     const int                   trackLeft,
                     const int                   trackTop,
                     const int                   trackWidth,
                     const int                   trackHeight,
                     const QSlider::TickPosition eTickPosition,
                     const QColor&               tickColor,
                     const bool                  bValueBased,
                     const int                   minValue,
                     const int                   maxValue )
{
    painter.setPen ( QPen ( tickColor, 1 ) );

    if ( bValueBased )
    {
        const int valueRange = std::max ( 1, maxValue - minValue );

        for ( int val = minValue; val <= maxValue; ++val )
        {
            const qreal fraction = static_cast<qreal> ( val - minValue ) / valueRange;
            const int   pos      = qRound ( startPos + ( endPos - startPos ) * fraction );

            if ( bVertical )
            {
                const bool drawLeft = ( eTickPosition == QSlider::TicksBothSides ) || ( eTickPosition == QSlider::TicksLeft ) ||
                                      ( eTickPosition == QSlider::TicksAbove );
                const bool drawRight = ( eTickPosition == QSlider::TicksBothSides ) || ( eTickPosition == QSlider::TicksRight ) ||
                                       ( eTickPosition == QSlider::TicksBelow );

                if ( drawLeft )
                {
                    painter.drawLine ( trackLeft - 7, pos, trackLeft - 2, pos );
                }
                if ( drawRight )
                {
                    painter.drawLine ( trackLeft + trackWidth + 2, pos, trackLeft + trackWidth + 7, pos );
                }
            }
            else
            {
                painter.drawLine ( pos, trackTop - 7, pos, trackTop - 2 );
                painter.drawLine ( pos, trackTop + trackHeight + 2, pos, trackTop + trackHeight + 7 );
            }
        }
        return;
    }

    const int trackLength = std::abs ( endPos - startPos );
    if ( trackLength <= 0 )
    {
        return;
    }

    const int intervalCount = std::max ( 1, trackLength / TICK_SPACING_PX );

    for ( int i = 0; i <= intervalCount; ++i )
    {
        const qreal fraction = static_cast<qreal> ( i ) / intervalCount;
        const int   pos      = qRound ( startPos + ( endPos - startPos ) * fraction );

        if ( bVertical )
        {
            const bool drawLeft =
                ( eTickPosition == QSlider::TicksBothSides ) || ( eTickPosition == QSlider::TicksLeft ) || ( eTickPosition == QSlider::TicksAbove );
            const bool drawRight =
                ( eTickPosition == QSlider::TicksBothSides ) || ( eTickPosition == QSlider::TicksRight ) || ( eTickPosition == QSlider::TicksBelow );

            if ( drawLeft )
            {
                painter.drawLine ( trackLeft - 7, pos, trackLeft - 2, pos );
            }
            if ( drawRight )
            {
                painter.drawLine ( trackLeft + trackWidth + 2, pos, trackLeft + trackWidth + 7, pos );
            }
        }
        else
        {
            painter.drawLine ( pos, trackTop - 7, pos, trackTop - 2 );
            painter.drawLine ( pos, trackTop + trackHeight + 2, pos, trackTop + trackHeight + 7 );
        }
    }
}

SControlPalette GetCustomSliderPalette ( const bool bDarkTheme, const bool bEnabled )
{
    SControlPalette palette = GetControlPalette ( bDarkTheme );
    if ( !bEnabled )
    {
        auto GetDisabledColor = [bDarkTheme] ( const QColor& color ) -> QColor {
            int r     = color.red();
            int g     = color.green();
            int b     = color.blue();
            int alpha = color.alpha();
            int gray  = qRound ( 0.299 * r + 0.587 * g + 0.114 * b );
            if ( bDarkTheme )
            {
                return QColor ( ( gray + 60 ) / 2, ( gray + 60 ) / 2, ( gray + 60 ) / 2, alpha );
            }
            else
            {
                return QColor ( ( gray + 200 ) / 2, ( gray + 200 ) / 2, ( gray + 200 ) / 2, alpha );
            }
        };

        palette.accent       = GetDisabledColor ( palette.accent );
        palette.accentGlow   = GetDisabledColor ( palette.accentGlow );
        palette.accentBright = GetDisabledColor ( palette.accentBright );
        palette.accentMid    = GetDisabledColor ( palette.accentMid );
        palette.accentDeep   = GetDisabledColor ( palette.accentDeep );

        palette.trackBackground = GetDisabledColor ( palette.trackBackground );
        palette.trackBorder     = GetDisabledColor ( palette.trackBorder );
        palette.handleTop       = GetDisabledColor ( palette.handleTop );
        palette.handleMid       = GetDisabledColor ( palette.handleMid );
        palette.handleBottom    = GetDisabledColor ( palette.handleBottom );
        palette.handleBorder    = GetDisabledColor ( palette.handleBorder );

        palette.tick = GetDisabledColor ( palette.tick );
    }
    return palette;
}
} // namespace

CCustomSlider::CCustomSlider ( QWidget* parent ) : CCustomSlider ( Qt::Vertical, parent ) {}

CCustomSlider::CCustomSlider ( Qt::Orientation orientation, QWidget* parent ) :
    QWidget ( parent ),
    iMinValue ( 0 ),
    iMaxValue ( 100 ),
    iCurrentValue ( 0 ),
    iPageStep ( 10 ),
    iTickInterval ( 0 ),
    bMousePressed ( false ),
    bHandleHovered ( false ),
    bDarkTheme ( true ),
    bCompact ( false ),
    eOrientation ( orientation ),
    eTickPosition ( QSlider::TicksAbove )
{
    if ( eOrientation == Qt::Vertical )
    {
        setSizePolicy ( QSizePolicy::Preferred, QSizePolicy::Expanding );
    }
    else
    {
        setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Preferred );
    }

    setFocusPolicy ( Qt::StrongFocus );
    setAttribute ( Qt::WA_OpaquePaintEvent );
    setMouseTracking ( true );
    if ( eOrientation == Qt::Vertical )
    {
        setSizePolicy ( QSizePolicy::Preferred, QSizePolicy::Expanding );
    }
    else
    {
        setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Preferred );
    }
    updateGeometry();
    update();
}

CCustomSlider::~CCustomSlider() {}

void CCustomSlider::SetDarkTheme ( bool bEnable )
{
    if ( bDarkTheme != bEnable )
    {
        bDarkTheme = bEnable;
        repaint();
    }
}

void CCustomSlider::SetCompactMode ( bool bEnable )
{
    if ( bCompact != bEnable )
    {
        bCompact = bEnable;
        updateGeometry();
        update();
    }
}

void CCustomSlider::setValue ( int val )
{
    int newVal = std::max ( iMinValue, std::min ( iMaxValue, val ) );
    if ( newVal != iCurrentValue )
    {
        iCurrentValue = newVal;
        update();
        emit valueChanged ( iCurrentValue );
    }
}

void CCustomSlider::setRange ( int minVal, int maxVal )
{
    iMinValue     = minVal;
    iMaxValue     = maxVal;
    iCurrentValue = std::max ( iMinValue, std::min ( iMaxValue, iCurrentValue ) );
    update();
}

void CCustomSlider::setMinimum ( int min ) { setRange ( min, iMaxValue ); }

void CCustomSlider::setMaximum ( int max ) { setRange ( iMinValue, max ); }

QSize CCustomSlider::sizeHint() const
{
    if ( eOrientation == Qt::Vertical )
        return bCompact ? QSize ( 36, 200 ) : QSize ( 50, 200 );
    else
        return bCompact ? QSize ( 200, 36 ) : QSize ( 200, 50 );
}

QSize CCustomSlider::minimumSizeHint() const
{
    if ( eOrientation == Qt::Vertical )
        return bCompact ? QSize ( 28, 100 ) : QSize ( 40, 100 );
    else
        return bCompact ? QSize ( 100, 28 ) : QSize ( 100, 40 );
}

int CCustomSlider::valueFromPosition ( int pos ) const
{
    int range = iMaxValue - iMinValue;
    if ( range == 0 )
        return iMinValue;

    int trackSize = 0;
    if ( eOrientation == Qt::Vertical )
    {
        // Keep vertical track insets symmetric and aligned with the meter widget.
        trackSize = height() - 2 * MARGINS;
        pos       = height() - MARGINS - pos; // inverted for vertical
    }
    else
    {
        trackSize = width() - 2 * MARGINS - HANDLE_WIDTH;
        pos       = pos - MARGINS - HANDLE_WIDTH / 2;
    }

    if ( trackSize <= 0 )
        return iMinValue;

    int value = iMinValue + ( pos * range ) / trackSize;
    return std::max ( iMinValue, std::min ( iMaxValue, value ) );
}

int CCustomSlider::positionFromValue ( int val ) const
{
    int range = iMaxValue - iMinValue;
    if ( range == 0 )
        range = 1;

    int trackSize = 0;
    int basePos   = 0;

    if ( eOrientation == Qt::Vertical )
    {
        trackSize = height() - 2 * MARGINS;
        basePos   = MARGINS;
        int pos   = basePos + ( ( iMaxValue - val ) * trackSize ) / range;
        return pos;
    }
    else
    {
        trackSize = width() - 2 * MARGINS - HANDLE_WIDTH;
        basePos   = MARGINS + HANDLE_WIDTH / 2;
        int pos   = basePos + ( ( val - iMinValue ) * trackSize ) / range;
        return pos;
    }
}

QRect CCustomSlider::currentHandleRect() const
{
    const int handlePos = positionFromValue ( iCurrentValue );

    if ( eOrientation == Qt::Vertical )
    {
        const int handleLeft = ( width() - HANDLE_WIDTH ) / 2;
        return QRect ( handleLeft, handlePos - HANDLE_HEIGHT / 2, HANDLE_WIDTH, HANDLE_HEIGHT );
    }

    const int handleTop = ( height() - HANDLE_HEIGHT ) / 2;
    return QRect ( handlePos - HANDLE_WIDTH / 2, handleTop, HANDLE_WIDTH, HANDLE_HEIGHT );
}

void CCustomSlider::updateValue ( int pos )
{
    int newVal = valueFromPosition ( pos );
    if ( newVal != iCurrentValue )
    {
        iCurrentValue = newVal;
        update();
        emit valueChanged ( iCurrentValue );
    }
}

void CCustomSlider::paintEvent ( QPaintEvent* event )
{
    Q_UNUSED ( event );

    QPainter painter ( this );
    painter.setRenderHint ( QPainter::Antialiasing );

    const SControlPalette palette         = GetCustomSliderPalette ( bDarkTheme, isEnabled() );
    const QColor          backgroundColor = palette.background;

    painter.fillRect ( rect(), backgroundColor );

    if ( eOrientation == Qt::Vertical )
        drawVerticalSlider ( painter );
    else
        drawHorizontalSlider ( painter );
}

void CCustomSlider::drawVerticalSlider ( QPainter& painter )
{
    int                   width           = this->width();
    int                   height          = this->height();
    int                   trackSize       = height - 2 * MARGINS;
    int                   trackLeft       = ( width - TRACK_WIDTH ) / 2;
    int                   trackTop        = MARGINS;
    int                   handlePos       = positionFromValue ( iCurrentValue );
    const SControlPalette palette         = GetCustomSliderPalette ( bDarkTheme, isEnabled() );
    const QColor          trackBackground = palette.trackBackground;
    const QColor          trackBorder     = palette.trackBorder;

    // Draw track background
    painter.fillRect ( trackLeft, trackTop, TRACK_WIDTH, trackSize, trackBackground );

    // Draw filled portion with a brighter cyan-to-blue gradient
    int filledHeight = height - MARGINS - handlePos;
    if ( filledHeight > 0 )
    {
        const QRect filledRect ( trackLeft, handlePos, TRACK_WIDTH, filledHeight );
        FillAccentGradient ( painter, filledRect, true, palette );

        // Thin bright center accent to mirror the reverb slider's lighter active stripe
        const int   accentWidth = 2;
        const int   accentLeft  = trackLeft + ( TRACK_WIDTH - accentWidth ) / 2;
        const QRect accentRect ( accentLeft, handlePos, accentWidth, filledHeight );
        FillAccentStripe ( painter, accentRect, true, palette );
    }

    // Draw track border
    painter.setPen ( QPen ( trackBorder, 1 ) );
    painter.drawRect ( trackLeft, trackTop, TRACK_WIDTH, trackSize );

    // Draw tick marks with a consistent pixel spacing across all sliders.
    if ( iTickInterval > 0 && eTickPosition != QSlider::NoTicks )
    {
        DrawTickMarks ( painter,
                        positionFromValue ( iMinValue ),
                        positionFromValue ( iMaxValue ),
                        true,
                        trackLeft,
                        trackTop,
                        TRACK_WIDTH,
                        trackSize,
                        eTickPosition,
                        palette.tick,
                        ( iTickInterval == 1 ),
                        iMinValue,
                        iMaxValue );
    }

    DrawHandle ( painter, currentHandleRect(), true, ( bHandleHovered || bMousePressed ), palette );
}

void CCustomSlider::drawHorizontalSlider ( QPainter& painter )
{
    int                   width           = this->width();
    int                   height          = this->height();
    int                   trackSize       = width - 2 * MARGINS - HANDLE_WIDTH;
    int                   trackTop        = ( height - TRACK_WIDTH ) / 2;
    int                   handlePos       = positionFromValue ( iCurrentValue );
    const SControlPalette palette         = GetCustomSliderPalette ( bDarkTheme, isEnabled() );
    const QColor          trackBackground = palette.trackBackground;
    const QColor          trackBorder     = palette.trackBorder;

    // Draw track background
    painter.fillRect ( MARGINS + HANDLE_WIDTH / 2, trackTop, trackSize, TRACK_WIDTH, trackBackground );

    // Draw filled portion with a brighter cyan-to-blue gradient
    int filledWidth = handlePos - MARGINS - HANDLE_WIDTH / 2;
    if ( filledWidth > 0 )
    {
        const QRect filledRect ( MARGINS + HANDLE_WIDTH / 2, trackTop, filledWidth, TRACK_WIDTH );
        FillAccentGradient ( painter, filledRect, false, palette );

        // Thin bright center accent to mirror the reverb slider's lighter active stripe
        const int   accentHeight = 2;
        const int   accentTop    = trackTop + ( TRACK_WIDTH - accentHeight ) / 2;
        const QRect accentRect ( MARGINS + HANDLE_WIDTH / 2, accentTop, filledWidth, accentHeight );
        FillAccentStripe ( painter, accentRect, false, palette );
    }

    // Draw track border
    painter.setPen ( QPen ( trackBorder, 1 ) );
    painter.drawRect ( MARGINS + HANDLE_WIDTH / 2, trackTop, trackSize, TRACK_WIDTH );

    // Draw tick marks with a consistent pixel spacing across all sliders.
    if ( iTickInterval > 0 && eTickPosition != QSlider::NoTicks )
    {
        DrawTickMarks ( painter,
                        positionFromValue ( iMinValue ),
                        positionFromValue ( iMaxValue ),
                        false,
                        MARGINS + HANDLE_WIDTH / 2,
                        trackTop,
                        TRACK_WIDTH,
                        TRACK_WIDTH,
                        eTickPosition,
                        palette.tick,
                        ( iTickInterval == 1 ),
                        iMinValue,
                        iMaxValue );
    }

    DrawHandle ( painter, currentHandleRect(), false, ( bHandleHovered || bMousePressed ), palette );
}

void CCustomSlider::mousePressEvent ( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton )
    {
        bMousePressed  = true;
        bHandleHovered = currentHandleRect().contains ( event->pos() );
        updateValue ( eOrientation == Qt::Vertical ? event->y() : event->x() );
        update();
    }
}

void CCustomSlider::mouseMoveEvent ( QMouseEvent* event )
{
    const bool bWasHandleHovered = bHandleHovered;
    bHandleHovered               = currentHandleRect().contains ( event->pos() );

    if ( bMousePressed )
    {
        updateValue ( eOrientation == Qt::Vertical ? event->y() : event->x() );
    }
    else if ( bWasHandleHovered != bHandleHovered )
    {
        update();
    }
}

void CCustomSlider::mouseReleaseEvent ( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton )
    {
        bMousePressed  = false;
        bHandleHovered = currentHandleRect().contains ( event->pos() );
        update();
    }
}

void CCustomSlider::leaveEvent ( QEvent* event )
{
    QWidget::leaveEvent ( event );

    if ( bHandleHovered )
    {
        bHandleHovered = false;
        update();
    }
}

void CCustomSlider::wheelEvent ( QWheelEvent* event )
{
    int delta    = event->angleDelta().y();
    int numSteps = delta / 120; // typically 120 per wheel click
    int newVal   = iCurrentValue + numSteps * std::max ( 1, iPageStep / 10 );
    setValue ( newVal );
    event->accept();
}
