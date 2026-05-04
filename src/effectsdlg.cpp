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

#include "effectsdlg.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>

CEffectsDlg::CEffectsDlg ( CClient* pNCliP, CClientSettings* pNSetP, QWidget* parent ) :
    CBaseDlg ( parent, Qt::Window ),
    pClient ( pNCliP ),
    pSettings ( pNSetP )
{
    setupUi ( this );
    pOutputBandMeterSafe = pOutputBandMeter;

    // Save tab index on change
    connect ( pTabs, &QTabWidget::currentChanged, this, [this] ( int idx ) { pSettings->iEffectsTab = idx; } );

    setWindowTitle ( tr ( "Effects" ) );
    pSldReverb->setRange ( 0, AUD_REVERB_MAX );
    pSldReverb->setTickInterval ( AUD_REVERB_MAX / 5 );
    pSldReverb->setTickPosition ( QSlider::TicksBothSides );

    pLblReverbPreDelayValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblReverbRoomValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblReverbDampingValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblReverbWetValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblReverbEarlyValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblReverbWidthValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblReverbPreDelayValue->setMinimumWidth ( 32 );
    pLblReverbRoomValue->setMinimumWidth ( 32 );
    pLblReverbDampingValue->setMinimumWidth ( 32 );
    pLblReverbWetValue->setMinimumWidth ( 32 );
    pLblReverbEarlyValue->setMinimumWidth ( 32 );
    pLblReverbWidthValue->setMinimumWidth ( 32 );

    pSldReverbPreDelay->setRange ( 0, REVERB_PRE_DELAY_MAX_MS );
    pSldReverbPreDelay->setTickInterval ( std::max ( 1, REVERB_PRE_DELAY_MAX_MS / 5 ) );
    pSldReverbPreDelay->setTickPosition ( QSlider::TicksBothSides );
    pSldReverbRoom->setRange ( 0, REVERB_ROOM_SIZE_MAX );
    pSldReverbRoom->setTickInterval ( std::max ( 1, REVERB_ROOM_SIZE_MAX / 5 ) );
    pSldReverbRoom->setTickPosition ( QSlider::TicksBothSides );
    pSldReverbDamping->setRange ( 0, REVERB_DAMPING_MAX );
    pSldReverbDamping->setTickInterval ( std::max ( 1, REVERB_DAMPING_MAX / 5 ) );
    pSldReverbDamping->setTickPosition ( QSlider::TicksBothSides );
    pSldReverbWet->setRange ( 0, REVERB_WET_MIX_MAX );
    pSldReverbWet->setTickInterval ( std::max ( 1, REVERB_WET_MIX_MAX / 5 ) );
    pSldReverbWet->setTickPosition ( QSlider::TicksBothSides );
    pSldReverbEarly->setRange ( 0, REVERB_EARLY_LEVEL_MAX );
    pSldReverbEarly->setTickInterval ( std::max ( 1, REVERB_EARLY_LEVEL_MAX / 5 ) );
    pSldReverbEarly->setTickPosition ( QSlider::TicksBothSides );
    pSldReverbWidth->setRange ( 0, REVERB_WIDTH_MAX );
    pSldReverbWidth->setTickInterval ( std::max ( 1, REVERB_WIDTH_MAX / 5 ) );
    pSldReverbWidth->setTickPosition ( QSlider::TicksBothSides );

    pLblHighPassValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblLowPassValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblHighPassValue->setMinimumWidth ( 36 );
    pLblLowPassValue->setMinimumWidth ( 36 );
    pSldHighPassCutoff->setRange ( 20, 1000 );
    pSldHighPassCutoff->setTickInterval ( std::max ( 1, ( 1000 - 20 ) / 5 ) );
    pSldHighPassCutoff->setTickPosition ( QSlider::TicksBothSides );
    pSldLowPassCutoff->setRange ( 1000, 20000 );
    pSldLowPassCutoff->setTickInterval ( std::max ( 1, ( 20000 - 1000 ) / 5 ) );
    pSldLowPassCutoff->setTickPosition ( QSlider::TicksBothSides );

    pLblCompressorThresholdValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblCompressorRatioValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblCompressorAttackValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblCompressorReleaseValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblCompressorMakeupValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblCompressorThresholdValue->setMinimumWidth ( 36 );
    pLblCompressorRatioValue->setMinimumWidth ( 36 );
    pLblCompressorAttackValue->setMinimumWidth ( 36 );
    pLblCompressorReleaseValue->setMinimumWidth ( 36 );
    pLblCompressorMakeupValue->setMinimumWidth ( 36 );
    pSldCompressorThreshold->setRange ( -60, 0 );
    pSldCompressorThreshold->setTickInterval ( std::max ( 1, 60 / 5 ) );
    pSldCompressorThreshold->setTickPosition ( QSlider::TicksBothSides );
    pSldCompressorRatio->setRange ( 1, 20 );
    pSldCompressorRatio->setTickInterval ( std::max ( 1, ( 20 - 1 ) / 5 ) );
    pSldCompressorRatio->setTickPosition ( QSlider::TicksBothSides );
    pSldCompressorAttack->setRange ( 1, 50 );
    pSldCompressorAttack->setTickInterval ( std::max ( 1, ( 50 - 1 ) / 5 ) );
    pSldCompressorAttack->setTickPosition ( QSlider::TicksBothSides );
    pSldCompressorRelease->setRange ( 10, 400 );
    pSldCompressorRelease->setTickInterval ( std::max ( 1, ( 400 - 10 ) / 5 ) );
    pSldCompressorRelease->setTickPosition ( QSlider::TicksBothSides );
    pSldCompressorMakeup->setRange ( 0, 24 );
    pSldCompressorMakeup->setTickInterval ( std::max ( 1, 24 / 5 ) );
    pSldCompressorMakeup->setTickPosition ( QSlider::TicksBothSides );

    pOutputBandMeter->setObjectName ( "pOutputBandMeter" );
    pOutputBandMeter->setMinimumHeight ( 72 );
    pOutputBandMeter->setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );

    QLabel* apFreqLabels[CAudioEqualizer::NUM_BANDS] = { pLblEQBandFreq0,
                                                         pLblEQBandFreq1,
                                                         pLblEQBandFreq2,
                                                         pLblEQBandFreq3,
                                                         pLblEQBandFreq4,
                                                         pLblEQBandFreq5,
                                                         pLblEQBandFreq6,
                                                         pLblEQBandFreq7,
                                                         pLblEQBandFreq8,
                                                         pLblEQBandFreq9,
                                                         pLblEQBandFreq10,
                                                         pLblEQBandFreq11,
                                                         pLblEQBandFreq12,
                                                         pLblEQBandFreq13,
                                                         pLblEQBandFreq14,
                                                         pLblEQBandFreq15 };

    CCustomSlider* apBandSliders[CAudioEqualizer::NUM_BANDS] = { pSldEQBand0,
                                                                 pSldEQBand1,
                                                                 pSldEQBand2,
                                                                 pSldEQBand3,
                                                                 pSldEQBand4,
                                                                 pSldEQBand5,
                                                                 pSldEQBand6,
                                                                 pSldEQBand7,
                                                                 pSldEQBand8,
                                                                 pSldEQBand9,
                                                                 pSldEQBand10,
                                                                 pSldEQBand11,
                                                                 pSldEQBand12,
                                                                 pSldEQBand13,
                                                                 pSldEQBand14,
                                                                 pSldEQBand15 };

    QLabel* apBandValues[CAudioEqualizer::NUM_BANDS] = { pLblEQBandValue0,
                                                         pLblEQBandValue1,
                                                         pLblEQBandValue2,
                                                         pLblEQBandValue3,
                                                         pLblEQBandValue4,
                                                         pLblEQBandValue5,
                                                         pLblEQBandValue6,
                                                         pLblEQBandValue7,
                                                         pLblEQBandValue8,
                                                         pLblEQBandValue9,
                                                         pLblEQBandValue10,
                                                         pLblEQBandValue11,
                                                         pLblEQBandValue12,
                                                         pLblEQBandValue13,
                                                         pLblEQBandValue14,
                                                         pLblEQBandValue15 };

    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pSldEQBands[iBand]      = apBandSliders[iBand];
        pLblEQBandValues[iBand] = apBandValues[iBand];

        pSldEQBands[iBand]->setRange ( -12, 12 );
        pSldEQBands[iBand]->setValue ( 0 );
        pSldEQBands[iBand]->setTickInterval ( std::max ( 1, 24 / 5 ) );
        pSldEQBands[iBand]->setTickPosition ( QSlider::TicksBothSides );
        pSldEQBands[iBand]->setMinimumHeight ( 100 );
        pSldEQBands[iBand]->setMinimumWidth ( 20 );
        pSldEQBands[iBand]->setMaximumWidth ( 24 );
        pSldEQBands[iBand]->setSizePolicy ( QSizePolicy::Fixed, QSizePolicy::Expanding );

        apFreqLabels[iBand]->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );
        apFreqLabels[iBand]->setObjectName ( QString ( "eqBandFreq%1" ).arg ( iBand ) );
        pLblEQBandValues[iBand]->setObjectName ( QString ( "eqBandValue%1" ).arg ( iBand ) );
        apFreqLabels[iBand]->setSizePolicy ( QSizePolicy::Preferred, QSizePolicy::Preferred );

        pLblEQBandValues[iBand]->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );
        pLblEQBandValues[iBand]->setMinimumWidth ( pSldEQBands[iBand]->minimumWidth() );
        pLblEQBandValues[iBand]->setMaximumWidth ( pSldEQBands[iBand]->maximumWidth() );

        QObject::connect ( pSldEQBands[iBand], &CCustomSlider::valueChanged, this, [this, iBand] ( int value ) {
            pLblEQBandValues[iBand]->setText ( QString::number ( value ) );
            emit EQBandGainChanged ( iBand, value );
        } );
    }

    bEQBandWidgetsReady = true;

    setMinimumHeight ( 420 );

    QObject::connect ( pSldReverb, &CCustomSlider::valueChanged, this, &CEffectsDlg::ReverbValueChanged );
    QObject::connect ( pChbReverbBypass, &QCheckBox::toggled, this, &CEffectsDlg::ReverbBypassChanged );
    QObject::connect ( pSldReverbPreDelay, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbPreDelayValue->setText ( QString::number ( value ) + tr ( " ms" ) );
        emit ReverbPreDelayChanged ( value );
    } );
    QObject::connect ( pSldReverbRoom, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbRoomValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbRoomSizeChanged ( value );
    } );
    QObject::connect ( pSldReverbDamping, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbDampingValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbDampingChanged ( value );
    } );
    QObject::connect ( pSldReverbWet, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbWetValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbWetMixChanged ( value );
    } );
    QObject::connect ( pSldReverbEarly, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbEarlyValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbEarlyLevelChanged ( value );
    } );
    QObject::connect ( pSldReverbWidth, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbWidthValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbWidthChanged ( value );
    } );
    QObject::connect ( pRbtReverbSelL, &QRadioButton::clicked, this, &CEffectsDlg::ReverbLeftSelected );
    QObject::connect ( pRbtReverbSelR, &QRadioButton::clicked, this, &CEffectsDlg::ReverbRightSelected );
    QObject::connect ( pChbReverbEarly, &QCheckBox::toggled, this, [this] ( bool enabled ) {
        pSldReverbEarly->setEnabled ( enabled );
        emit ReverbEarlyEnabledChanged ( enabled );
    } );
    QObject::connect ( pChbReverbFreeze, &QCheckBox::toggled, this, &CEffectsDlg::ReverbFreezeChanged );
    QObject::connect ( pButReverbReset, &QPushButton::clicked, this, &CEffectsDlg::OnResetReverbClicked );
    QObject::connect ( pChbFilterBypass, &QCheckBox::toggled, this, [this] ( bool bypassed ) {
        emit FilterBypassChanged ( bypassed );
        UpdateFilterControls();
    } );
    QObject::connect ( pChbHighPass, &QCheckBox::toggled, this, [this] ( bool enabled ) {
        pSldHighPassCutoff->setEnabled ( enabled );
        emit HighPassEnabledChanged ( enabled );
    } );
    QObject::connect ( pChbLowPass, &QCheckBox::toggled, this, [this] ( bool enabled ) {
        pSldLowPassCutoff->setEnabled ( enabled );
        emit LowPassEnabledChanged ( enabled );
    } );
    QObject::connect ( pSldHighPassCutoff, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblHighPassValue->setText ( QString::number ( value ) + tr ( " Hz" ) );
        emit HighPassCutoffChanged ( value );
    } );
    QObject::connect ( pSldLowPassCutoff, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblLowPassValue->setText ( QString::number ( value ) + tr ( " Hz" ) );
        emit LowPassCutoffChanged ( value );
    } );
    QObject::connect ( pButFilterReset, &QPushButton::clicked, this, &CEffectsDlg::OnResetFilterClicked );
    QObject::connect ( pChbCompressorBypass, &QCheckBox::toggled, this, &CEffectsDlg::CompressorBypassChanged );
    QObject::connect ( pChbCompressorLimiter, &QCheckBox::toggled, this, &CEffectsDlg::CompressorLimiterChanged );
    QObject::connect ( pSldCompressorThreshold, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblCompressorThresholdValue->setText ( QString::number ( value ) + tr ( " dB" ) );
        emit CompressorThresholdChanged ( value );
    } );
    QObject::connect ( pSldCompressorRatio, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblCompressorRatioValue->setText ( QString::number ( value ) + tr ( ":1" ) );
        emit CompressorRatioChanged ( value );
    } );
    QObject::connect ( pSldCompressorAttack, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblCompressorAttackValue->setText ( QString::number ( value ) + tr ( " ms" ) );
        emit CompressorAttackChanged ( value );
    } );
    QObject::connect ( pSldCompressorRelease, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblCompressorReleaseValue->setText ( QString::number ( value ) + tr ( " ms" ) );
        emit CompressorReleaseChanged ( value );
    } );
    QObject::connect ( pSldCompressorMakeup, &CCustomSlider::valueChanged, this, [this] ( int value ) {
        pLblCompressorMakeupValue->setText ( QString::number ( value ) + tr ( " dB" ) );
        emit CompressorMakeupChanged ( value );
    } );
    QObject::connect ( pButCompressorReset, &QPushButton::clicked, this, &CEffectsDlg::OnResetCompressorClicked );
    QObject::connect ( pChbEQBypass, &QCheckBox::toggled, this, &CEffectsDlg::EQBypassChanged );
    QObject::connect ( pCbxEffectsPresets,
                       QOverload<int>::of ( &QComboBox::currentIndexChanged ),
                       this,
                       &CEffectsDlg::ApplyEffectsPresetFromComboIndex );
    QObject::connect ( pButEffectsSavePreset, &QPushButton::clicked, this, &CEffectsDlg::OnSaveEffectsPresetClicked );
    QObject::connect ( pButEffectsSaveAsPreset, &QPushButton::clicked, this, &CEffectsDlg::OnSaveAsEffectsPresetClicked );
    QObject::connect ( pButEffectsDeletePreset, &QPushButton::clicked, this, &CEffectsDlg::OnDeleteEffectsPresetClicked );
    QObject::connect ( pCbxEQPresets, QOverload<int>::of ( &QComboBox::currentIndexChanged ), this, &CEffectsDlg::ApplyPresetFromComboIndex );
    QObject::connect ( pButEQReset, &QPushButton::clicked, this, &CEffectsDlg::OnResetEQClicked );
    QObject::connect ( pButEQSavePreset, &QPushButton::clicked, this, &CEffectsDlg::OnSaveEQPresetClicked );
    QObject::connect ( pButEQSaveAsPreset, &QPushButton::clicked, this, &CEffectsDlg::OnSaveAsEQPresetClicked );
    QObject::connect ( pButEQDeletePreset, &QPushButton::clicked, this, &CEffectsDlg::OnDeleteEQPresetClicked );

    UpdateReverbControls();
    UpdateFilterControls();
    UpdateCompressorControls();
    UpdateEQReadouts();
    PopulateEffectsPresetCombo();
    PopulateEQPresetCombo();
    UpdateEQPresetSelection();
}

