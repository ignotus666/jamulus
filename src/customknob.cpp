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

#include "customknob.h"
#include "uicolors.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#if QT_VERSION >= QT_VERSION_CHECK ( 6, 0, 0 )
#include <QEnterEvent>
#endif
#include <cmath>
#include <algorithm>

const double PI = 3.14159265359;

namespace
{
void DrawKnobCap ( QPainter&     painter,
                   const QRect&  knobRect,
                   const QRect&  innerRect,
                   const QColor& knobTop,
                   const QColor& knobMid,
                   const QColor& knobBottom,
                   const QColor& outlineColor,
                   const QColor& bevelColor )
{
    QLinearGradient gradient ( knobRect.topLeft(), knobRect.bottomLeft() );
    gradient.setColorAt ( 0.0, knobTop );
    gradient.setColorAt ( 0.5, knobMid );
    gradient.setColorAt ( 1.0, knobBottom );
    painter.setBrush ( gradient );
    painter.setPen ( QPen ( outlineColor, 1.2 ) );
    painter.drawEllipse ( knobRect );

    painter.setPen ( QPen ( bevelColor, 1 ) );
    painter.setBrush ( Qt::NoBrush );
    painter.drawEllipse ( innerRect );
}

QPoint PointOnCircle ( const int centerX, const int centerY, const double radius, const double angleRadians )
{
    return QPoint ( centerX + static_cast<int> ( radius * std::cos ( angleRadians ) ),
                    centerY + static_cast<int> ( radius * std::sin ( angleRadians ) ) );
}
} // namespace

CCustomKnob::CCustomKnob ( QWidget* parent ) :
    QWidget ( parent ),
    iMinValue ( 0 ),
    iMaxValue ( 100 ),
    iCurrentValue ( 50 ),
    iPageStep ( 1 ),
    iDragStartValue ( 50 ),
    iDragStartY ( 0 ),
    bMousePressed ( false ),
    bKnobHovered ( false ),
    bDarkTheme ( true )
{
    setFocusPolicy ( Qt::StrongFocus );
    setAttribute ( Qt::WA_OpaquePaintEvent );
    setMouseTracking ( true );
}

CCustomKnob::~CCustomKnob() {}

void CCustomKnob::setValue ( int val )
{
    int newVal = std::max ( iMinValue, std::min ( iMaxValue, val ) );
    if ( newVal != iCurrentValue )
    {
        iCurrentValue = newVal;
        update();
        emit valueChanged ( iCurrentValue );
    }
}

void CCustomKnob::setRange ( int minVal, int maxVal )
{
    iMinValue     = minVal;
    iMaxValue     = maxVal;
    iCurrentValue = std::max ( iMinValue, std::min ( iMaxValue, iCurrentValue ) );
    update();
}

double CCustomKnob::angleFromValue ( int val ) const
{
    int range = iMaxValue - iMinValue;
    if ( range == 0 )
        return 0;

    // Map value to angle: 135 to 405 degrees (270 degree sweep, centered)
    // Min value at bottom-left, max value at bottom-right
    double ratio = static_cast<double> ( val - iMinValue ) / range;
    double angle = 135.0 + ( ratio * 270.0 );
    return angle;
}

int CCustomKnob::valueFromAngle ( double angle ) const
{
    // Normalize angle to 0-360 range
    while ( angle < 0 )
        angle += 360;
    while ( angle >= 360 )
        angle -= 360;

    // Map angle 135-405 to value range
    // Treat angles that wrap around (e.g., 350+ as coming from 135 direction)
    if ( angle < 135 )
        angle += 360;

    double ratio = ( angle - 135.0 ) / 270.0;
    ratio        = std::max ( 0.0, std::min ( 1.0, ratio ) );

    int range = iMaxValue - iMinValue;
    return iMinValue + static_cast<int> ( ratio * range );
}

void CCustomKnob::updateValue ( QMouseEvent* event )
{
    int centerX = width() / 2;
    int centerY = height() / 2;

    int dx = event->x() - centerX;
    int dy = event->y() - centerY;

    double angle = std::atan2 ( dy, dx ) * 180.0 / PI;
    angle += 90; // Offset so 0 degrees points up

    int newVal = valueFromAngle ( angle );
    if ( newVal != iCurrentValue )
    {
        iCurrentValue = newVal;
        update();
        emit valueChanged ( iCurrentValue );
    }
}

