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

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QTabWidget>
#include <QShowEvent>
#include <QHideEvent>
#include <QEvent>
#include "client.h"
#include "plugins/audioequalizer.h"
#include "settings.h"
#include "ui_effectsdlgbase.h"
#include "util.h"

#include <QWidget>
#include <QPainter>
#include <QLinearGradient>
#include <QPainterPath>
#include <QVector>
#include <algorithm>

class QMouseEvent;

class CGRMeter : public QWidget
{
    Q_OBJECT
public:
    CGRMeter ( QWidget* parent = nullptr ) : QWidget ( parent ), fGainReductionDb ( 0.0f ), fVisualGRDb ( 0.0f ), bDarkTheme ( true )
    {
        setFixedWidth ( 40 );
        setMinimumHeight ( 120 );
    }

    void SetGainReductionDb ( const float fGRDb )
    {
        const float fVal = std::min ( 0.0f, fGRDb );
        if ( fVal < fGainReductionDb )
        {
            fGainReductionDb = fVal;
        }
        else
        {
            fGainReductionDb = 0.9f * fGainReductionDb + 0.1f * fVal;
        }

        if ( fGainReductionDb < fVisualGRDb )
        {
            fVisualGRDb = fGainReductionDb;
        }
        else
        {
            fVisualGRDb = 0.93f * fVisualGRDb + 0.07f * 0.0f;
        }
        update();
    }

    void SetDarkTheme ( const bool bEnable )
    {
        if ( bDarkTheme != bEnable )
        {
            bDarkTheme = bEnable;
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

        painter.fillRect ( r, bDarkTheme ? QColor ( 28, 28, 31 ) : QColor ( 247, 248, 250 ) );

        const int iBarW = 8;
        const int iBarL = 6;
        const int iBarT = 20;
        const int iBarH = r.height() - 30;
        QRect     barRect ( iBarL, iBarT, iBarW, iBarH );

        painter.fillRect ( barRect, bDarkTheme ? QColor ( 28, 35, 45 ) : QColor ( 214, 220, 228 ) );

        const float fMaxGRDb = -20.0f;
        const float fNormGR  = std::max ( 0.0f, std::min ( 1.0f, fVisualGRDb / fMaxGRDb ) );
        const int   iActiveH = static_cast<int> ( fNormGR * iBarH );

        if ( iActiveH > 0 )
        {
            QRect           activeRect ( iBarL, iBarT, iBarW, iActiveH );
            QLinearGradient grad ( activeRect.left(), activeRect.top(), activeRect.left(), activeRect.bottom() );
            grad.setColorAt ( 0.0, QColor ( 255, 140, 0 ) );
            grad.setColorAt ( 1.0, QColor ( 255, 69, 0 ) );

            painter.setPen ( Qt::NoPen );
            painter.setBrush ( grad );
            painter.drawRoundedRect ( activeRect, 1.5, 1.5 );
        }

        painter.setPen ( bDarkTheme ? QColor ( 170, 185, 200 ) : QColor ( 80, 90, 100 ) );
        QFont font = painter.font();
        font.setPointSize ( 7 );
        font.setBold ( true );
        painter.setFont ( font );
        painter.drawText ( QRect ( 0, 4, r.width(), 12 ), Qt::AlignCenter, "GR" );

        font.setBold ( false );
        painter.setFont ( font );
        painter.setPen ( bDarkTheme ? QColor ( 100, 115, 130 ) : QColor ( 140, 150, 160 ) );

        const int iTickX  = iBarL + iBarW + 4;
        const int iLabelX = iTickX + 5;

        const float adBValues[] = { 0.0f, -3.0f, -6.0f, -9.0f, -12.0f, -18.0f, -20.0f };
        for ( float fDb : adBValues )
        {
            const float fNorm = fDb / fMaxGRDb;
            const int   iY    = iBarT + static_cast<int> ( fNorm * iBarH );

            painter.drawLine ( iTickX, iY, iTickX + 3, iY );

            if ( fDb != -20.0f || r.height() > 140 )
            {
                painter.drawText ( iLabelX, iY + 3, QString::number ( static_cast<int> ( fDb ) ) );
            }
        }
    }

private:
    float fGainReductionDb;
    float fVisualGRDb;
    bool  bDarkTheme;
};

class CCompCurveWidget : public QWidget
{
    Q_OBJECT
public:
    CCompCurveWidget ( QWidget* parent = nullptr ) :
        QWidget ( parent ),
        fThresholdDb ( -12.0f ),
        fRatio ( 3.0f ),
        fMakeupDb ( 3.0f ),
        fCurrentInputDb ( -120.0f ),
        bBypass ( true ),
        bLimiterEnabled ( true ),
        bDarkTheme ( true )
    {
        setMinimumSize ( 140, 140 );
        setSizePolicy ( QSizePolicy::Preferred, QSizePolicy::Preferred );
    }