void CEffectsDlg::UpdateOutputBandLevels ( const CVector<float>& vecOutLevels )
{
    if ( pOutputBandMeterSafe )
    {
        pOutputBandMeterSafe->SetLevels ( vecOutLevels );
    }
}

void CEffectsDlg::showEvent ( QShowEvent* Event )
{
    ApplyThemeToCustomWidgets();

    // Restore last used tab if valid (do this first)
    if ( pSettings->iEffectsTab >= 0 && pSettings->iEffectsTab < pTabs->count() )
    {
        pTabs->setCurrentIndex ( pSettings->iEffectsTab );
    }
    PopulateEffectsPresetCombo();
    UpdateReverbControls();
    UpdateFilterControls();
    UpdateCompressorControls();
    UpdateEQControls();
    UpdateEQPresetSelection();
    CBaseDlg::showEvent ( Event );
}

void CEffectsDlg::OnUIThemeChanged() { ApplyThemeToCustomWidgets(); }

void CEffectsDlg::ApplyThemeToCustomWidgets()
{
    const bool bDarkTheme = ( ResolveUITheme ( pSettings->eUITheme ) == UIT_DARK );

    CCustomSlider* apThemeSliders[] = {
        pSldReverb,
        pSldReverbPreDelay,
        pSldReverbRoom,
        pSldReverbDamping,
        pSldReverbWet,
        pSldReverbEarly,
        pSldReverbWidth,
        pSldHighPassCutoff,
        pSldLowPassCutoff,
        pSldCompressorThreshold,
        pSldCompressorRatio,
        pSldCompressorAttack,
        pSldCompressorRelease,
        pSldCompressorMakeup,
    };

    for ( CCustomSlider* pSlider : apThemeSliders )
    {
        if ( pSlider )
        {
            pSlider->SetDarkTheme ( bDarkTheme );
        }
    }

    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        if ( pSldEQBands[iBand] )
        {
            pSldEQBands[iBand]->SetDarkTheme ( bDarkTheme );
        }
    }

    if ( pOutputBandMeterSafe )
    {
        pOutputBandMeterSafe->SetDarkTheme ( bDarkTheme );
    }
}