void CCustomKnob::paintEvent ( QPaintEvent* event )
{
    Q_UNUSED ( event );

    QPainter painter ( this );
    painter.setRenderHint ( QPainter::Antialiasing );

    int       w          = width();
    int       h          = height();
    int       centerX    = w / 2;
    int       centerY    = h / 2;
    const int dialRadius = std::max ( 8, ( std::min ( w, h ) / 2 ) - 2 );
    const int knobRadius = std::max ( 5, dialRadius - 6 );

    const SControlPalette palette             = GetControlPalette ( bDarkTheme );
    const QColor          bgColor             = palette.background;
    const QColor          dialColor           = palette.dial;
    const QColor          dialOutlineColor    = palette.dialOutline;
    const QColor          ringColor           = palette.ring;
    const QColor          arcColor            = palette.accent;
    const QColor          glowColor           = palette.accentGlow;
    const QColor          knobNormalTop       = palette.knobNormalTop;
    const QColor          knobNormalMid       = palette.knobNormalMid;
    const QColor          knobNormalBottom    = palette.knobNormalBottom;
    const QColor          knobHoverTop        = palette.knobHoverTop;
    const QColor          knobHoverMid        = palette.knobHoverMid;
    const QColor          knobHoverBottom     = palette.knobHoverBottom;
    const QColor          knobOutlineColor    = palette.knobOutline;
    const QColor          markerNormalColor   = palette.markerNormal;
    const QColor          markerHoverColor    = palette.markerHover;
    const QColor          markerDotColor      = palette.markerDot;
    const QColor          markerDotHoverColor = palette.markerDotHover;
    const QColor          innerBevelColor     = palette.innerBevel;

    // Fill background
    painter.fillRect ( rect(), bgColor );

    // Draw outer dial plate
    painter.setPen ( QPen ( dialOutlineColor, 1 ) );
    painter.setBrush ( dialColor );
    painter.drawEllipse ( centerX - dialRadius, centerY - dialRadius, dialRadius * 2, dialRadius * 2 );

    // Subtle ring base, always visible
    painter.setPen ( QPen ( ringColor, 1.2 ) );
    painter.setBrush ( Qt::NoBrush );
    painter.drawEllipse ( centerX - ( dialRadius - 2 ), centerY - ( dialRadius - 2 ), ( dialRadius - 2 ) * 2, ( dialRadius - 2 ) * 2 );

    // No notch marks for this control.

    // Draw center-out pan arc: hidden at center, grows to left or right
    const int arcRadius = std::max ( 6, dialRadius - 3 );
    QRect     arcRect ( centerX - arcRadius, centerY - arcRadius, arcRadius * 2, arcRadius * 2 );
    const int range       = std::max ( 1, iMaxValue - iMinValue );
    const int centerValue = iMinValue + range / 2;

    auto toQtArcAngle16 = [] ( double mathDeg ) { return static_cast<int> ( -16.0 * mathDeg ); };

    if ( iCurrentValue != centerValue )
    {
        const double centerAngle = angleFromValue ( centerValue );
        const double valueAngle  = angleFromValue ( iCurrentValue );
        const double sweepDeg    = valueAngle - centerAngle;

        painter.setPen ( QPen ( arcColor, 2.4 ) );
        painter.setBrush ( Qt::NoBrush );
        painter.drawArc ( arcRect, toQtArcAngle16 ( centerAngle ), toQtArcAngle16 ( sweepDeg ) );
    }

    // Draw indicator line from center
    double       angle     = angleFromValue ( iCurrentValue );
    double       radians   = angle * PI / 180.0;
    const QPoint linePoint = PointOnCircle ( centerX, centerY, knobRadius - 2, radians );

    // Cyan indicator glow
    painter.setPen ( QPen ( glowColor, 4 ) );
    painter.drawLine ( centerX, centerY, linePoint.x(), linePoint.y() );

    // Indicator line (cyan)
    painter.setPen ( QPen ( arcColor, 2 ) );
    painter.drawLine ( centerX, centerY, linePoint.x(), linePoint.y() );

    // Draw center knob (dark cap)
    QRect knobRect ( centerX - knobRadius, centerY - knobRadius, knobRadius * 2, knobRadius * 2 );

    const bool   bHighlight  = bKnobHovered || bMousePressed;
    const QColor capTop      = bHighlight ? knobHoverTop : knobNormalTop;
    const QColor capMid      = bHighlight ? knobHoverMid : knobNormalMid;
    const QColor capBottom   = bHighlight ? knobHoverBottom : knobNormalBottom;
    const QColor capOutline  = bHighlight ? arcColor : knobOutlineColor;
    const int    innerRadius = std::max ( 2, knobRadius - 4 );
    QRect        innerRect ( centerX - innerRadius, centerY - innerRadius, innerRadius * 2, innerRadius * 2 );
    DrawKnobCap ( painter, knobRect, innerRect, capTop, capMid, capBottom, capOutline, innerBevelColor );

    // Rotating cap marker so the knob body itself appears to rotate
    const int    markerOuterRadius = std::max ( 3, knobRadius - 2 );
    const int    markerInnerRadius = std::max ( 2, knobRadius - 8 );
    const QPoint markerInnerPoint  = PointOnCircle ( centerX, centerY, markerInnerRadius, radians );
    const QPoint markerOuterPoint  = PointOnCircle ( centerX, centerY, markerOuterRadius, radians );

    painter.setPen ( QPen ( ( bKnobHovered || bMousePressed ) ? markerHoverColor : markerNormalColor, 2 ) );
    painter.drawLine ( markerInnerPoint, markerOuterPoint );

    // Marker tip dot
    const int    dotRadius = std::max ( 2, knobRadius / 7 );
    const QPoint dotPoint  = PointOnCircle ( centerX, centerY, knobRadius - 1, radians );
    const int    dotX      = dotPoint.x() - dotRadius;
    const int    dotY      = dotPoint.y() - dotRadius;
    painter.setPen ( Qt::NoPen );
    painter.setBrush ( ( bKnobHovered || bMousePressed ) ? markerDotHoverColor : markerDotColor );
    painter.drawEllipse ( dotX, dotY, dotRadius * 2, dotRadius * 2 );
}

