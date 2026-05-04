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

#include <QVector>
#include <QWidget>
#include "util.h"

class COutputBandMeter : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kBandCount = 16;

    COutputBandMeter ( QWidget* parent = nullptr );

    void SetLevels ( const CVector<float>& vecLevels );
    void SetBandCenters ( const QVector<int>& vecBandCentersPx );
    void SetDarkTheme ( bool bEnable );

protected:
    virtual void paintEvent ( QPaintEvent* pEvent ) override;

private:
    float        afLevels[kBandCount];
    QVector<int> vecBandCentersPx;
    bool         bDarkTheme;
};