void CEffectsDlg::resizeEvent ( QResizeEvent* Event )
{
    CBaseDlg::resizeEvent ( Event );
    UpdateOutputBandAlignment();
}

void CEffectsDlg::UpdateOutputBandAlignment()
{
    // Direct mapping: each meter bar is centered under its slider
    if ( bEQBandWidgetsReady && pOutputBandMeterSafe )
    {
        QVector<int> bandCenters;
        bandCenters.reserve ( CAudioEqualizer::NUM_BANDS );
        for ( int i = 0; i < CAudioEqualizer::NUM_BANDS; ++i )
        {
            CCustomSlider* pSlider = pSldEQBands[i];
            if ( !pSlider || !pSlider->isVisible() )
            {
                bandCenters.append ( -1 );
                continue;
            }
            QPoint sliderCenter = pSlider->rect().center();
            QPoint globalCenter = pSlider->mapToGlobal ( sliderCenter );
            QPoint meterLocal   = pOutputBandMeterSafe->mapFromGlobal ( globalCenter );
            bandCenters.append ( meterLocal.x() );
        }
        pOutputBandMeterSafe->SetBandCenters ( bandCenters );
    }
}

void CEffectsDlg::UpdateFilterControls()
{
    pChbFilterBypass->blockSignals ( true );
    pChbFilterBypass->setChecked ( pClient->GetFilterBypass() );
    pChbFilterBypass->blockSignals ( false );

    pChbHighPass->blockSignals ( true );
    pChbHighPass->setChecked ( pClient->GetHighPassEnabled() );
    pChbHighPass->blockSignals ( false );

    pChbLowPass->blockSignals ( true );
    pChbLowPass->setChecked ( pClient->GetLowPassEnabled() );
    pChbLowPass->blockSignals ( false );

    pSldHighPassCutoff->blockSignals ( true );
    pSldHighPassCutoff->setValue ( pClient->GetHighPassCutoffHz() );
    pSldHighPassCutoff->blockSignals ( false );

    pSldLowPassCutoff->blockSignals ( true );
    pSldLowPassCutoff->setValue ( pClient->GetLowPassCutoffHz() );
    pSldLowPassCutoff->blockSignals ( false );

    const bool bBypass = pClient->GetFilterBypass();
    (void) bBypass;
    pChbHighPass->setEnabled ( true );
    pChbLowPass->setEnabled ( true );
    pSldHighPassCutoff->setEnabled ( pClient->GetHighPassEnabled() );
    pSldLowPassCutoff->setEnabled ( pClient->GetLowPassEnabled() );

    pLblHighPassValue->setText ( QString::number ( pSldHighPassCutoff->value() ) + tr ( " Hz" ) );
    pLblLowPassValue->setText ( QString::number ( pSldLowPassCutoff->value() ) + tr ( " Hz" ) );
}

