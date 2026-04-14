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
#include <QSlider>
#include <QTabWidget>
#include <QShowEvent>
#include <QVBoxLayout>
#include "client.h"
#include "outputbandmeter.h"
#include "plugins/audioequalizer.h"
#include "settings.h"
#include "util.h"

class CEffectsDlg : public CBaseDlg
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

protected:
    virtual void showEvent ( QShowEvent* Event ) override;

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
    QTabWidget*   pTabs;
    QComboBox*    pCbxEffectsPresets;
    QPushButton*  pButEffectsSavePreset;
    QPushButton*  pButEffectsSaveAsPreset;
    QPushButton*  pButEffectsDeletePreset;
    QSlider*      pSldReverb;
    QSlider*      pSldReverbPreDelay;
    QSlider*      pSldReverbRoom;
    QSlider*      pSldReverbDamping;
    QSlider*      pSldReverbWet;
    QSlider*      pSldReverbEarly;
    QSlider*      pSldReverbWidth;
    QRadioButton* pRbtReverbSelL;
    QRadioButton* pRbtReverbSelR;
    QLabel*       pLblStereoHint;
    QLabel*       pLblReverbPreDelayValue;
    QLabel*       pLblReverbRoomValue;
    QLabel*       pLblReverbDampingValue;
    QLabel*       pLblReverbWetValue;
    QLabel*       pLblReverbEarlyValue;
    QLabel*       pLblReverbWidthValue;
    QCheckBox*    pChbReverbEarly;
    QCheckBox*    pChbReverbFreeze;
    QCheckBox*    pChbReverbBypass;
    QPushButton*  pButReverbReset;

    QCheckBox*    pChbCompressorBypass;
    QCheckBox*    pChbCompressorLimiter;
    QPushButton*  pButCompressorReset;
    QSlider*      pSldCompressorThreshold;
    QSlider*      pSldCompressorRatio;
    QSlider*      pSldCompressorAttack;
    QSlider*      pSldCompressorRelease;
    QSlider*      pSldCompressorMakeup;
    QLabel*       pLblCompressorThresholdValue;
    QLabel*       pLblCompressorRatioValue;
    QLabel*       pLblCompressorAttackValue;
    QLabel*       pLblCompressorReleaseValue;
    QLabel*       pLblCompressorMakeupValue;

    QCheckBox*    pChbFilterBypass;
    QCheckBox*    pChbHighPass;
    QCheckBox*    pChbLowPass;
    QPushButton*  pButFilterReset;
    QSlider*      pSldHighPassCutoff;
    QSlider*      pSldLowPassCutoff;
    QLabel*       pLblHighPassValue;
    QLabel*       pLblLowPassValue;

    QCheckBox*    pChbEQBypass;
    QComboBox*    pCbxEQPresets;
    QPushButton*  pButEQSavePreset;
    QPushButton*  pButEQSaveAsPreset;
    QPushButton*  pButEQDeletePreset;
    QPushButton*  pButEQReset;
    QSlider*      pSldEQBands[CAudioEqualizer::NUM_BANDS];
    QLabel*       pLblEQBandValues[CAudioEqualizer::NUM_BANDS];
    QLabel*       pLblOutputBandTitle;
    COutputBandMeter* pOutputBandMeter;

    void PopulateEffectsPresetCombo();
    void ApplyEffectsPresetFromComboIndex ( const int iPresetIndex );
    void ApplyEffectsPresetFromSlot ( const int iPresetSlot );
    int  FindEffectsPresetSlotByName ( const QString& strName ) const;
    int  FindFreeEffectsPresetSlot() const;
    void PopulateEQPresetCombo();
    void ApplyPresetFromComboIndex ( const int iPresetIndex );
    void UpdateEQPresetSelection();
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