    void SetThreshold ( const float f )
    {
        fThresholdDb = f;
        update();
    }
    void SetRatio ( const float f )
    {
        fRatio = f;
        update();
    }
    void SetMakeup ( const float f )
    {
        fMakeupDb = f;
        update();
    }
    void SetCurrentInputDb ( const float f )
    {
        fCurrentInputDb = f;
        update();
    }
    void SetBypass ( const bool b )
    {
        bBypass = b;
        update();
    }
    void SetLimiterEnabled ( const bool b )
    {
        bLimiterEnabled = b;
        update();
    }
    void SetDarkTheme ( const bool b )
    {
        bDarkTheme = b;
        update();
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

        painter.fillRect ( r, bDarkTheme ? QColor ( 28, 28, 31 ) : QColor ( 247, 248, 250 ) );

        const int left   = 30;
        const int right  = 10;
        const int top    = 10;
        const int bottom = 20;

        const int plotW = r.width() - left - right;
        const int plotH = r.height() - top - bottom;

        if ( plotW <= 0 || plotH <= 0 )
            return;

        QRect plotRect ( left, top, plotW, plotH );
        painter.fillRect ( plotRect, QColor ( 20, 20, 22 ) );
        painter.setPen ( QColor ( 45, 45, 50 ) );
        painter.drawRect ( plotRect );

        auto mapX = [=] ( float dB ) -> float {
            float norm = ( dB - ( -60.0f ) ) / 60.0f;
            norm       = std::max ( 0.0f, std::min ( 1.0f, norm ) );
            return left + norm * plotW;
        };

        auto mapY = [=] ( float dB ) -> float {
            float norm = ( dB - ( -60.0f ) ) / 60.0f;
            norm       = std::max ( 0.0f, std::min ( 1.0f, norm ) );
            return top + ( 1.0f - norm ) * plotH;
        };

        const float gridValues[] = { -60.0f, -40.0f, -20.0f, 0.0f };
        painter.setFont ( QFont ( painter.font().family(), 7 ) );

        for ( float val : gridValues )
        {
            float y = mapY ( val );
            painter.setPen ( QColor ( 40, 45, 50, 120 ) );
            if ( val != -60.0f && val != 0.0f )
            {
                painter.drawLine ( left + 1, y, left + plotW - 1, y );
            }
            painter.setPen ( bDarkTheme ? QColor ( 100, 115, 130 ) : QColor ( 140, 150, 160 ) );
            painter.drawText ( QRect ( 2, y - 6, left - 6, 12 ), Qt::AlignRight | Qt::AlignVCenter, QString::number ( static_cast<int> ( val ) ) );

            float x = mapX ( val );
            painter.setPen ( QColor ( 40, 45, 50, 120 ) );
            if ( val != -60.0f && val != 0.0f )
            {
                painter.drawLine ( x, top + 1, x, top + plotH - 1 );
            }
            painter.setPen ( bDarkTheme ? QColor ( 100, 115, 130 ) : QColor ( 140, 150, 160 ) );
            painter.drawText ( QRect ( x - 15, top + plotH + 2, 30, 12 ), Qt::AlignCenter, QString::number ( static_cast<int> ( val ) ) );
        }

        painter.setPen ( QPen ( QColor ( 80, 90, 100, 100 ), 1, Qt::DashLine ) );
        painter.drawLine ( mapX ( -60.0f ), mapY ( -60.0f ), mapX ( 0.0f ), mapY ( 0.0f ) );

        auto computeGainDb = [this] ( float inputDb ) -> float {
            float fKneeDb   = 6.0f;
            float fKneeHalf = fKneeDb * 0.5f;

            if ( inputDb <= ( fThresholdDb - fKneeHalf ) )
            {
                return 0.0f;
            }

            if ( inputDb >= ( fThresholdDb + fKneeHalf ) )
            {
                const float fOverDb       = inputDb - fThresholdDb;
                const float fCompressedDb = fOverDb / fRatio;
                return fCompressedDb - fOverDb;
            }

            const float fKneeInput    = inputDb - ( fThresholdDb - fKneeHalf );
            const float fKneeRatio    = fKneeInput / fKneeDb;
            const float fOverDb       = inputDb - fThresholdDb;
            const float fCompressedDb = fOverDb / fRatio;
            const float fGainFull     = fCompressedDb - fOverDb;

            return fGainFull * ( fKneeRatio * fKneeRatio );
        };

        QPainterPath curvePath;
        const int    steps = plotW;
        bool         first = true;

        for ( int px = 0; px <= steps; ++px )
        {
            float inputDb  = -60.0f + ( static_cast<float> ( px ) / steps ) * 60.0f;
            float fGR      = computeGainDb ( inputDb );
            float outputDb = inputDb + fGR + fMakeupDb;

            if ( bLimiterEnabled )
            {
                outputDb = std::min ( -1.0f, outputDb );
            }

            float py = mapY ( outputDb );
            if ( first )
            {
                curvePath.moveTo ( left + px, py );
                first = false;
            }
            else
            {
                curvePath.lineTo ( left + px, py );
            }
        }

        QPainterPath fillPath = curvePath;
        fillPath.lineTo ( left + plotW, top + plotH );
        fillPath.lineTo ( left, top + plotH );
        fillPath.closeSubpath();

        QLinearGradient curveGrad ( left, top, left, top + plotH );
        curveGrad.setColorAt ( 0.0, QColor ( 0, 191, 255, 60 ) );
        curveGrad.setColorAt ( 1.0, QColor ( 0, 100, 255, 10 ) );
        painter.setPen ( Qt::NoPen );
        painter.setBrush ( curveGrad );
        painter.drawPath ( fillPath );

        QPen curvePen = QPen ( QColor ( 0, 191, 255 ), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
        painter.setPen ( curvePen );
        painter.setBrush ( Qt::NoBrush );
        painter.drawPath ( curvePath );

        float threshX = mapX ( fThresholdDb );
        painter.setPen ( QPen ( QColor ( 255, 165, 0, 150 ), 1, Qt::DashLine ) );
        painter.drawLine ( threshX, top + 1, threshX, top + plotH - 1 );

        if ( fCurrentInputDb > -60.0f )
        {
            float inputDb  = std::max ( -60.0f, std::min ( 0.0f, fCurrentInputDb ) );
            float fGR      = computeGainDb ( inputDb );
            float outputDb = inputDb + fGR + fMakeupDb;
            if ( bLimiterEnabled )
            {
                outputDb = std::min ( -1.0f, outputDb );
            }

            QPointF center ( mapX ( inputDb ), mapY ( outputDb ) );

            painter.setPen ( Qt::NoPen );
            painter.setBrush ( QColor ( 255, 69, 0, 80 ) );
            painter.drawEllipse ( center, 6.0, 6.0 );

            painter.setBrush ( QColor ( 255, 140, 0 ) );
            painter.drawEllipse ( center, 3.0, 3.0 );
        }
    }

private:
    float fThresholdDb;
    float fRatio;
    float fMakeupDb;
    float fCurrentInputDb;
    bool  bBypass;
    bool  bLimiterEnabled;
    bool  bDarkTheme;
};

class CReverbDecayWidget : public QWidget
{
    Q_OBJECT
public:
    CReverbDecayWidget ( QWidget* parent = nullptr ) :
        QWidget ( parent ),
        fPreDelayMs ( 0.0f ),
        fRoomSize ( 0.5f ),
        fDamping ( 0.3f ),
        fWetMix ( 0.5f ),
        fEarlyLevel ( 0.5f ),
        bBypass ( true ),
        bDarkTheme ( true ),
        fDisplayLevelDb ( -120.0f )
    {
        setMinimumSize ( 140, 120 );
        setSizePolicy ( QSizePolicy::Preferred, QSizePolicy::Preferred );
    }