void CEffectsDlg::UpdateCompressorControls()
{
    pChbCompressorBypass->blockSignals ( true );
    pChbCompressorBypass->setChecked ( pClient->GetCompressorBypass() );
    pChbCompressorBypass->blockSignals ( false );

    pChbCompressorLimiter->blockSignals ( true );
    pChbCompressorLimiter->setChecked ( pClient->GetCompressorLimiterEnabled() );
    pChbCompressorLimiter->blockSignals ( false );

    pSldCompressorThreshold->blockSignals ( true );
    pSldCompressorThreshold->setValue ( static_cast<int> ( pClient->GetCompressorThresholdDb() ) );
    pSldCompressorThreshold->blockSignals ( false );

    pSldCompressorRatio->blockSignals ( true );
    pSldCompressorRatio->setValue ( static_cast<int> ( pClient->GetCompressorRatio() ) );
    pSldCompressorRatio->blockSignals ( false );

    pSldCompressorAttack->blockSignals ( true );
    pSldCompressorAttack->setValue ( static_cast<int> ( pClient->GetCompressorAttackMs() ) );
    pSldCompressorAttack->blockSignals ( false );

    pSldCompressorRelease->blockSignals ( true );
    pSldCompressorRelease->setValue ( static_cast<int> ( pClient->GetCompressorReleaseMs() ) );
    pSldCompressorRelease->blockSignals ( false );

    pSldCompressorMakeup->blockSignals ( true );
    pSldCompressorMakeup->setValue ( static_cast<int> ( pClient->GetCompressorMakeupDb() ) );
    pSldCompressorMakeup->blockSignals ( false );

    const bool bBypass = pClient->GetCompressorBypass();
    (void) bBypass;
    pSldCompressorThreshold->setEnabled ( true );
    pSldCompressorRatio->setEnabled ( true );
    pSldCompressorAttack->setEnabled ( true );
    pSldCompressorRelease->setEnabled ( true );
    pSldCompressorMakeup->setEnabled ( true );
    pChbCompressorLimiter->setEnabled ( true );

    pLblCompressorThresholdValue->setText ( QString::number ( pSldCompressorThreshold->value() ) + tr ( " dB" ) );
    pLblCompressorRatioValue->setText ( QString::number ( pSldCompressorRatio->value() ) + tr ( ":1" ) );
    pLblCompressorAttackValue->setText ( QString::number ( pSldCompressorAttack->value() ) + tr ( " ms" ) );
    pLblCompressorReleaseValue->setText ( QString::number ( pSldCompressorRelease->value() ) + tr ( " ms" ) );
    pLblCompressorMakeupValue->setText ( QString::number ( pSldCompressorMakeup->value() ) + tr ( " dB" ) );
}

void CEffectsDlg::UpdateReverbControls()
{
    pSldReverb->blockSignals ( true );
    pSldReverb->setValue ( pClient->GetReverbLevel() );
    pSldReverb->blockSignals ( false );

    pSldReverbPreDelay->blockSignals ( true );
    pSldReverbPreDelay->setValue ( pClient->GetReverbPreDelayMs() );
    pSldReverbPreDelay->blockSignals ( false );

    pSldReverbRoom->blockSignals ( true );
    pSldReverbRoom->setValue ( pClient->GetReverbRoomSize() );
    pSldReverbRoom->blockSignals ( false );

    pSldReverbDamping->blockSignals ( true );
    pSldReverbDamping->setValue ( pClient->GetReverbDamping() );
    pSldReverbDamping->blockSignals ( false );

    pSldReverbWet->blockSignals ( true );
    pSldReverbWet->setValue ( pClient->GetReverbWetMix() );
    pSldReverbWet->blockSignals ( false );

    pSldReverbEarly->blockSignals ( true );
    pSldReverbEarly->setValue ( pClient->GetReverbEarlyLevel() );
    pSldReverbEarly->blockSignals ( false );

    pSldReverbWidth->blockSignals ( true );
    pSldReverbWidth->setValue ( pClient->GetReverbWidth() );
    pSldReverbWidth->blockSignals ( false );

    pChbReverbBypass->blockSignals ( true );
    pChbReverbBypass->setChecked ( pClient->GetReverbBypass() );
    pChbReverbBypass->blockSignals ( false );

    pChbReverbEarly->blockSignals ( true );
    pChbReverbEarly->setChecked ( pClient->GetReverbEarlyEnabled() );
    pChbReverbEarly->blockSignals ( false );
    pSldReverbEarly->setEnabled ( pClient->GetReverbEarlyEnabled() );

    pChbReverbFreeze->blockSignals ( true );
    pChbReverbFreeze->setChecked ( pClient->GetReverbFreeze() );
    pChbReverbFreeze->blockSignals ( false );

    const bool bBypass = pClient->GetReverbBypass();
    (void) bBypass;
    pSldReverb->setEnabled ( true );
    pSldReverbPreDelay->setEnabled ( true );
    pSldReverbRoom->setEnabled ( true );
    pSldReverbDamping->setEnabled ( true );
    pSldReverbWet->setEnabled ( true );
    pSldReverbEarly->setEnabled ( pClient->GetReverbEarlyEnabled() );
    pSldReverbWidth->setEnabled ( true );
    pChbReverbEarly->setEnabled ( true );
    pChbReverbFreeze->setEnabled ( true );
    pRbtReverbSelL->setEnabled ( true );
    pRbtReverbSelR->setEnabled ( true );

    pLblReverbPreDelayValue->setText ( QString::number ( pSldReverbPreDelay->value() ) + tr ( " ms" ) );
    pLblReverbRoomValue->setText ( QString::number ( pSldReverbRoom->value() ) + tr ( " %" ) );
    pLblReverbDampingValue->setText ( QString::number ( pSldReverbDamping->value() ) + tr ( " %" ) );
    pLblReverbWetValue->setText ( QString::number ( pSldReverbWet->value() ) + tr ( " %" ) );
    pLblReverbEarlyValue->setText ( QString::number ( pSldReverbEarly->value() ) + tr ( " %" ) );
    pLblReverbWidthValue->setText ( QString::number ( pSldReverbWidth->value() ) + tr ( " %" ) );

    const bool bShowChannelSelection = pClient->GetAudioChannels() != CC_STEREO;

    pRbtReverbSelL->setVisible ( bShowChannelSelection );
    pRbtReverbSelR->setVisible ( bShowChannelSelection );
    pLblStereoHint->setVisible ( !bShowChannelSelection );

    if ( bShowChannelSelection )
    {
        if ( pClient->IsReverbOnLeftChan() )
        {
            pRbtReverbSelL->setChecked ( true );
        }
        else
        {
            pRbtReverbSelR->setChecked ( true );
        }
    }
}

