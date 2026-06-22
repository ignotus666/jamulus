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

#pragma once

#include <QWidget>

/**
 * @brief Modern custom pan knob with circular dial design.
 *
 * This widget provides a professional circular dial control for pan (left-right)
 * or other continuous parameters. It features a dark background with bright cyan
 * accents matching the custom slider aesthetic.
 */
class CCustomKnob : public QWidget
{
    Q_OBJECT

public:
    explicit CCustomKnob ( QWidget* parent = nullptr );
    virtual ~CCustomKnob() = default;

    // Value management (QDial compatible)
    int  value() const { return iCurrentValue; }
    void setValue ( int val );
    void setRange ( int minVal, int maxVal );
    int  minimum() const { return iMinValue; }
    int  maximum() const { return iMaxValue; }
    void setPageStep ( int step ) { iPageStep = step; }
    void SetDarkTheme ( bool bEnable )
    {
        if ( bDarkTheme != bEnable )
        {
            bDarkTheme = bEnable;
            update();
        }
    }
    void SetCenterArc ( bool bEnable )
    {
        if ( bCenterArc != bEnable )
        {
            bCenterArc = bEnable;
            update();
        }
    }
    void SetAccentColor ( const QColor& color )
    {
        if ( colAccentOverride != color )
        {
            colAccentOverride = color;
            update();
        }
    }
    void ClearAccentColor()
    {
        if ( colAccentOverride.isValid() )
        {
            colAccentOverride = QColor(); // invalid = use default
            update();
        }
    }

    void setMinimum ( int min ) { setRange ( min, iMaxValue ); }
    void setMaximum ( int max ) { setRange ( iMinValue, max ); }

    // Sizing
    QSize sizeHint() const override { return QSize ( 50, 50 ); }
    QSize minimumSizeHint() const override { return QSize ( 36, 36 ); }

signals:
    void valueChanged ( int value );
    void sliderPressed();
    void sliderReleased();

protected:
    void paintEvent ( QPaintEvent* event ) override;
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    void enterEvent ( QEnterEvent* event ) override;
#else
    void enterEvent ( QEvent* event ) override;
#endif
    void leaveEvent ( QEvent* event ) override;
    void mousePressEvent ( QMouseEvent* event ) override;
    void mouseMoveEvent ( QMouseEvent* event ) override;
    void mouseReleaseEvent ( QMouseEvent* event ) override;
    void wheelEvent ( QWheelEvent* event ) override;

private:
    double angleFromValue ( int val ) const;

    // State
    int    iMinValue;
    int    iMaxValue;
    int    iCurrentValue;
    int    iPageStep;
    int    iDragStartValue;
    int    iDragStartY;
    bool   bMousePressed;
    bool   bKnobHovered;
    bool   bDarkTheme;
    bool   bCenterArc;
    QColor colAccentOverride; // If valid, overrides the palette accent for arc/glow
};