    void SetPreDelayMs ( const float f )
    {
        fPreDelayMs = f;
        update();
    }
    void SetRoomSize ( const float f )
    {
        fRoomSize = f;
        update();
    }
    void SetDamping ( const float f )
    {
        fDamping = f;
        update();
    }
    void SetWetMix ( const float f )
    {
        fWetMix = f;
        update();
    }
    void SetEarlyLevel ( const float f )
    {
        fEarlyLevel = f;
        update();
    }
    void SetBypass ( const bool b )
    {
        bBypass = b;
        update();
    }
    void SetDarkTheme ( const bool b )
    {
        bDarkTheme = b;
        update();
    }

    void SetOutputLevelDb ( const float f )
    {
        if ( f > fDisplayLevelDb )
        {
            fDisplayLevelDb = f;
        }
        else
        {
            // Smooth release: decay by 1.5 dB per timer tick
            fDisplayLevelDb = std::max ( -120.0f, fDisplayLevelDb - 1.5f );
        }
        update();
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

        painter.fillRect ( r, bDarkTheme ? QColor ( 28, 28, 31 ) : QColor ( 247, 248, 250 ) );

        const int left   = 15;
        const int right  = 15;
        const int top    = 15;
        const int bottom = 20;

        const int plotW = r.width() - left - right;
        const int plotH = r.height() - top - bottom;

        if ( plotW <= 0 || plotH <= 0 )
            return;

        QRect plotRect ( left, top, plotW, plotH );
        painter.fillRect ( plotRect, QColor ( 20, 20, 22 ) );
        painter.setPen ( QColor ( 45, 45, 50 ) );
        painter.drawRect ( plotRect );

        painter.setPen ( QColor ( 40, 45, 50, 120 ) );
        for ( int i = 1; i <= 3; ++i )
        {
            float gridX = left + ( static_cast<float> ( i ) / 4.0f ) * plotW;
            painter.drawLine ( gridX, top + 1, gridX, top + plotH - 1 );
        }

        painter.setFont ( QFont ( painter.font().family(), 6 ) );
        painter.setPen ( bDarkTheme ? QColor ( 100, 115, 130 ) : QColor ( 140, 150, 160 ) );
        painter.drawText ( QRect ( left - 20, top + plotH + 2, 40, 12 ), Qt::AlignCenter, "0 ms" );
        painter.drawText ( QRect ( left + plotW / 2 - 30, top + plotH + 2, 60, 12 ), Qt::AlignCenter, "500 ms" );
        painter.drawText ( QRect ( left + plotW - 40, top + plotH + 2, 60, 12 ), Qt::AlignCenter, "1000 ms" );

        float xPreDelay = left + ( std::min ( fPreDelayMs, 120.0f ) / 1000.0f ) * plotW;

        painter.setPen ( QPen ( QColor ( 80, 90, 100, 150 ), 1.5, Qt::SolidLine ) );
        painter.drawLine ( left, top + plotH - 1, xPreDelay, top + plotH - 1 );

        float spikeOffsetsMs[] = { 15.0f, 35.0f, 55.0f, 75.0f, 95.0f };
        float spikeScales[]    = { 0.8f, 0.6f, 0.45f, 0.3f, 0.15f };

        QPen spikePen ( QColor ( 255, 140, 0 ), 1.5, Qt::SolidLine, Qt::RoundCap );
        painter.setPen ( spikePen );

        for ( int i = 0; i < 5; ++i )
        {
            float spikeX = xPreDelay + ( spikeOffsetsMs[i] / 1000.0f ) * plotW;
            if ( spikeX < left + plotW )
            {
                float spikeH = spikeScales[i] * fEarlyLevel * plotH * 0.8f;
                painter.drawLine ( QPointF ( spikeX, top + plotH ), QPointF ( spikeX, top + plotH - spikeH ) );
            }
        }

        float normLevel = ( fDisplayLevelDb + 60.0f ) / 60.0f;
        if ( normLevel < 0.0f )
            normLevel = 0.0f;
        if ( normLevel > 1.0f )
            normLevel = 1.0f;

        float xLate = xPreDelay + ( 40.0f / 1000.0f ) * plotW;
        if ( xLate < left + plotW )
        {
            QPainterPath curvePath;
            bool         first = true;

            float tau = 0.08f + ( fRoomSize * 0.65f ) * ( 1.0f - fDamping * 0.45f );

            int steps = static_cast<int> ( left + plotW - xLate );
            for ( int px = 0; px <= steps; ++px )
            {
                float curX  = xLate + px;
                float t     = ( static_cast<float> ( px ) / plotW ) * 1.2f;
                float decay = std::exp ( -t / tau );
                float amp   = fWetMix * decay;
                float curY  = ( top + plotH ) - amp * plotH * 0.8f;

                if ( first )
                {
                    curvePath.moveTo ( curX, curY );
                    first = false;
                }
                else
                {
                    curvePath.lineTo ( curX, curY );
                }
            }

            QPainterPath fillPath = curvePath;
            fillPath.lineTo ( left + plotW, top + plotH );
            fillPath.lineTo ( xLate, top + plotH );
            fillPath.closeSubpath();

            // Pulsate decay curve fill opacity with wet level
            int alpha0 = 10 + static_cast<int> ( normLevel * 60 ); // 10 to 70
            int alpha1 = 5 + static_cast<int> ( normLevel * 30 );  // 5 to 35
            int alpha2 = 2 + static_cast<int> ( normLevel * 3 );   // 2 to 5

            QLinearGradient decayGrad ( xLate, top, left + plotW, top );
            decayGrad.setColorAt ( 0.0, QColor ( 0, 191, 255, alpha0 ) );
            decayGrad.setColorAt ( 0.5, QColor ( 0, 150, 255, alpha1 ) );
            decayGrad.setColorAt ( 1.0, QColor ( 30, 58, 138, alpha2 ) );

            painter.setPen ( Qt::NoPen );
            painter.setBrush ( decayGrad );
            painter.drawPath ( fillPath );

            // Pulsate curve outline opacity with wet level
            int  curveAlpha = 80 + static_cast<int> ( normLevel * 175 ); // 80 to 255
            QPen curvePen ( QColor ( 0, 191, 255, curveAlpha ), 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
            painter.setPen ( curvePen );
            painter.setBrush ( Qt::NoBrush );
            painter.drawPath ( curvePath );
        }

        // Draw dynamic VU-meter bar on the right side of the plot area
        const int   meterW = 6;
        const int   meterX = left + plotW - meterW - 4;
        const QRect meterRect ( meterX, top + 4, meterW, plotH - 8 );

        painter.fillRect ( meterRect, QColor ( 15, 15, 17 ) );
        painter.setPen ( QColor ( 45, 45, 50 ) );
        painter.drawRect ( meterRect );

        if ( normLevel > 0.0f )
        {
            int   fillH = static_cast<int> ( normLevel * ( meterRect.height() - 2 ) );
            QRect fillRect ( meterRect.left() + 1, meterRect.bottom() - fillH, meterRect.width() - 2, fillH );

            QLinearGradient meterGrad ( fillRect.left(), fillRect.top(), fillRect.left(), fillRect.bottom() );
            meterGrad.setColorAt ( 0.0, QColor ( 255, 140, 0 ) ); // Orange
            meterGrad.setColorAt ( 1.0, QColor ( 255, 69, 0 ) );  // Red-orange

            painter.fillRect ( fillRect, meterGrad );
        }
    }

private:
    float fPreDelayMs;
    float fRoomSize;
    float fDamping;
    float fWetMix;
    float fEarlyLevel;
    bool  bBypass;
    bool  bDarkTheme;
    float fDisplayLevelDb;
};

class CEffectsDlg : public CBaseDlg, private Ui_CEffectsDlgBase
{
    Q_OBJECT

public:
    enum EEffectsContext
    {
        EC_INPUT,
        EC_OUTPUT
    };