void CEffectsDlg::UpdateEQReadouts()
{
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pLblEQBandValues[iBand]->setText ( QString::number ( pSldEQBands[iBand]->value() ) );
    }
}

void CEffectsDlg::UpdateEQControls()
{
    pChbEQBypass->blockSignals ( true );
    pChbEQBypass->setChecked ( pClient->GetEQBypass() );
    pChbEQBypass->blockSignals ( false );

    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pSldEQBands[iBand]->blockSignals ( true );
        pSldEQBands[iBand]->setValue ( pClient->GetEQBandGainDb ( iBand ) );
        pSldEQBands[iBand]->blockSignals ( false );
    }

    UpdateEQReadouts();
}

void CEffectsDlg::PopulateEffectsPresetCombo()
{
    const bool bBlocked = pCbxEffectsPresets->blockSignals ( true );
    pCbxEffectsPresets->clear();

    for ( int iPreset = 0; iPreset < MAX_NUM_EFFECT_PRESETS; ++iPreset )
    {
        const QString strName = pSettings->vstrEffectsPresetNames[iPreset];
        if ( strName.isEmpty() )
        {
            continue;
        }

        pCbxEffectsPresets->addItem ( strName );
        pCbxEffectsPresets->setItemData ( pCbxEffectsPresets->count() - 1, iPreset, Qt::UserRole );
    }

    pCbxEffectsPresets->blockSignals ( bBlocked );
}

void CEffectsDlg::ApplyEffectsPresetFromComboIndex ( const int iPresetIndex )
{
    if ( iPresetIndex < 0 )
    {
        return;
    }

    const int iPresetSlot = pCbxEffectsPresets->itemData ( iPresetIndex, Qt::UserRole ).toInt();
    ApplyEffectsPresetFromSlot ( iPresetSlot );
}

void CEffectsDlg::ApplyEffectsPresetFromSlot ( const int iPresetSlot )
{
    if ( iPresetSlot < 0 || iPresetSlot >= MAX_NUM_EFFECT_PRESETS )
    {
        return;
    }

    pClient->SetReverbLevel ( pSettings->iEffectsPresetReverbLevel[iPresetSlot] );
    pClient->SetReverbOnLeftChan ( pSettings->bEffectsPresetReverbOnLeftChan[iPresetSlot] );
    pClient->SetReverbPreDelayMs ( pSettings->iEffectsPresetReverbPreDelayMs[iPresetSlot] );
    pClient->SetReverbRoomSize ( pSettings->iEffectsPresetReverbRoomSize[iPresetSlot] );
    pClient->SetReverbDamping ( pSettings->iEffectsPresetReverbDamping[iPresetSlot] );
    pClient->SetReverbWetMix ( pSettings->iEffectsPresetReverbWetMix[iPresetSlot] );
    pClient->SetReverbEarlyLevel ( pSettings->iEffectsPresetReverbEarlyLevel[iPresetSlot] );
    pClient->SetReverbWidth ( pSettings->iEffectsPresetReverbWidth[iPresetSlot] );
    pClient->SetReverbEarlyEnabled ( pSettings->bEffectsPresetReverbEarlyEnabled[iPresetSlot] );
    pClient->SetReverbFreeze ( pSettings->bEffectsPresetReverbFreeze[iPresetSlot] );
    pClient->SetReverbBypass ( pSettings->bEffectsPresetReverbBypass[iPresetSlot] );

    pClient->SetCompressorBypass ( pSettings->bEffectsPresetCompressorBypass[iPresetSlot] );
    pClient->SetCompressorThresholdDb ( static_cast<float> ( pSettings->iEffectsPresetCompressorThresholdDb[iPresetSlot] ) );
    pClient->SetCompressorRatio ( static_cast<float> ( pSettings->iEffectsPresetCompressorRatio[iPresetSlot] ) );
    pClient->SetCompressorAttackMs ( static_cast<float> ( pSettings->iEffectsPresetCompressorAttackMs[iPresetSlot] ) );
    pClient->SetCompressorReleaseMs ( static_cast<float> ( pSettings->iEffectsPresetCompressorReleaseMs[iPresetSlot] ) );
    pClient->SetCompressorMakeupDb ( static_cast<float> ( pSettings->iEffectsPresetCompressorMakeupDb[iPresetSlot] ) );
    pClient->SetCompressorLimiterEnabled ( pSettings->bEffectsPresetCompressorLimiterEnabled[iPresetSlot] );

    pClient->SetFilterBypass ( pSettings->bEffectsPresetFilterBypass[iPresetSlot] );
    pClient->SetHighPassEnabled ( pSettings->bEffectsPresetHighPassEnabled[iPresetSlot] );
    pClient->SetLowPassEnabled ( pSettings->bEffectsPresetLowPassEnabled[iPresetSlot] );
    pClient->SetHighPassCutoffHz ( pSettings->iEffectsPresetHighPassCutoffHz[iPresetSlot] );
    pClient->SetLowPassCutoffHz ( pSettings->iEffectsPresetLowPassCutoffHz[iPresetSlot] );

    pClient->SetEQBypass ( pSettings->bEffectsPresetEQBypass[iPresetSlot] );
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pClient->SetEQBandGainDb ( iBand, pSettings->aiEffectsPresetEQBandGainDb[iPresetSlot][iBand] );
    }

    UpdateReverbControls();
    UpdateFilterControls();
    UpdateCompressorControls();
    UpdateEQControls();
    UpdateEQPresetSelection();
}

