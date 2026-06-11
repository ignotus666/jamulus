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
#include "eqcurvewidget.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QIntValidator>
#include <QLineEdit>
#include <QMessageBox>

CEffectsDlg::CEffectsDlg ( CClient* pNCliP, CClientSettings* pNSetP, QWidget* parent ) :
    CBaseDlg ( parent, Qt::Window ),
    pClient ( pNCliP ),
    pSettings ( pNSetP )
{
    setupUi ( this );

    pGRMeter = new CGRMeter ( this );
    gridLayoutCompressorRows->addWidget ( pGRMeter, 0, 3, 5, 1 );

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

    pEQCurveWidget->SetSampleRate ( SYSTEM_SAMPLE_RATE_HZ );
    QObject::connect ( pEQCurveWidget, &CEQCurveWidget::bandGainChanged, this, &CEffectsDlg::OnEQBandGainChanged );
    QObject::connect ( pEQCurveWidget, &CEQCurveWidget::bandFrequencyChanged, this, &CEffectsDlg::OnEQBandFrequencyChanged );
    QObject::connect ( pEQCurveWidget, &CEQCurveWidget::bandSelected, this, &CEffectsDlg::OnEQBandSelected );
    QObject::connect ( pEQCurveWidget, &CEQCurveWidget::bandGainReset, this, &CEffectsDlg::OnEQBandGainReset );

    pLblEQDynThresholdValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblEQDynRatioValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblEQDynAttackValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblEQDynReleaseValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pLblEQDynThresholdValue->setMinimumWidth ( 60 );
    pLblEQDynRatioValue->setMinimumWidth ( 60 );
    pLblEQDynAttackValue->setMinimumWidth ( 60 );
    pLblEQDynReleaseValue->setMinimumWidth ( 60 );

    pSldEQDynThreshold->setRange ( -60, 0 );
    pSldEQDynThreshold->setTickInterval ( 12 );
    pSldEQDynThreshold->setTickPosition ( QSlider::TicksBothSides );

    pSldEQDynRatio->setRange ( 1, 20 );
    pSldEQDynRatio->setTickInterval ( 4 );
    pSldEQDynRatio->setTickPosition ( QSlider::TicksBothSides );

    pSldEQDynAttack->setRange ( 1, 100 );
    pSldEQDynAttack->setTickInterval ( 20 );
    pSldEQDynAttack->setTickPosition ( QSlider::TicksBothSides );

    pSldEQDynRelease->setRange ( 10, 500 );
    pSldEQDynRelease->setTickInterval ( 100 );
    pSldEQDynRelease->setTickPosition ( QSlider::TicksBothSides );

    QObject::connect ( pChbEQDynEnabled, &QCheckBox::toggled, this, &CEffectsDlg::OnEQDynEnabledChanged );
    QObject::connect ( pSldEQDynThreshold, &CCustomSlider::valueChanged, this, &CEffectsDlg::OnEQDynThresholdChanged );
    QObject::connect ( pSldEQDynRatio, &CCustomSlider::valueChanged, this, &CEffectsDlg::OnEQDynRatioChanged );
    QObject::connect ( pSldEQDynAttack, &CCustomSlider::valueChanged, this, &CEffectsDlg::OnEQDynAttackChanged );
    QObject::connect ( pSldEQDynRelease, &CCustomSlider::valueChanged, this, &CEffectsDlg::OnEQDynReleaseChanged );

    // Frequency input: starts read-only, click to edit, commit on Return or focus loss
    pEdtEQDynFreq->setValidator (
        new QIntValidator ( static_cast<int> ( CEQCurveWidget::kFreqMin ), static_cast<int> ( CEQCurveWidget::kFreqMax ), pEdtEQDynFreq ) );
    pEdtEQDynFreq->installEventFilter ( this );
    QObject::connect ( pEdtEQDynFreq, &QLineEdit::returnPressed, this, &CEffectsDlg::OnEQDynFreqEditFinished );

    iSelectedBand = 0;

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
    QObject::connect ( pChbEQBypass, &QCheckBox::toggled, this, [this] ( bool bBypassed ) {
        if ( pEQCurveWidget )
        {
            pEQCurveWidget->SetBypassed ( bBypassed );
        }
        emit EQBypassChanged ( bBypassed );
    } );
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
    UpdateCompressorControls();
    UpdateEQReadouts();
    PopulateEffectsPresetCombo();
    PopulateEQPresetCombo();
    UpdateEQPresetSelection();
}