#if QT_VERSION >= QT_VERSION_CHECK ( 6, 0, 0 )
void CCustomKnob::enterEvent ( QEnterEvent* event )
#else
void CCustomKnob::enterEvent ( QEvent* event )
#endif
{
    QWidget::enterEvent ( event );
    bKnobHovered = true;
    update();
}

void CCustomKnob::leaveEvent ( QEvent* event )
{
    QWidget::leaveEvent ( event );
    bKnobHovered = false;
    update();
}

void CCustomKnob::mousePressEvent ( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton )
    {
        bMousePressed   = true;
        bKnobHovered    = true;
        iDragStartValue = iCurrentValue;
        iDragStartY     = event->y();
        emit sliderPressed();
        setCursor ( Qt::SizeVerCursor );
        updateValue ( event );
        event->accept();
    }
}

void CCustomKnob::mouseMoveEvent ( QMouseEvent* event )
{
    bKnobHovered = rect().contains ( event->pos() );

    if ( bMousePressed )
    {
        const int range       = std::max ( 1, iMaxValue - iMinValue );
        const int deltaPixels = iDragStartY - event->y();
        const int newVal      = iDragStartValue + ( deltaPixels * range ) / 60;

        if ( newVal != iCurrentValue )
        {
            iCurrentValue = std::max ( iMinValue, std::min ( iMaxValue, newVal ) );
            update();
            emit valueChanged ( iCurrentValue );
        }

        event->accept();
    }
}

void CCustomKnob::mouseReleaseEvent ( QMouseEvent* event )
{
    if ( bMousePressed )
    {
        unsetCursor();
        emit sliderReleased();
    }

    bMousePressed = false;
    bKnobHovered  = rect().contains ( event->pos() );
    update();
}

void CCustomKnob::wheelEvent ( QWheelEvent* event )
{
    int delta  = event->angleDelta().y();
    int newVal = iCurrentValue + ( ( delta > 0 ) ? iPageStep : -iPageStep );
    setValue ( newVal );
    event->accept();
}