int CEffectsDlg::FindEffectsPresetSlotByName ( const QString& strName ) const
{
    for ( int iPreset = 0; iPreset < MAX_NUM_EFFECT_PRESETS; ++iPreset )
    {
        if ( pSettings->vstrEffectsPresetNames[iPreset].compare ( strName, Qt::CaseInsensitive ) == 0 )
        {
            return iPreset;
        }
    }

    return INVALID_INDEX;
}

int CEffectsDlg::FindFreeEffectsPresetSlot() const
{
    for ( int iPreset = 0; iPreset < MAX_NUM_EFFECT_PRESETS; ++iPreset )
    {
        if ( pSettings->vstrEffectsPresetNames[iPreset].isEmpty() )
        {
            return iPreset;
        }
    }

    return INVALID_INDEX;
}

void CEffectsDlg::OnSaveEffectsPresetClicked()
{
    const int iComboIndex = pCbxEffectsPresets->currentIndex();
    if ( iComboIndex < 0 )
    {
        return;
    }

    const int iPresetSlot = pCbxEffectsPresets->itemData ( iComboIndex, Qt::UserRole ).toInt();
    if ( iPresetSlot < 0 || iPresetSlot >= MAX_NUM_EFFECT_PRESETS )
    {
        return;
    }

    const QString strName                          = pCbxEffectsPresets->itemText ( iComboIndex );
    pSettings->vstrEffectsPresetNames[iPresetSlot] = strName;

    pSettings->bEffectsPresetEQBypass[iPresetSlot] = pClient->GetEQBypass();
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pSettings->aiEffectsPresetEQBandGainDb[iPresetSlot][iBand] = pClient->GetEQBandGainDb ( iBand );
    }

    pSettings->iEffectsPresetReverbLevel[iPresetSlot]        = pClient->GetReverbLevel();
    pSettings->bEffectsPresetReverbOnLeftChan[iPresetSlot]   = pClient->IsReverbOnLeftChan();
    pSettings->iEffectsPresetReverbPreDelayMs[iPresetSlot]   = pClient->GetReverbPreDelayMs();
    pSettings->iEffectsPresetReverbRoomSize[iPresetSlot]     = pClient->GetReverbRoomSize();
    pSettings->iEffectsPresetReverbDamping[iPresetSlot]      = pClient->GetReverbDamping();
    pSettings->iEffectsPresetReverbWetMix[iPresetSlot]       = pClient->GetReverbWetMix();
    pSettings->iEffectsPresetReverbEarlyLevel[iPresetSlot]   = pClient->GetReverbEarlyLevel();
    pSettings->iEffectsPresetReverbWidth[iPresetSlot]        = pClient->GetReverbWidth();
    pSettings->bEffectsPresetReverbEarlyEnabled[iPresetSlot] = pClient->GetReverbEarlyEnabled();
    pSettings->bEffectsPresetReverbFreeze[iPresetSlot]       = pClient->GetReverbFreeze();
    pSettings->bEffectsPresetReverbBypass[iPresetSlot]       = pClient->GetReverbBypass();

    pSettings->bEffectsPresetCompressorBypass[iPresetSlot]         = pClient->GetCompressorBypass();
    pSettings->iEffectsPresetCompressorThresholdDb[iPresetSlot]    = static_cast<int> ( pClient->GetCompressorThresholdDb() );
    pSettings->iEffectsPresetCompressorRatio[iPresetSlot]          = static_cast<int> ( pClient->GetCompressorRatio() );
    pSettings->iEffectsPresetCompressorAttackMs[iPresetSlot]       = static_cast<int> ( pClient->GetCompressorAttackMs() );
    pSettings->iEffectsPresetCompressorReleaseMs[iPresetSlot]      = static_cast<int> ( pClient->GetCompressorReleaseMs() );
    pSettings->iEffectsPresetCompressorMakeupDb[iPresetSlot]       = static_cast<int> ( pClient->GetCompressorMakeupDb() );
    pSettings->bEffectsPresetCompressorLimiterEnabled[iPresetSlot] = pClient->GetCompressorLimiterEnabled();

    pSettings->bEffectsPresetFilterBypass[iPresetSlot]     = pClient->GetFilterBypass();
    pSettings->bEffectsPresetHighPassEnabled[iPresetSlot]  = pClient->GetHighPassEnabled();
    pSettings->bEffectsPresetLowPassEnabled[iPresetSlot]   = pClient->GetLowPassEnabled();
    pSettings->iEffectsPresetHighPassCutoffHz[iPresetSlot] = pClient->GetHighPassCutoffHz();
    pSettings->iEffectsPresetLowPassCutoffHz[iPresetSlot]  = pClient->GetLowPassCutoffHz();

    PopulateEffectsPresetCombo();
    const int iUpdatedIndex = pCbxEffectsPresets->findText ( strName );
    if ( iUpdatedIndex >= 0 )
    {
        pCbxEffectsPresets->setCurrentIndex ( iUpdatedIndex );
    }
}

void CEffectsDlg::OnResetReverbClicked()
{
    pClient->SetReverbLevel ( 0 );
    pClient->SetReverbOnLeftChan ( false );
    pClient->SetReverbPreDelayMs ( 0 );
    pClient->SetReverbRoomSize ( 60 );
    pClient->SetReverbDamping ( 30 );
    pClient->SetReverbWetMix ( 25 );
    pClient->SetReverbEarlyLevel ( 30 );
    pClient->SetReverbWidth ( 100 );
    pClient->SetReverbEarlyEnabled ( true );
    pClient->SetReverbFreeze ( false );
    pClient->SetReverbBypass ( true );

    UpdateReverbControls();
    // Remove focus from the button to prevent blue outline
    pTabs->setFocus();
}

void CEffectsDlg::OnResetFilterClicked()
{
    pClient->SetFilterBypass ( true );
    pClient->SetHighPassEnabled ( false );
    pClient->SetLowPassEnabled ( false );
    pClient->SetHighPassCutoffHz ( 80 );
    pClient->SetLowPassCutoffHz ( 12000 );

    UpdateFilterControls();
    // Remove focus from the button to prevent blue outline
    pTabs->setFocus();
}

void CEffectsDlg::OnResetCompressorClicked()
{
    pClient->SetCompressorBypass ( true );
    pClient->SetCompressorThresholdDb ( -12.0f );
    pClient->SetCompressorRatio ( 3.0f );
    pClient->SetCompressorAttackMs ( 5.0f );
    pClient->SetCompressorReleaseMs ( 120.0f );
    pClient->SetCompressorMakeupDb ( 3.0f );
    pClient->SetCompressorLimiterEnabled ( true );

    UpdateCompressorControls();
    // Remove focus from the button to prevent blue outline
    pTabs->setFocus();
}