    CEffectsDlg ( CClient* pNCliP, CClientSettings* pNSetP, QWidget* parent = nullptr );

    EEffectsContext GetContext() const { return eCurrentContext; }
    void            SetContext ( EEffectsContext eContext );

    void UpdateReverbControls();
    void UpdateCompressorControls();
    void UpdateEQControls();
    void UpdateOutputBandLevels ( const CVector<float>& vecOutLevels );
    void UpdateCompressorGainReduction ( const float fGRDb, const float fInputDb );
    void UpdateReverbOutputLevel ( const float fLevelDb );
    void OnUIThemeChanged();
    void ApplyEffectsPreset ( const int iPresetSlot );

protected:
    virtual void showEvent ( QShowEvent* Event ) override;
    virtual void hideEvent ( QHideEvent* Event ) override;
    virtual void mousePressEvent ( QMouseEvent* pEvent ) override;
    virtual bool eventFilter ( QObject* pObj, QEvent* pEvent ) override;

signals:
    void ReverbValueChanged ( int value );
    void ReverbBypassChanged ( bool bypassed );
    void ReverbPreDelayChanged ( int value );
    void ReverbRoomSizeChanged ( int value );
    void ReverbDampingChanged ( int value );
    void ReverbWetMixChanged ( int value );
    void ReverbEarlyLevelChanged ( int value );
    void ReverbWidthChanged ( int value );
    void ReverbLeftSelected();
    void ReverbRightSelected();
    void ReverbEarlyEnabledChanged ( bool enabled );
    void ReverbFreezeChanged ( bool enabled );
    void CompressorBypassChanged ( bool bypassed );
    void CompressorThresholdChanged ( int value );
    void CompressorRatioChanged ( int value );
    void CompressorAttackChanged ( int value );
    void CompressorReleaseChanged ( int value );
    void CompressorMakeupChanged ( int value );
    void CompressorLimiterChanged ( bool enabled );
    void EQBypassChanged ( bool bypassed );

private:
    CClient*            pClient;
    CClientSettings*    pSettings;
    int                 iSelectedBand      = 0;
    CGRMeter*           pGRMeter           = nullptr;
    CCompCurveWidget*   pCompCurveWidget   = nullptr;
    CReverbDecayWidget* pReverbDecayWidget = nullptr;
    EEffectsContext     eCurrentContext    = EC_INPUT;