void CEffectsDlg::UpdateOutputBandLevels ( const CVector<float>& vecOutLevels )
{
    if ( pEQCurveWidget && pEQCurveWidget->isVisible() && pClient )
    {
        if ( pClient->GetEQBypass() )
        {
            return;
        }

        // 1. Pass Goertzel spectrum levels to in-graph background analyzer
        QVector<float> vecLevels;
        vecLevels.reserve ( vecOutLevels.Size() );
        for ( int i = 0; i < vecOutLevels.Size(); ++i )
        {
            vecLevels.append ( vecOutLevels[i] );
        }
        pEQCurveWidget->SetSpectrumLevels ( vecLevels );

        // 2. Pass real-time dynamic gain reductions to in-graph indicators
        for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
        {
            pEQCurveWidget->SetBandGainReduction ( iBand, pClient->GetEQBandGainReductionDb ( iBand ) );
        }
    }
}

void CEffectsDlg::UpdateCompressorGainReduction ( const float fGRDb )
{
    if ( pGRMeter )
    {
        pGRMeter->SetGainReductionDb ( fGRDb );
    }
}

void CEffectsDlg::showEvent ( QShowEvent* Event )
{
    if ( pClient )
    {
        pClient->SetOutputBandLevelsEnabled ( true );
    }
    ApplyThemeToCustomWidgets();

    // Restore last used tab if valid (do this first)
    if ( pSettings->iEffectsTab >= 0 && pSettings->iEffectsTab < pTabs->count() )
    {
        pTabs->setCurrentIndex ( pSettings->iEffectsTab );
    }
    PopulateEffectsPresetCombo();
    UpdateReverbControls();
    UpdateCompressorControls();
    UpdateEQControls();
    UpdateEQPresetSelection();
    CBaseDlg::showEvent ( Event );
}

void CEffectsDlg::hideEvent ( QHideEvent* Event )
{
    if ( pClient )
    {
        pClient->SetOutputBandLevelsEnabled ( false );
    }
    CBaseDlg::hideEvent ( Event );
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
        pSldCompressorThreshold,
        pSldCompressorRatio,
        pSldCompressorAttack,
        pSldCompressorRelease,
        pSldCompressorMakeup,
        pSldEQDynThreshold,
        pSldEQDynRatio,
        pSldEQDynAttack,
        pSldEQDynRelease,
    };

    for ( CCustomSlider* pSlider : apThemeSliders )
    {
        if ( pSlider )
        {
            pSlider->SetDarkTheme ( bDarkTheme );
        }
    }

    if ( pEQCurveWidget )
    {
        pEQCurveWidget->SetDarkTheme ( bDarkTheme );
    }

    if ( pGRMeter )
    {
        pGRMeter->SetDarkTheme ( bDarkTheme );
    }
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
    // Obsolete with graphical EQ curve
}

void CEffectsDlg::UpdateEQControls()
{
    const bool bBypassed = pClient->GetEQBypass();
    pChbEQBypass->blockSignals ( true );
    pChbEQBypass->setChecked ( bBypassed );
    pChbEQBypass->blockSignals ( false );

    if ( pEQCurveWidget && pClient )
    {
        pEQCurveWidget->SetBypassed ( bBypassed );
        for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
        {
            pEQCurveWidget->SetBandGain ( iBand, pClient->GetEQBandGainDb ( iBand ) );
            pEQCurveWidget->SetBandFrequency ( iBand, pClient->GetEQBandFrequency ( iBand ) );
        }
    }

    UpdateEQDynControls ( iSelectedBand );
}