void CEffectsDlg::OnSaveAsEffectsPresetClicked()
{
    bool    bOk     = false;
    QString strName = QInputDialog::getText ( this, tr ( "Save Effects Preset" ), tr ( "Preset name:" ), QLineEdit::Normal, "", &bOk ).trimmed();

    if ( !bOk || strName.isEmpty() )
    {
        return;
    }

    int iPresetSlot = FindEffectsPresetSlotByName ( strName );
    if ( iPresetSlot == INVALID_INDEX )
    {
        iPresetSlot = FindFreeEffectsPresetSlot();
    }

    if ( iPresetSlot == INVALID_INDEX )
    {
        QMessageBox::warning ( this, tr ( "Preset Limit Reached" ), tr ( "No free preset slot is available. Delete an existing preset first." ) );
        return;
    }

    pSettings->vstrEffectsPresetNames[iPresetSlot] = strName;
    pSettings->bEffectsPresetEQBypass[iPresetSlot] = pClient->GetEQBypass();
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pSettings->aiEffectsPresetEQBandGainDb[iPresetSlot][iBand] = pClient->GetEQBandGainDb ( iBand );
    }

    pSettings->iEffectsPresetReverbLevel[iPresetSlot]        = pClient->GetReverbLevel();
    pSettings->bEffectsPresetReverbOnLeftChan[iPresetSlot]   = pClient->IsReverbOnLeftChan();
    pSettings->iEffectsPresetReverbPreDelayMs[iPresetSlot]   = pClient->GetReverbPreDelayMs();
    pSettings->iEffectsPresetReverbRoomSize[iPresetSlot]     = pClient->GetReverbRoomSize();
    pSettings->iEffectsPresetReverbDamping[iPresetSlot]      = pClient->GetReverbDamping();
    pSettings->iEffectsPresetReverbWetMix[iPresetSlot]       = pClient->GetReverbWetMix();
    pSettings->iEffectsPresetReverbEarlyLevel[iPresetSlot]   = pClient->GetReverbEarlyLevel();
    pSettings->iEffectsPresetReverbWidth[iPresetSlot]        = pClient->GetReverbWidth();
    pSettings->bEffectsPresetReverbEarlyEnabled[iPresetSlot] = pClient->GetReverbEarlyEnabled();
    pSettings->bEffectsPresetReverbFreeze[iPresetSlot]       = pClient->GetReverbFreeze();
    pSettings->bEffectsPresetReverbBypass[iPresetSlot]       = pClient->GetReverbBypass();

    pSettings->bEffectsPresetCompressorBypass[iPresetSlot]         = pClient->GetCompressorBypass();
    pSettings->iEffectsPresetCompressorThresholdDb[iPresetSlot]    = static_cast<int> ( pClient->GetCompressorThresholdDb() );
    pSettings->iEffectsPresetCompressorRatio[iPresetSlot]          = static_cast<int> ( pClient->GetCompressorRatio() );
    pSettings->iEffectsPresetCompressorAttackMs[iPresetSlot]       = static_cast<int> ( pClient->GetCompressorAttackMs() );
    pSettings->iEffectsPresetCompressorReleaseMs[iPresetSlot]      = static_cast<int> ( pClient->GetCompressorReleaseMs() );
    pSettings->iEffectsPresetCompressorMakeupDb[iPresetSlot]       = static_cast<int> ( pClient->GetCompressorMakeupDb() );
    pSettings->bEffectsPresetCompressorLimiterEnabled[iPresetSlot] = pClient->GetCompressorLimiterEnabled();

    pSettings->bEffectsPresetFilterBypass[iPresetSlot]     = pClient->GetFilterBypass();
    pSettings->bEffectsPresetHighPassEnabled[iPresetSlot]  = pClient->GetHighPassEnabled();
    pSettings->bEffectsPresetLowPassEnabled[iPresetSlot]   = pClient->GetLowPassEnabled();
    pSettings->iEffectsPresetHighPassCutoffHz[iPresetSlot] = pClient->GetHighPassCutoffHz();
    pSettings->iEffectsPresetLowPassCutoffHz[iPresetSlot]  = pClient->GetLowPassCutoffHz();

    PopulateEffectsPresetCombo();
    const int iSavedIndex = pCbxEffectsPresets->findText ( strName );
    if ( iSavedIndex >= 0 )
    {
        pCbxEffectsPresets->setCurrentIndex ( iSavedIndex );
    }
}

void CEffectsDlg::OnDeleteEffectsPresetClicked()
{
    const int iPresetSlot = pCbxEffectsPresets->currentData ( Qt::UserRole ).toInt();
    if ( iPresetSlot < 0 || iPresetSlot >= MAX_NUM_EFFECT_PRESETS )
    {
        return;
    }

    pSettings->vstrEffectsPresetNames[iPresetSlot].clear();
    pSettings->bEffectsPresetEQBypass[iPresetSlot] = true;
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pSettings->aiEffectsPresetEQBandGainDb[iPresetSlot][iBand] = 0;
    }

    pSettings->iEffectsPresetReverbLevel[iPresetSlot]        = 0;
    pSettings->iEffectsPresetReverbPreDelayMs[iPresetSlot]   = 0;
    pSettings->iEffectsPresetReverbRoomSize[iPresetSlot]     = 60;
    pSettings->iEffectsPresetReverbDamping[iPresetSlot]      = 30;
    pSettings->iEffectsPresetReverbWetMix[iPresetSlot]       = 25;
    pSettings->iEffectsPresetReverbEarlyLevel[iPresetSlot]   = 30;
    pSettings->iEffectsPresetReverbWidth[iPresetSlot]        = 100;
    pSettings->bEffectsPresetReverbEarlyEnabled[iPresetSlot] = true;
    pSettings->bEffectsPresetReverbFreeze[iPresetSlot]       = false;
    pSettings->bEffectsPresetReverbBypass[iPresetSlot]       = true;
    pSettings->bEffectsPresetReverbOnLeftChan[iPresetSlot]   = false;

    pSettings->bEffectsPresetCompressorBypass[iPresetSlot]         = true;
    pSettings->iEffectsPresetCompressorThresholdDb[iPresetSlot]    = -12;
    pSettings->iEffectsPresetCompressorRatio[iPresetSlot]          = 3;
    pSettings->iEffectsPresetCompressorAttackMs[iPresetSlot]       = 5;
    pSettings->iEffectsPresetCompressorReleaseMs[iPresetSlot]      = 120;
    pSettings->iEffectsPresetCompressorMakeupDb[iPresetSlot]       = 3;
    pSettings->bEffectsPresetCompressorLimiterEnabled[iPresetSlot] = true;

    pSettings->bEffectsPresetFilterBypass[iPresetSlot]     = true;
    pSettings->bEffectsPresetHighPassEnabled[iPresetSlot]  = false;
    pSettings->bEffectsPresetLowPassEnabled[iPresetSlot]   = false;
    pSettings->iEffectsPresetHighPassCutoffHz[iPresetSlot] = 80;
    pSettings->iEffectsPresetLowPassCutoffHz[iPresetSlot]  = 12000;

    PopulateEffectsPresetCombo();
}

