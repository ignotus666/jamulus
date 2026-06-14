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
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
#    include <QEnterEvent>
#endif
#include <cmath>
#include <algorithm>

const double PI = 3.14159265359;

namespace
{
QPoint PointOnCircle ( const int centerX, const int centerY, const double radius, const double angleRadians )
{
    return QPoint ( centerX + static_cast<int> ( radius * std::cos ( angleRadians ) ),
                    centerY + static_cast<int> ( radius * std::sin ( angleRadians ) ) );
}

SControlPalette GetCustomKnobPalette ( const bool bDarkTheme, const bool bEnabled )
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

        palette.knobNormalTop    = GetDisabledColor ( palette.knobNormalTop );
        palette.knobNormalMid    = GetDisabledColor ( palette.knobNormalMid );
        palette.knobNormalBottom = GetDisabledColor ( palette.knobNormalBottom );
        palette.knobHoverTop     = GetDisabledColor ( palette.knobHoverTop );
        palette.knobHoverMid     = GetDisabledColor ( palette.knobHoverMid );
        palette.knobHoverBottom  = GetDisabledColor ( palette.knobHoverBottom );
        palette.knobOutline      = GetDisabledColor ( palette.knobOutline );
        palette.markerNormal     = GetDisabledColor ( palette.markerNormal );
        palette.markerHover      = GetDisabledColor ( palette.markerHover );
    }
    return palette;
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
    bDarkTheme ( true ),
    bCenterArc ( false )
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
    const int knobRadius = std::max ( 5, dialRadius - 5 );

    const SControlPalette palette           = GetCustomKnobPalette ( bDarkTheme, isEnabled() );
    const QColor          bgColor           = palette.background;
    const QColor          arcColor          = palette.accent;
    const QColor          glowColor         = palette.accentGlow;
    const QColor          knobNormalTop     = palette.knobNormalTop;
    const QColor          knobNormalMid     = palette.knobNormalMid;
    const QColor          knobNormalBottom  = palette.knobNormalBottom;
    const QColor          knobHoverTop      = palette.knobHoverTop;
    const QColor          knobHoverMid      = palette.knobHoverMid;
    const QColor          knobHoverBottom   = palette.knobHoverBottom;
    const QColor          knobOutlineColor  = palette.knobOutline;
    const QColor          markerNormalColor = palette.markerNormal;
    const QColor          markerHoverColor  = palette.markerHover;

    // Fill background
    painter.fillRect ( rect(), bgColor );

    // Draw track background arc
    const QColor trackBg = bDarkTheme ? QColor ( 45, 52, 60 ) : QColor ( 210, 215, 222 );
    painter.setPen ( QPen ( trackBg, 3.0, Qt::SolidLine, Qt::RoundCap ) );
    painter.setBrush ( Qt::NoBrush );

    const int arcRadius = dialRadius - 2;
    QRect     arcRect ( centerX - arcRadius, centerY - arcRadius, arcRadius * 2, arcRadius * 2 );

    auto toQtArcAngle16 = [] ( double mathDeg ) { return static_cast<int> ( -16.0 * mathDeg ); };
    painter.drawArc ( arcRect, toQtArcAngle16 ( 135.0 ), toQtArcAngle16 ( 270.0 ) );

    // Draw active value arc (with glow underlay)
    const int range       = std::max ( 1, iMaxValue - iMinValue );
    const int centerValue = iMinValue + range / 2;

    if ( bCenterArc )
    {
        if ( iCurrentValue != centerValue )
        {
            const double centerAngle = angleFromValue ( centerValue );
            const double valueAngle  = angleFromValue ( iCurrentValue );
            const double sweepDeg    = valueAngle - centerAngle;

            painter.setPen ( QPen ( glowColor, 5.0, Qt::SolidLine, Qt::RoundCap ) );
            painter.drawArc ( arcRect, toQtArcAngle16 ( centerAngle ), toQtArcAngle16 ( sweepDeg ) );

            painter.setPen ( QPen ( arcColor, 3.0, Qt::SolidLine, Qt::RoundCap ) );
            painter.drawArc ( arcRect, toQtArcAngle16 ( centerAngle ), toQtArcAngle16 ( sweepDeg ) );
        }
    }
    else
    {
        if ( iCurrentValue != iMinValue )
        {
            const double minAngle   = angleFromValue ( iMinValue );
            const double valueAngle = angleFromValue ( iCurrentValue );
            const double sweepDeg   = valueAngle - minAngle;

            painter.setPen ( QPen ( glowColor, 5.0, Qt::SolidLine, Qt::RoundCap ) );
            painter.drawArc ( arcRect, toQtArcAngle16 ( minAngle ), toQtArcAngle16 ( sweepDeg ) );

            painter.setPen ( QPen ( arcColor, 3.0, Qt::SolidLine, Qt::RoundCap ) );
            painter.drawArc ( arcRect, toQtArcAngle16 ( minAngle ), toQtArcAngle16 ( sweepDeg ) );
        }
    }

    // Draw center knob cap
    QRect        knobRect ( centerX - knobRadius, centerY - knobRadius, knobRadius * 2, knobRadius * 2 );
    const bool   bHighlight = bKnobHovered || bMousePressed;
    const QColor capTop     = bHighlight ? knobHoverTop : knobNormalTop;
    const QColor capMid     = bHighlight ? knobHoverMid : knobNormalMid;
    const QColor capBottom  = bHighlight ? knobHoverBottom : knobNormalBottom;

    QLinearGradient capGrad ( knobRect.topLeft(), knobRect.bottomRight() );
    capGrad.setColorAt ( 0.0, capTop );
    capGrad.setColorAt ( 0.5, capMid );
    capGrad.setColorAt ( 1.0, capBottom );

    painter.setBrush ( capGrad );
    painter.setPen ( QPen ( bHighlight ? arcColor : knobOutlineColor, 1.2 ) );
    painter.drawEllipse ( knobRect );

    // Draw inner subtle highlight ring on the knob cap
    painter.setPen ( QPen ( QColor ( 255, 255, 255, bHighlight ? 30 : 15 ), 1.0 ) );
    painter.setBrush ( Qt::NoBrush );
    painter.drawEllipse ( knobRect.adjusted ( 1, 1, -1, -1 ) );

    // Draw notch pointer on the knob cap itself
    double       angle   = angleFromValue ( iCurrentValue );
    double       radians = angle * PI / 180.0;
    const QPoint p1      = PointOnCircle ( centerX, centerY, knobRadius - 5, radians );
    const QPoint p2      = PointOnCircle ( centerX, centerY, knobRadius - 1, radians );

    painter.setPen ( QPen ( bHighlight ? markerHoverColor : markerNormalColor, 2.0, Qt::SolidLine, Qt::RoundCap ) );
    painter.drawLine ( p1, p2 );
}

#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
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