void CEffectsDlg::PopulateEffectsPresetCombo()
{
    const bool bBlocked = pCbxEffectsPresets->blockSignals ( true );
    pCbxEffectsPresets->clear();

    int iTargetIndex = -1;
    for ( int iPreset = 0; iPreset < MAX_NUM_EFFECT_PRESETS; ++iPreset )
    {
        const QString strName = pSettings->vstrEffectsPresetNames[iPreset];
        if ( strName.isEmpty() )
        {
            continue;
        }

        pCbxEffectsPresets->addItem ( strName );
        const int iCurrentItemIdx = pCbxEffectsPresets->count() - 1;
        pCbxEffectsPresets->setItemData ( iCurrentItemIdx, iPreset, Qt::UserRole );

        if ( pSettings->iSelectedEffectsPreset != INVALID_INDEX && iPreset == pSettings->iSelectedEffectsPreset )
        {
            iTargetIndex = iCurrentItemIdx;
        }
    }

    if ( iTargetIndex >= 0 )
    {
        pCbxEffectsPresets->setCurrentIndex ( iTargetIndex );
    }
    else if ( pCbxEffectsPresets->count() > 0 )
    {
        pCbxEffectsPresets->setCurrentIndex ( 0 );
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

void CEffectsDlg::ApplyEffectsPreset ( const int iPresetSlot )
{
    ApplyEffectsPresetFromSlot ( iPresetSlot );
}

void CEffectsDlg::ApplyEffectsPresetFromSlot ( const int iPresetSlot )
{
    if ( iPresetSlot < 0 || iPresetSlot >= MAX_NUM_EFFECT_PRESETS )
    {
        return;
    }

    pSettings->iSelectedEffectsPreset = iPresetSlot;

    // Set the combo box selection to match this preset slot
    int iComboIdx = -1;
    for ( int i = 0; i < pCbxEffectsPresets->count(); ++i )
    {
        if ( pCbxEffectsPresets->itemData ( i, Qt::UserRole ).toInt() == iPresetSlot )
        {
            iComboIdx = i;
            break;
        }
    }
    if ( iComboIdx >= 0 )
    {
        const bool bBlocked = pCbxEffectsPresets->blockSignals ( true );
        pCbxEffectsPresets->setCurrentIndex ( iComboIdx );
        pCbxEffectsPresets->blockSignals ( bBlocked );
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

    pClient->SetEQBypass ( pSettings->bEffectsPresetEQBypass[iPresetSlot] );
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pClient->SetEQBandGainDb ( iBand, pSettings->aiEffectsPresetEQBandGainDb[iPresetSlot][iBand] );
        pClient->SetEQBandFrequency ( iBand, pSettings->aiEffectsPresetEQBandFrequency[iPresetSlot][iBand] );
        pClient->SetEQBandDynEnabled ( iBand, pSettings->abEffectsPresetEQBandDynEnabled[iPresetSlot][iBand] );
        pClient->SetEQBandDynThresholdDb ( iBand, pSettings->aiEffectsPresetEQBandDynThresholdDb[iPresetSlot][iBand] );
        pClient->SetEQBandDynRatio ( iBand, pSettings->aiEffectsPresetEQBandDynRatio[iPresetSlot][iBand] );
        pClient->SetEQBandDynAttackMs ( iBand, pSettings->aiEffectsPresetEQBandDynAttackMs[iPresetSlot][iBand] );
        pClient->SetEQBandDynReleaseMs ( iBand, pSettings->aiEffectsPresetEQBandDynReleaseMs[iPresetSlot][iBand] );
    }

    UpdateReverbControls();
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

    pSettings->SaveEffectsPresetFromClient ( iPresetSlot );

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

    pSettings->SaveEffectsPresetFromClient ( iPresetSlot );

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
        pSettings->aiEffectsPresetEQBandGainDb[iPresetSlot][iBand]         = 0;
        pSettings->abEffectsPresetEQBandDynEnabled[iPresetSlot][iBand]     = false;
        pSettings->aiEffectsPresetEQBandDynThresholdDb[iPresetSlot][iBand] = -20;
        pSettings->aiEffectsPresetEQBandDynRatio[iPresetSlot][iBand]       = 4;
        pSettings->aiEffectsPresetEQBandDynAttackMs[iPresetSlot][iBand]    = 5;
        pSettings->aiEffectsPresetEQBandDynReleaseMs[iPresetSlot][iBand]   = 80;
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

    if ( iPresetSlot == pSettings->iSelectedEffectsPreset )
    {
        pSettings->iSelectedEffectsPreset = INVALID_INDEX;
    }

    PopulateEffectsPresetCombo();
}

void CEffectsDlg::PopulateEQPresetCombo()
{
    const bool bBlocked = pCbxEQPresets->blockSignals ( true );
    pCbxEQPresets->clear();

    int iTargetIndex = -1;
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
        const int iCurrentItemIdx = pCbxEQPresets->count() - 1;
        pCbxEQPresets->setItemData ( iCurrentItemIdx, strGains, Qt::UserRole );
        pCbxEQPresets->setItemData ( iCurrentItemIdx, iPreset, Qt::UserRole + 1 );

        if ( pSettings->iSelectedEQPreset != INVALID_INDEX && iPreset == pSettings->iSelectedEQPreset )
        {
            iTargetIndex = iCurrentItemIdx;
        }
    }

    if ( iTargetIndex >= 0 )
    {
        pCbxEQPresets->setCurrentIndex ( iTargetIndex );
    }
    else if ( pCbxEQPresets->count() > 0 )
    {
        pCbxEQPresets->setCurrentIndex ( 0 );
    }

    pCbxEQPresets->blockSignals ( bBlocked );
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
        currentGains += QString::number ( pClient->GetEQBandGainDb ( iBand ) );
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
        pSettings->iSelectedEQPreset = pCbxEQPresets->itemData ( matchedIndex, Qt::UserRole + 1 ).toInt();
    }
}