    // Context-aware settings helpers
    CVector<QString>& GetEffectsPresetNames() const { return pSettings->vstrEffectsPresetNames[eCurrentContext == EC_OUTPUT ? 1 : 0]; }
    int&              GetSelectedEffectsPreset() const { return pSettings->iSelectedEffectsPreset[eCurrentContext == EC_OUTPUT ? 1 : 0]; }

public:
    // Context-aware client parameters getters & setters
    int  GetReverbLevel() const { return pClient ? static_cast<int> ( pClient->GetReverbLevel ( eCurrentContext == EC_OUTPUT ) ) : 0; }
    void SetReverbLevel ( int iNL )
    {
        if ( pClient )
        {
            pClient->GetReverbLevel ( eCurrentContext == EC_OUTPUT ) = iNL;
        }
    }
    bool IsReverbOnLeftChan() const { return pClient ? static_cast<bool> ( pClient->GetReverbOnLeftChan ( eCurrentContext == EC_OUTPUT ) ) : false; }
    void SetReverbOnLeftChan ( bool bOnLeft )
    {
        if ( pClient )
        {
            pClient->GetReverbOnLeftChan ( eCurrentContext == EC_OUTPUT ) = bOnLeft;
        }
    }
    int  GetReverbPreDelayMs() const { return pClient ? pClient->GetReverbPreDelayMs ( eCurrentContext == EC_OUTPUT ) : 0; }
    void SetReverbPreDelayMs ( int iMs )
    {
        if ( pClient )
        {
            pClient->GetReverbPreDelayMs ( eCurrentContext == EC_OUTPUT ) = iMs;
        }
    }
    int  GetReverbRoomSize() const { return pClient ? pClient->GetReverbRoomSize ( eCurrentContext == EC_OUTPUT ) : 0; }
    void SetReverbRoomSize ( int iValue )
    {
        if ( pClient )
        {
            pClient->GetReverbRoomSize ( eCurrentContext == EC_OUTPUT ) = iValue;
        }
    }
    int  GetReverbDamping() const { return pClient ? pClient->GetReverbDamping ( eCurrentContext == EC_OUTPUT ) : 0; }
    void SetReverbDamping ( int iValue )
    {
        if ( pClient )
        {
            pClient->GetReverbDamping ( eCurrentContext == EC_OUTPUT ) = iValue;
        }
    }
    int  GetReverbWetMix() const { return pClient ? pClient->GetReverbWetMix ( eCurrentContext == EC_OUTPUT ) : 0; }
    void SetReverbWetMix ( int iValue )
    {
        if ( pClient )
        {
            pClient->GetReverbWetMix ( eCurrentContext == EC_OUTPUT ) = iValue;
        }
    }
    int  GetReverbEarlyLevel() const { return pClient ? pClient->GetReverbEarlyLevel ( eCurrentContext == EC_OUTPUT ) : 0; }
    void SetReverbEarlyLevel ( int iValue )
    {
        if ( pClient )
        {
            pClient->GetReverbEarlyLevel ( eCurrentContext == EC_OUTPUT ) = iValue;
        }
    }
    bool GetReverbEarlyEnabled() const { return pClient ? pClient->GetReverbEarlyEnabled ( eCurrentContext == EC_OUTPUT ) : false; }
    void SetReverbEarlyEnabled ( bool bEnabled )
    {
        if ( pClient )
        {
            pClient->GetReverbEarlyEnabled ( eCurrentContext == EC_OUTPUT ) = bEnabled;
        }
    }
    int  GetReverbWidth() const { return pClient ? pClient->GetReverbWidth ( eCurrentContext == EC_OUTPUT ) : 0; }
    void SetReverbWidth ( int iValue )
    {
        if ( pClient )
        {
            pClient->GetReverbWidth ( eCurrentContext == EC_OUTPUT ) = iValue;
        }
    }
    bool GetReverbFreeze() const { return pClient ? pClient->GetReverbFreeze ( eCurrentContext == EC_OUTPUT ) : false; }
    void SetReverbFreeze ( bool bEnabled )
    {
        if ( pClient )
        {
            pClient->GetReverbFreeze ( eCurrentContext == EC_OUTPUT ) = bEnabled;
        }
    }
    bool GetReverbBypass() const { return pClient ? pClient->GetReverbBypass ( eCurrentContext == EC_OUTPUT ) : true; }
    void SetReverbBypass ( bool bBypass )
    {
        if ( pClient )
        {
            pClient->GetReverbBypass ( eCurrentContext == EC_OUTPUT ) = bBypass;
        }
    }

