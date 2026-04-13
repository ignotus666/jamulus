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
#include <QPainter>

COutputBandMeter::COutputBandMeter ( QWidget* parent ) :
    QWidget ( parent )
{
    for ( float& fLevel : afLevels )
    {
        fLevel = 0.0f;
    }

    setMinimumSize ( 180, 72 );
    setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );
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

void COutputBandMeter::paintEvent ( QPaintEvent* pEvent )
{
    Q_UNUSED ( pEvent )

    QPainter painter ( this );
    painter.setRenderHint ( QPainter::Antialiasing, false );

    const int iMargin = 2;
    const int iGap    = 2;
    const int iCount  = kBandCount;
    const int iH      = qMax ( 1, height() - 2 * iMargin );
    const int iWAvail = qMax ( 1, width() - 2 * iMargin - ( iCount - 1 ) * iGap );
    const int iBarW   = qMax ( 1, iWAvail / iCount );

    for ( int iBand = 0; iBand < iCount; ++iBand )
    {
        const int iX = iMargin + iBand * ( iBarW + iGap );
        const int iY = iMargin;

        painter.fillRect ( QRect ( iX, iY, iBarW, iH ), QColor ( 28, 32, 36, 90 ) );

        const int iFillH = qBound ( 0, static_cast<int> ( afLevels[iBand] * iH ), iH );
        if ( iFillH <= 0 )
        {
            continue;
        }

        QColor cColor ( 42, 198, 74 );
        if ( afLevels[iBand] > 0.90f )
        {
            cColor = QColor ( 218, 56, 56 );
        }
        else if ( afLevels[iBand] > 0.75f )
        {
            cColor = QColor ( 234, 136, 44 );
        }
        else if ( afLevels[iBand] > 0.60f )
        {
            cColor = QColor ( 214, 206, 72 );
        }

        painter.fillRect ( QRect ( iX, iY + iH - iFillH, iBarW, iFillH ), cColor );
    }
}