void CEffectsDlg::ApplyPresetFromComboIndex ( const int iPresetIndex )
{
    if ( iPresetIndex < 0 )
    {
        return;
    }

    const int iPresetSlot = pCbxEQPresets->itemData ( iPresetIndex, Qt::UserRole + 1 ).toInt();
    if ( iPresetSlot < 0 || iPresetSlot >= MAX_NUM_EQ_USER_PRESETS )
    {
        return;
    }

    pSettings->iSelectedEQPreset = iPresetSlot;

    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        pClient->SetEQBandGainDb ( iBand, pSettings->aiEQPresetBandGainDb[iPresetSlot][iBand] );
        pClient->SetEQBandFrequency ( iBand, pSettings->aiEQPresetBandFrequency[iPresetSlot][iBand] );
        pClient->SetEQBandDynEnabled ( iBand, pSettings->abEQPresetBandDynEnabled[iPresetSlot][iBand] );
        pClient->SetEQBandDynThresholdDb ( iBand, pSettings->aiEQPresetBandDynThresholdDb[iPresetSlot][iBand] );
        pClient->SetEQBandDynRatio ( iBand, pSettings->aiEQPresetBandDynRatio[iPresetSlot][iBand] );
        pClient->SetEQBandDynAttackMs ( iBand, pSettings->aiEQPresetBandDynAttackMs[iPresetSlot][iBand] );
        pClient->SetEQBandDynReleaseMs ( iBand, pSettings->aiEQPresetBandDynReleaseMs[iPresetSlot][iBand] );
    }

    UpdateEQControls();
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

    pSettings->SaveEQPresetFromClient ( iPresetSlot );

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

    pSettings->SaveEQPresetFromClient ( iPresetSlot );

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
        pSettings->aiEQPresetBandGainDb[iPresetSlot][iBand]         = 0;
        pSettings->abEQPresetBandDynEnabled[iPresetSlot][iBand]     = false;
        pSettings->aiEQPresetBandDynThresholdDb[iPresetSlot][iBand] = -20;
        pSettings->aiEQPresetBandDynRatio[iPresetSlot][iBand]       = 4;
        pSettings->aiEQPresetBandDynAttackMs[iPresetSlot][iBand]    = 5;
        pSettings->aiEQPresetBandDynReleaseMs[iPresetSlot][iBand]   = 80;
    }

    if ( iPresetSlot == pSettings->iSelectedEQPreset )
    {
        pSettings->iSelectedEQPreset = INVALID_INDEX;
    }

    PopulateEQPresetCombo();
}

void CEffectsDlg::OnEQBandGainChanged ( int iBand, int iGainDb )
{
    if ( pClient )
    {
        pClient->SetEQBandGainDb ( iBand, iGainDb );
        UpdateEQPresetSelection();
    }
}

void CEffectsDlg::OnEQBandFrequencyChanged ( int iBand, float fFreqHz )
{
    if ( pClient )
    {
        pClient->SetEQBandFrequency ( iBand, fFreqHz );
        UpdateEQPresetSelection();
        if ( iBand == iSelectedBand )
        {
            UpdateEQDynControls ( iBand );
        }
    }
}

void CEffectsDlg::OnEQBandSelected ( int iBand ) { UpdateEQDynControls ( iBand ); }

