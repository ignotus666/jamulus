/******************************************************************************\
 * Copyright (c) 2004-2026
 *
 * Author(s):
 *  Volker Fischer
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

#include <QFrame>
#include <QTimer>
#include <QLayout>
#include "util.h"
#include "global.h"

/* Definitions ****************************************************************/
#define CLIP_IND_TIME_OUT_MS 20000

/* Classes ********************************************************************/
class CLevelMeter : public QWidget
{
    Q_OBJECT

    class CGradientLevelBar;

public:
    enum ELevelMeterType
    {
        MT_BAR_NARROW,
        MT_BAR_WIDE
    };

    CLevelMeter ( QWidget* parent = nullptr );
    virtual ~CLevelMeter();

    void SetValue ( const double dValue );
    void SetLevelMeterType ( const ELevelMeterType eNType );
    void SetNormalModeStyle ( const bool bEnable );
    void SetDarkTheme ( const bool bEnable );

protected:
    virtual void mousePressEvent ( QMouseEvent* ) override { ClipReset(); }

    void SetBarMeterStyleAndClipStatus ( const ELevelMeterType eNType, const bool bIsClip );

    CMinimumStackedLayout* pMinStackedLayout;
    ELevelMeterType        eLevelMeterType;
    bool                   bNormalModeStyle;
    bool                   bDarkTheme;
    CGradientLevelBar*     pGradientBar;

    QTimer TimerClip;

public slots:
    void ClipReset();
};