void CEffectsDlg::PopulateEQPresetCombo()
{
    pCbxEQPresets->clear();

    for ( int iPreset = 0; iPreset < MAX_NUM_EQ_USER_PRESETS; ++iPreset )
    {
        const QString strName = pSettings->vstrEQPresetNames[iPreset];
        if ( strName.isEmpty() )
        {
            continue;
        }

        QString strGains;
        for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
        {
            if ( !strGains.isEmpty() )
            {
                strGains += ",";
            }
            strGains += QString::number ( pSettings->aiEQPresetBandGainDb[iPreset][iBand] );
        }

        pCbxEQPresets->addItem ( strName );
        pCbxEQPresets->setItemData ( pCbxEQPresets->count() - 1, strGains, Qt::UserRole );
        pCbxEQPresets->setItemData ( pCbxEQPresets->count() - 1, iPreset, Qt::UserRole + 1 );
    }
}

void CEffectsDlg::UpdateEQPresetSelection()
{
    if ( pCbxEQPresets->count() == 0 )
    {
        return;
    }

    QString currentGains;
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        if ( !currentGains.isEmpty() )
        {
            currentGains += ",";
        }
        currentGains += QString::number ( pSldEQBands[iBand]->value() );
    }

    int matchedIndex = -1;
    for ( int iPreset = 0; iPreset < pCbxEQPresets->count(); ++iPreset )
    {
        const QString presetGains = pCbxEQPresets->itemData ( iPreset, Qt::UserRole ).toString();
        if ( presetGains == currentGains )
        {
            matchedIndex = iPreset;
            break;
        }
    }

    if ( matchedIndex >= 0 )
    {
        pCbxEQPresets->blockSignals ( true );
        pCbxEQPresets->setCurrentIndex ( matchedIndex );
        pCbxEQPresets->blockSignals ( false );
    }
}

void CEffectsDlg::ApplyPresetFromComboIndex ( const int iPresetIndex )
{
    if ( iPresetIndex < 0 )
    {
        return;
    }

    const QString     strGains = pCbxEQPresets->itemData ( iPresetIndex, Qt::UserRole ).toString();
    const QStringList gains    = strGains.split ( "," );

    if ( gains.size() != CAudioEqualizer::NUM_BANDS )
    {
        return;
    }

    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pSldEQBands[iBand]->setValue ( gains[iBand].toInt() );
    }
}

int CEffectsDlg::FindPresetSlotByName ( const QString& strName ) const
{
    for ( int iPreset = 0; iPreset < MAX_NUM_EQ_USER_PRESETS; ++iPreset )
    {
        if ( pSettings->vstrEQPresetNames[iPreset].compare ( strName, Qt::CaseInsensitive ) == 0 )
        {
            return iPreset;
        }
    }

    return INVALID_INDEX;
}

int CEffectsDlg::FindFreePresetSlot() const
{
    for ( int iPreset = 0; iPreset < MAX_NUM_EQ_USER_PRESETS; ++iPreset )
    {
        if ( pSettings->vstrEQPresetNames[iPreset].isEmpty() )
        {
            return iPreset;
        }
    }

    return INVALID_INDEX;
}

void CEffectsDlg::OnResetEQClicked()
{
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pSldEQBands[iBand]->setValue ( 0 );
    }

    emit EQResetRequested();
    // Remove focus from the button to prevent blue outline
    pTabs->setFocus();
}

void CEffectsDlg::OnSaveEQPresetClicked()
{
    const int iComboIndex = pCbxEQPresets->currentIndex();
    if ( iComboIndex < 0 )
    {
        QMessageBox::warning ( this,
                               tr ( "No Preset Selected" ),
                               tr ( "Select an existing preset to overwrite, or use Save As... to create a new one." ) );
        return;
    }

    const int     iPresetSlot = pCbxEQPresets->itemData ( iComboIndex, Qt::UserRole + 1 ).toInt();
    const QString strName     = pCbxEQPresets->itemText ( iComboIndex );

    if ( iPresetSlot == INVALID_INDEX )
    {
        QMessageBox::warning ( this,
                               tr ( "Preset Limit Reached" ),
                               tr ( "No free preset slot is available. Delete an existing user preset first." ) );
        return;
    }

    pSettings->vstrEQPresetNames[iPresetSlot] = strName;
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pSettings->aiEQPresetBandGainDb[iPresetSlot][iBand] = pSldEQBands[iBand]->value();
    }

    PopulateEQPresetCombo();
    const int iUpdatedIndex = pCbxEQPresets->findText ( strName );
    if ( iUpdatedIndex >= 0 )
    {
        pCbxEQPresets->setCurrentIndex ( iUpdatedIndex );
    }
}

void CEffectsDlg::OnSaveAsEQPresetClicked()
{
    bool    bOk     = false;
    QString strName = QInputDialog::getText ( this, tr ( "Save EQ Preset" ), tr ( "Preset name:" ), QLineEdit::Normal, "", &bOk ).trimmed();

    if ( !bOk || strName.isEmpty() )
    {
        return;
    }

    int iPresetSlot = FindPresetSlotByName ( strName );
    if ( iPresetSlot == INVALID_INDEX )
    {
        iPresetSlot = FindFreePresetSlot();
    }

    if ( iPresetSlot == INVALID_INDEX )
    {
        QMessageBox::warning ( this,
                               tr ( "Preset Limit Reached" ),
                               tr ( "No free preset slot is available. Delete an existing user preset first." ) );
        return;
    }

    pSettings->vstrEQPresetNames[iPresetSlot] = strName;
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pSettings->aiEQPresetBandGainDb[iPresetSlot][iBand] = pSldEQBands[iBand]->value();
    }

    PopulateEQPresetCombo();
    const int iSavedIndex = pCbxEQPresets->findText ( strName );
    if ( iSavedIndex >= 0 )
    {
        pCbxEQPresets->setCurrentIndex ( iSavedIndex );
    }
}

void CEffectsDlg::OnDeleteEQPresetClicked()
{
    const int iPresetSlot = pCbxEQPresets->currentData ( Qt::UserRole + 1 ).toInt();
    if ( iPresetSlot < 0 || iPresetSlot >= MAX_NUM_EQ_USER_PRESETS )
    {
        return;
    }

    pSettings->vstrEQPresetNames[iPresetSlot].clear();
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pSettings->aiEQPresetBandGainDb[iPresetSlot][iBand] = 0;
    }

    PopulateEQPresetCombo();
}
