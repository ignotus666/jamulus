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
#include <QPointer>
#include <QShowEvent>
#include <QHideEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QVBoxLayout>
#include "client.h"
#include "plugins/audioequalizer.h"
#include "settings.h"
#include "ui_effectsdlgbase.h"
#include "util.h"

#include <QWidget>
#include <QPainter>
#include <QLinearGradient>
#include <algorithm>

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

class CEffectsDlg : public CBaseDlg, private Ui_CEffectsDlgBase
{
    Q_OBJECT

public:
    CEffectsDlg ( CClient* pNCliP, CClientSettings* pNSetP, QWidget* parent = nullptr );

    void UpdateReverbControls();
    void UpdateCompressorControls();
    void UpdateEQControls();
    void UpdateEQReadouts();
    void UpdateOutputBandLevels ( const CVector<float>& vecOutLevels );
    void UpdateCompressorGainReduction ( const float fGRDb );
    void OnUIThemeChanged();
    void ApplyEffectsPreset ( const int iPresetSlot );

protected:
    virtual void showEvent ( QShowEvent* Event ) override;
    virtual void hideEvent ( QHideEvent* Event ) override;
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
    void EQBandGainChanged ( int bandIndex, int gainDb );
    void EQResetRequested();

private:
    CClient*         pClient;
    CClientSettings* pSettings;
    int              iSelectedBand = 0;
    CGRMeter*        pGRMeter      = nullptr;

    void PopulateEffectsPresetCombo();
    void ApplyEffectsPresetFromComboIndex ( const int iPresetIndex );
    void ApplyEffectsPresetFromSlot ( const int iPresetSlot );
    int  FindEffectsPresetSlotByName ( const QString& strName ) const;
    int  FindFreeEffectsPresetSlot() const;
    void PopulateEQPresetCombo();
    void ApplyPresetFromComboIndex ( const int iPresetIndex );
    void UpdateEQPresetSelection();
    void ApplyThemeToCustomWidgets();

    int  FindPresetSlotByName ( const QString& strName ) const;
    int  FindFreePresetSlot() const;
    void UpdateEQDynControls ( const int iBand );

private slots:
    void OnResetReverbClicked();
    void OnResetCompressorClicked();
    void OnSaveEffectsPresetClicked();
    void OnSaveAsEffectsPresetClicked();
    void OnDeleteEffectsPresetClicked();
    void OnResetEQClicked();
    void OnSaveEQPresetClicked();
    void OnSaveAsEQPresetClicked();
    void OnDeleteEQPresetClicked();

    // EQ dynamics and curve interaction slots
    void OnEQBandGainChanged ( int iBand, int iGainDb );
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
};