    bool GetCompressorBypass() const { return pClient ? pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).GetBypass() : true; }
    void SetCompressorBypass ( bool bBypass )
    {
        if ( pClient )
        {
            pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).SetBypass ( bBypass );
        }
    }
    float GetCompressorThresholdDb() const { return pClient ? pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).GetThresholdDb() : 0.0f; }
    void  SetCompressorThresholdDb ( float fDb )
    {
        if ( pClient )
        {
            pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).SetThresholdDb ( fDb );
        }
    }
    float GetCompressorRatio() const { return pClient ? pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).GetRatio() : 1.0f; }
    void  SetCompressorRatio ( float fValue )
    {
        if ( pClient )
        {
            pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).SetRatio ( fValue );
        }
    }
    float GetCompressorAttackMs() const { return pClient ? pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).GetAttackMs() : 1.0f; }
    void  SetCompressorAttackMs ( float fMs )
    {
        if ( pClient )
        {
            pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).SetAttackMs ( fMs );
        }
    }
    float GetCompressorReleaseMs() const { return pClient ? pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).GetReleaseMs() : 1.0f; }
    void  SetCompressorReleaseMs ( float fMs )
    {
        if ( pClient )
        {
            pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).SetReleaseMs ( fMs );
        }
    }
    float GetCompressorMakeupDb() const { return pClient ? pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).GetMakeupDb() : 0.0f; }
    void  SetCompressorMakeupDb ( float fDb )
    {
        if ( pClient )
        {
            pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).SetMakeupDb ( fDb );
        }
    }
    bool GetCompressorLimiterEnabled() const { return pClient ? pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).GetLimiterEnabled() : false; }
    void SetCompressorLimiterEnabled ( bool bEnabled )
    {
        if ( pClient )
        {
            pClient->GetCompressor ( eCurrentContext == EC_OUTPUT ).SetLimiterEnabled ( bEnabled );
        }
    }

    bool GetEQBypass() const { return pClient ? pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).GetBypass() : true; }
    void SetEQBypass ( bool bNBypass )
    {
        if ( pClient )
        {
            pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).SetBypass ( bNBypass );
        }
    }
    float GetEQBandGainDb ( int iBandIndex ) const
    {
        return pClient ? pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).GetBandGainDb ( iBandIndex ) : 0.0f;
    }
    void SetEQBandGainDb ( int iBandIndex, float fGainDb )
    {
        if ( pClient )
        {
            pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).SetBandGainDb ( iBandIndex, fGainDb );
        }
    }
    float GetEQBandFrequency ( int iBandIndex ) const
    {
        return pClient ? pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).GetBandFrequency ( iBandIndex ) : 0.0f;
    }
    void SetEQBandFrequency ( int iBandIndex, float fFreqHz )
    {
        if ( pClient )
        {
            pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).SetBandFrequency ( iBandIndex, fFreqHz );
        }
    }
    float GetEQBandQ ( int iBand ) const { return pClient ? pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).GetBandQ ( iBand ) : 1.0f; }
    void  SetEQBandQ ( int iBand, float fQ )
    {
        if ( pClient )
        {
            pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).SetBandQ ( iBand, fQ );
        }
    }
    bool GetEQBandDynEnabled ( int iBand ) const
    {
        return pClient ? pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).GetBandDynEnabled ( iBand ) : false;
    }
    void SetEQBandDynEnabled ( int iBand, bool bEnabled )
    {
        if ( pClient )
        {
            pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).SetBandDynEnabled ( iBand, bEnabled );
        }
    }
    float GetEQBandDynThresholdDb ( int iBand ) const
    {
        return pClient ? pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).GetBandDynThresholdDb ( iBand ) : -20.0f;
    }
    void SetEQBandDynThresholdDb ( int iBand, float fDb )
    {
        if ( pClient )
        {
            pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).SetBandDynThresholdDb ( iBand, fDb );
        }
    }
    float GetEQBandDynRatio ( int iBand ) const { return pClient ? pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).GetBandDynRatio ( iBand ) : 4.0f; }
    void  SetEQBandDynRatio ( int iBand, float fRatio )
    {
        if ( pClient )
        {
            pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).SetBandDynRatio ( iBand, fRatio );
        }
    }
    float GetEQBandDynAttackMs ( int iBand ) const
    {
        return pClient ? pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).GetBandDynAttackMs ( iBand ) : 5.0f;
    }
    void SetEQBandDynAttackMs ( int iBand, float fMs )
    {
        if ( pClient )
        {
            pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).SetBandDynAttackMs ( iBand, fMs );
        }
    }
    float GetEQBandDynReleaseMs ( int iBand ) const
    {
        return pClient ? pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).GetBandDynReleaseMs ( iBand ) : 80.0f;
    }
    void SetEQBandDynReleaseMs ( int iBand, float fMs )
    {
        if ( pClient )
        {
            pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).SetBandDynReleaseMs ( iBand, fMs );
        }
    }
    float GetEQBandGainReductionDb ( int iBand ) const
    {
        return pClient ? pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).GetBandGainReductionDb ( iBand ) : 0.0f;
    }

    void PopulateEffectsPresetCombo();
    void ApplyEffectsPresetFromComboIndex ( const int iPresetIndex );
    void ApplyEffectsPresetFromSlot ( const int iPresetSlot );
    int  FindEffectsPresetSlotByName ( const QString& strName ) const;
    int  FindFreeEffectsPresetSlot() const;
    void ApplyThemeToCustomWidgets();

    void UpdateEQDynControls ( const int iBand );