void CEffectsDlg::OnEQBandGainReset ( int iBand )
{
    if ( pClient )
    {
        pClient->SetEQBandGainDb ( iBand, 0.0f );
        UpdateEQPresetSelection();
    }
}

void CEffectsDlg::OnEQDynEnabledChanged ( bool bEnabled )
{
    if ( pClient )
    {
        pClient->SetEQBandDynEnabled ( iSelectedBand, bEnabled );
        UpdateEQDynControls ( iSelectedBand );
    }
}

void CEffectsDlg::OnEQDynThresholdChanged ( int iValue )
{
    if ( pClient )
    {
        pClient->SetEQBandDynThresholdDb ( iSelectedBand, static_cast<float> ( iValue ) );
        pLblEQDynThresholdValue->setText ( QString ( "%1 dB" ).arg ( iValue ) );
    }
}

void CEffectsDlg::OnEQDynRatioChanged ( int iValue )
{
    if ( pClient )
    {
        pClient->SetEQBandDynRatio ( iSelectedBand, static_cast<float> ( iValue ) );
        pLblEQDynRatioValue->setText ( QString ( "%1:1" ).arg ( iValue ) );
    }
}

void CEffectsDlg::OnEQDynAttackChanged ( int iValue )
{
    if ( pClient )
    {
        pClient->SetEQBandDynAttackMs ( iSelectedBand, static_cast<float> ( iValue ) );
        pLblEQDynAttackValue->setText ( QString ( "%1 ms" ).arg ( iValue ) );
    }
}

void CEffectsDlg::OnEQDynReleaseChanged ( int iValue )
{
    if ( pClient )
    {
        pClient->SetEQBandDynReleaseMs ( iSelectedBand, static_cast<float> ( iValue ) );
        pLblEQDynReleaseValue->setText ( QString ( "%1 ms" ).arg ( iValue ) );
    }
}

void CEffectsDlg::OnResetEQClicked()
{
    if ( pClient )
    {
        pClient->ResetEQ();
    }
    UpdateEQControls();
    pTabs->setFocus();
}

void CEffectsDlg::UpdateEQDynControls ( const int iBand )
{
    if ( !pClient || iBand < 0 || iBand >= CAudioEqualizer::NUM_BANDS )
    {
        return;
    }

    iSelectedBand = iBand;

    pChbEQDynEnabled->blockSignals ( true );
    pSldEQDynThreshold->blockSignals ( true );
    pSldEQDynRatio->blockSignals ( true );
    pSldEQDynAttack->blockSignals ( true );
    pSldEQDynRelease->blockSignals ( true );

    const bool bEnabled = pClient->GetEQBandDynEnabled ( iBand );
    pChbEQDynEnabled->setChecked ( bEnabled );

    const float fFreq = pClient->GetEQBandFrequency ( iBand );
    QString     strFreq;
    if ( fFreq >= 1000.0f )
    {
        strFreq = QString ( "%1k Hz" ).arg ( fFreq / 1000.0f, 0, 'f', 1 );
    }
    else
    {
        strFreq = QString ( "%1 Hz" ).arg ( static_cast<int> ( fFreq ) );
    }
    pEdtEQDynFreq->blockSignals ( true );
    pEdtEQDynFreq->setReadOnly ( true );
    pEdtEQDynFreq->setFrame ( false );
    pEdtEQDynFreq->setText ( strFreq );
    pEdtEQDynFreq->blockSignals ( false );

    const float fThreshold = pClient->GetEQBandDynThresholdDb ( iBand );
    pSldEQDynThreshold->setValue ( static_cast<int> ( std::round ( fThreshold ) ) );
    pLblEQDynThresholdValue->setText ( QString ( "%1 dB" ).arg ( static_cast<int> ( std::round ( fThreshold ) ) ) );

    const float fRatio = pClient->GetEQBandDynRatio ( iBand );
    pSldEQDynRatio->setValue ( static_cast<int> ( std::round ( fRatio ) ) );
    pLblEQDynRatioValue->setText ( QString ( "%1:1" ).arg ( static_cast<int> ( std::round ( fRatio ) ) ) );

    const float fAttack = pClient->GetEQBandDynAttackMs ( iBand );
    pSldEQDynAttack->setValue ( static_cast<int> ( std::round ( fAttack ) ) );
    pLblEQDynAttackValue->setText ( QString ( "%1 ms" ).arg ( static_cast<int> ( std::round ( fAttack ) ) ) );

    const float fRelease = pClient->GetEQBandDynReleaseMs ( iBand );
    pSldEQDynRelease->setValue ( static_cast<int> ( std::round ( fRelease ) ) );
    pLblEQDynReleaseValue->setText ( QString ( "%1 ms" ).arg ( static_cast<int> ( std::round ( fRelease ) ) ) );

    pChbEQDynEnabled->blockSignals ( false );
    pSldEQDynThreshold->blockSignals ( false );
    pSldEQDynRatio->blockSignals ( false );
    pSldEQDynAttack->blockSignals ( false );
    pSldEQDynRelease->blockSignals ( false );

    pSldEQDynThreshold->setEnabled ( bEnabled );
    pSldEQDynRatio->setEnabled ( bEnabled );
    pSldEQDynAttack->setEnabled ( bEnabled );
    pSldEQDynRelease->setEnabled ( bEnabled );
    pLblEQDynThreshold->setEnabled ( bEnabled );
    pLblEQDynThresholdValue->setEnabled ( bEnabled );
    pLblEQDynRatio->setEnabled ( bEnabled );
    pLblEQDynRatioValue->setEnabled ( bEnabled );
    pLblEQDynAttack->setEnabled ( bEnabled );
    pLblEQDynAttackValue->setEnabled ( bEnabled );
    pLblEQDynRelease->setEnabled ( bEnabled );
    pLblEQDynReleaseValue->setEnabled ( bEnabled );
}

