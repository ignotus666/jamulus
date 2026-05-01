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
#include "customslider.h"
#include <QTabWidget>
#include <QPointer>
#include <QShowEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include "client.h"
#include "outputbandmeter.h"
#include "plugins/audioequalizer.h"
#include "settings.h"
#include "ui_effectsdlgbase.h"
#include "util.h"

class CEffectsDlg : public CBaseDlg, private Ui_CEffectsDlgBase
{
    Q_OBJECT

public:
    CEffectsDlg ( CClient* pNCliP, CClientSettings* pNSetP, QWidget* parent = nullptr );

    void UpdateReverbControls();
    void UpdateFilterControls();
    void UpdateCompressorControls();
    void UpdateEQControls();
    void UpdateEQReadouts();
    void UpdateOutputBandLevels ( const CVector<float>& vecOutLevels );
    void OnUIThemeChanged();

protected:
    virtual void showEvent ( QShowEvent* Event ) override;
    virtual void resizeEvent ( QResizeEvent* Event ) override;

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
    void FilterBypassChanged ( bool bypassed );
    void HighPassEnabledChanged ( bool enabled );
    void LowPassEnabledChanged ( bool enabled );
    void HighPassCutoffChanged ( int value );
    void LowPassCutoffChanged ( int value );
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
    CClient*      pClient;
    CClientSettings* pSettings;
    QPointer<COutputBandMeter> pOutputBandMeterSafe;
    QPointer<CCustomSlider> pSldEQBands[CAudioEqualizer::NUM_BANDS] = {};
    QPointer<QLabel>  pLblEQBandValues[CAudioEqualizer::NUM_BANDS] = {};
    bool          bEQBandWidgetsReady = false;

    void PopulateEffectsPresetCombo();
    void ApplyEffectsPresetFromComboIndex ( const int iPresetIndex );
    void ApplyEffectsPresetFromSlot ( const int iPresetSlot );
    int  FindEffectsPresetSlotByName ( const QString& strName ) const;
    int  FindFreeEffectsPresetSlot() const;
    void PopulateEQPresetCombo();
    void ApplyPresetFromComboIndex ( const int iPresetIndex );
    void UpdateEQPresetSelection();
    void ApplyThemeToCustomWidgets();
    void UpdateOutputBandAlignment();
    int  FindPresetSlotByName ( const QString& strName ) const;
    int  FindFreePresetSlot() const;

private slots:
    void OnResetReverbClicked();
    void OnResetFilterClicked();
    void OnResetCompressorClicked();
    void OnSaveEffectsPresetClicked();
    void OnSaveAsEffectsPresetClicked();
    void OnDeleteEffectsPresetClicked();
    void OnResetEQClicked();
    void OnSaveEQPresetClicked();
    void OnSaveAsEQPresetClicked();
    void OnDeleteEQPresetClicked();
};
