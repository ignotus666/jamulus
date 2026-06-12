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
#include <QSlider>

/**
 * Custom slider. Provides a drop-in replacement for QSlider with custom painting.
 * Emits the core change notifications and supports mouse interactions.
 */
class CCustomSlider : public QWidget
{
    Q_OBJECT

public:
    explicit CCustomSlider ( QWidget* parent = nullptr );
    explicit CCustomSlider ( Qt::Orientation orientation, QWidget* parent = nullptr );
    virtual ~CCustomSlider();

    // Value management (QSlider compatible)
    int  value() const { return iCurrentValue; }
    void setValue ( int val );
    void setRange ( int minVal, int maxVal );
    void setMinimum ( int min );
    void setMaximum ( int max );
    int  minimum() const { return iMinValue; }
    int  maximum() const { return iMaxValue; }
    void setPageStep ( int step ) { iPageStep = step; }
    void setTickInterval ( int interval ) { iTickInterval = interval; }
    void setTickPosition ( QSlider::TickPosition position ) { eTickPosition = position; }
    void setOrientation ( Qt::Orientation orientation )
    {
        eOrientation = orientation;
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
    void SetDarkTheme ( bool bEnable );
    void SetCompactMode ( bool bEnable );
    void SetCenterSweep ( bool bEnable )
    {
        if ( bCenterSweep != bEnable )
        {
            bCenterSweep = bEnable;
            update();
        }
    }
    bool GetCenterSweep() const { return bCenterSweep; }

    // Sizing
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void valueChanged ( int value );

protected:
    void paintEvent ( QPaintEvent* event ) override;
    void mousePressEvent ( QMouseEvent* event ) override;
    void mouseMoveEvent ( QMouseEvent* event ) override;
    void mouseReleaseEvent ( QMouseEvent* event ) override;
    void leaveEvent ( QEvent* event ) override;
    void wheelEvent ( QWheelEvent* event ) override;

private:
    QRect currentHandleRect() const;
    int   valueFromPosition ( int pos ) const;
    int   positionFromValue ( int val ) const;
    void  updateValue ( int pos );
    void  drawVerticalSlider ( QPainter& painter );
    void  drawHorizontalSlider ( QPainter& painter );

    // State
    int                   iMinValue;
    int                   iMaxValue;
    int                   iCurrentValue;
    int                   iPageStep;
    int                   iTickInterval;
    bool                  bMousePressed;
    bool                  bHandleHovered;
    bool                  bDarkTheme;
    bool                  bCompact;
    bool                  bCenterSweep;
    Qt::Orientation       eOrientation;
    QSlider::TickPosition eTickPosition;

    // Constants
    static const int MARGINS       = 10;
};