bool CEffectsDlg::eventFilter ( QObject* pObj, QEvent* pEvent )
{
    if ( pObj == pEdtEQDynFreq )
    {
        if ( pEvent->type() == QEvent::MouseButtonPress || pEvent->type() == QEvent::MouseButtonDblClick )
        {
            if ( pEdtEQDynFreq->isReadOnly() )
            {
                // Enter edit mode: show raw Hz value so user can type precisely
                const float fFreq = pClient ? pClient->GetEQBandFrequency ( iSelectedBand ) : 0.0f;
                pEdtEQDynFreq->setReadOnly ( false );
                pEdtEQDynFreq->setFrame ( true );
                pEdtEQDynFreq->setText ( QString::number ( static_cast<int> ( fFreq ) ) );
                pEdtEQDynFreq->selectAll();
                pEdtEQDynFreq->setFocus();
                return true;
            }
        }
        else if ( pEvent->type() == QEvent::FocusOut )
        {
            if ( !pEdtEQDynFreq->isReadOnly() )
            {
                OnEQDynFreqEditFinished();
                return true;
            }
        }
    }

    return CBaseDlg::eventFilter ( pObj, pEvent );
}

void CEffectsDlg::OnEQDynFreqEditFinished()
{
    if ( !pClient || iSelectedBand < 0 || iSelectedBand >= CAudioEqualizer::NUM_BANDS )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    bool  bOk     = false;
    float fFreqHz = pEdtEQDynFreq->text().toFloat ( &bOk );

    if ( !bOk )
    {
        // Invalid input – revert display
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    // Clamp to valid range
    fFreqHz = std::max ( CEQCurveWidget::kFreqMin, std::min ( CEQCurveWidget::kFreqMax, fFreqHz ) );

    // Prevent band crossover: keep 10% margin from neighbours (same logic as mouse drag)
    if ( iSelectedBand > 0 )
    {
        const float fMinFreq = pClient->GetEQBandFrequency ( iSelectedBand - 1 ) * 1.10f;
        fFreqHz              = std::max ( fFreqHz, fMinFreq );
    }

    if ( iSelectedBand < CAudioEqualizer::NUM_BANDS - 1 )
    {
        const float fMaxFreq = pClient->GetEQBandFrequency ( iSelectedBand + 1 ) * 0.90f;
        fFreqHz              = std::min ( fFreqHz, fMaxFreq );
    }

    pClient->SetEQBandFrequency ( iSelectedBand, fFreqHz );

    if ( pEQCurveWidget )
    {
        pEQCurveWidget->SetBandFrequency ( iSelectedBand, fFreqHz );
    }

    UpdateEQDynControls ( iSelectedBand );
    UpdateEQPresetSelection();
}