private slots:
    void OnContextInputClicked();
    void OnContextOutputClicked();

    void OnResetReverbClicked();
    void OnResetCompressorClicked();
    void OnSaveEffectsPresetClicked();
    void OnSaveAsEffectsPresetClicked();
    void OnDeleteEffectsPresetClicked();
    void OnResetEQClicked();

    void OnEQBandGainChanged ( int iBand, float fGainDb );
    void OnEQBandFrequencyChanged ( int iBand, float fFreqHz );
    void OnEQBandSelected ( int iBand );
    void OnEQBandGainReset ( int iBand );
    void OnEQDynEnabledChanged ( bool bEnabled );
    void OnEQDynThresholdChanged ( int iValue );
    void OnEQDynRatioChanged ( int iValue );
    void OnEQDynAttackChanged ( int iValue );
    void OnEQDynReleaseChanged ( int iValue );
    void OnEQBandQChanged ( int iValue );
    void OnEQDynFreqEditFinished();
    void OnEQDynGainEditFinished();
    void OnEQBandGainKnobChanged ( int iValue );
    void OnEQBandQEditFinished();
    void OnEQDynThresholdEditFinished();
    void OnEQDynRatioEditFinished();
    void OnEQDynAttackEditFinished();
    void OnEQDynReleaseEditFinished();
};
