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

#include <QColor>

/**
 * @brief Returns a distinct color for each EQ band (0..7).
 *
 * The palette is inspired by professional parametric EQ interfaces where
 * each frequency band has a unique, vibrant color to aid identification.
 */
inline QColor GetBandColor ( const int iBand )
{
    // 8 distinct, vibrant colors for the 8 EQ bands
    static const QColor aBandColors[8] = {
        QColor ( 102, 217,  73 ),  // Band 0: green
        QColor ( 100, 180, 255 ),  // Band 1: light blue
        QColor ( 255, 210,  60 ),  // Band 2: yellow
        QColor ( 230,  80, 180 ),  // Band 3: magenta / pink
        QColor ( 255, 130,  80 ),  // Band 4: coral / orange
        QColor ( 140, 120, 255 ),  // Band 5: purple / indigo
        QColor ( 190, 230,  60 ),  // Band 6: lime / yellow-green
        QColor (  80, 210, 220 ),  // Band 7: cyan / teal
    };

    if ( iBand >= 0 && iBand < 8 )
    {
        return aBandColors[iBand];
    }

    return QColor ( 255, 140, 0 ); // fallback orange
}

struct SControlPalette
{
    // Base
    QColor background;
    QColor dial;
    QColor dialOutline;
    QColor ring;

    // Accents
    QColor accent;
    QColor accentGlow;
    QColor accentBright;
    QColor accentMid;
    QColor accentDeep;

    // Knob
    QColor knobNormalTop;
    QColor knobNormalMid;
    QColor knobNormalBottom;
    QColor knobHoverTop;
    QColor knobHoverMid;
    QColor knobHoverBottom;
    QColor knobOutline;
    QColor markerNormal;
    QColor markerHover;
    QColor markerDot;
    QColor markerDotHover;
    QColor innerBevel;

    // Slider
    QColor trackBackground;
    QColor trackBorder;
    QColor handleTop;
    QColor handleMid;
    QColor handleBottom;
    QColor handleBorder;

    // Misc
    QColor tick;
};

inline SControlPalette GetControlPalette ( const bool bDarkTheme )
{
    if ( bDarkTheme )
    {
        return {
            QColor ( 28, 28, 31 ),        // background
            QColor ( 34, 34, 38 ),        // dial
            QColor ( 62, 62, 66 ),        // dial outline
            QColor ( 82, 82, 88 ),        // ring
            QColor ( 54, 207, 255 ),      // accent
            QColor ( 54, 207, 255, 120 ), // accent glow
            QColor ( 118, 244, 255 ),     // accent bright
            QColor ( 80, 220, 255 ),      // accent mid
            QColor ( 40, 160, 220 ),      // accent deep
            QColor ( 70, 82, 98 ),        // knob normal top
            QColor ( 44, 54, 66 ),        // knob normal mid
            QColor ( 30, 38, 48 ),        // knob normal bottom
            QColor ( 96, 114, 132 ),      // knob hover top
            QColor ( 58, 74, 90 ),        // knob hover mid
            QColor ( 34, 48, 62 ),        // knob hover bottom
            QColor ( 92, 106, 122 ),      // knob outline
            QColor ( 168, 216, 244 ),     // marker normal
            QColor ( 118, 238, 255 ),     // marker hover
            QColor ( 160, 200, 230 ),     // marker dot
            QColor ( 176, 228, 255 ),     // marker dot hover
            QColor ( 84, 96, 112 ),       // inner bevel
            QColor ( 44, 44, 48 ),        // track background
            QColor ( 72, 82, 94 ),        // track border
            QColor ( 82, 96, 112 ),       // handle top
            QColor ( 44, 54, 66 ),        // handle mid
            QColor ( 28, 36, 46 ),        // handle bottom
            QColor ( 92, 108, 124 ),      // handle border
            QColor ( 216, 224, 232, 230 ) // tick
        };
    }

    return {
        QColor ( 247, 248, 250 ),     // background
        QColor ( 230, 232, 236 ),     // dial
        QColor ( 180, 190, 200 ),     // dial outline
        QColor ( 200, 210, 220 ),     // ring
        QColor ( 50, 150, 200 ),      // accent
        QColor ( 50, 150, 200, 120 ), // accent glow
        QColor ( 118, 244, 255 ),     // accent bright
        QColor ( 80, 220, 255 ),      // accent mid
        QColor ( 40, 160, 220 ),      // accent deep
        QColor ( 255, 255, 255 ),     // knob normal top
        QColor ( 232, 237, 242 ),     // knob normal mid
        QColor ( 214, 221, 228 ),     // knob normal bottom
        QColor ( 245, 250, 255 ),     // knob hover top
        QColor ( 235, 242, 252 ),     // knob hover mid
        QColor ( 225, 235, 248 ),     // knob hover bottom
        QColor ( 145, 155, 165 ),     // knob outline
        QColor ( 80, 100, 150 ),      // marker normal
        QColor ( 50, 140, 200 ),      // marker hover
        QColor ( 80, 140, 200 ),      // marker dot
        QColor ( 100, 160, 220 ),     // marker dot hover
        QColor ( 180, 190, 200 ),     // inner bevel
        QColor ( 225, 228, 234 ),     // track background
        QColor ( 165, 175, 185 ),     // track border
        QColor ( 255, 255, 255 ),     // handle top
        QColor ( 232, 237, 242 ),     // handle mid
        QColor ( 214, 221, 228 ),     // handle bottom
        QColor ( 145, 155, 165 ),     // handle border
        QColor ( 120, 130, 142, 220 ) // tick
    };
}
