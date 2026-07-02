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
#include "customknob.h"
#include "eqcurvewidget.h"
#include "uicolors.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QIntValidator>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>

CEffectsDlg::CEffectsDlg ( CClient* pNCliP, CClientSettings* pNSetP, QWidget* parent ) :
    CBaseDlg ( parent, Qt::Window ),
    pClient ( pNCliP ),
    pSettings ( pNSetP )
{
    setupUi ( this );

    pGRMeter = new CGRMeter ( this );
    gridLayoutCompressorRows->addWidget ( pGRMeter, 0, 7, 3, 1 );

    pCompCurveWidget = new CCompCurveWidget ( this );
    gridLayoutCompressorRows->addWidget ( pCompCurveWidget, 0, 8, 3, 1 );

    pReverbDecayWidget = new CReverbDecayWidget ( this );
    gridLayoutReverbRows->addWidget ( pReverbDecayWidget, 0, 7, 3, 1 );

    // Save tab index on change
    connect ( pTabs, &QTabWidget::currentChanged, this, [this] ( int idx ) { pSettings->iEffectsTab = idx; } );

    setWindowTitle ( tr ( "Effects" ) );
    pKnobReverb->setRange ( 0, AUD_REVERB_MAX );

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

    pKnobReverbPreDelay->setRange ( 0, REVERB_PRE_DELAY_MAX_MS );
    pKnobReverbRoom->setRange ( 0, REVERB_ROOM_SIZE_MAX );
    pKnobReverbDamping->setRange ( 0, REVERB_DAMPING_MAX );
    pKnobReverbWet->setRange ( 0, REVERB_WET_MIX_MAX );
    pKnobReverbEarly->setRange ( 0, REVERB_EARLY_LEVEL_MAX );
    pKnobReverbWidth->setRange ( 0, REVERB_WIDTH_MAX );

    pLblCompressorThresholdValue->setAlignment ( Qt::AlignLeft | Qt::AlignVCenter );
    pLblCompressorRatioValue->setAlignment ( Qt::AlignLeft | Qt::AlignVCenter );
    pLblCompressorAttackValue->setAlignment ( Qt::AlignLeft | Qt::AlignVCenter );
    pLblCompressorReleaseValue->setAlignment ( Qt::AlignLeft | Qt::AlignVCenter );
    pLblCompressorMakeupValue->setAlignment ( Qt::AlignLeft | Qt::AlignVCenter );
    pLblCompressorThresholdValue->setMinimumWidth ( 48 );
    pLblCompressorRatioValue->setMinimumWidth ( 48 );
    pLblCompressorAttackValue->setMinimumWidth ( 48 );
    pLblCompressorReleaseValue->setMinimumWidth ( 55 );
    pLblCompressorMakeupValue->setMinimumWidth ( 48 );
    pKnobCompressorThreshold->setRange ( -60, 0 );
    pKnobCompressorRatio->setRange ( 1, 20 );
    pKnobCompressorAttack->setRange ( 1, 50 );
    pKnobCompressorRelease->setRange ( 10, 400 );
    pKnobCompressorMakeup->setRange ( 0, 24 );

    pEQCurveWidget->SetSampleRate ( SYSTEM_SAMPLE_RATE_HZ );
    QObject::connect ( pEQCurveWidget, &CEQCurveWidget::bandGainChanged, this, &CEffectsDlg::OnEQBandGainChanged );
    QObject::connect ( pEQCurveWidget, &CEQCurveWidget::bandFrequencyChanged, this, &CEffectsDlg::OnEQBandFrequencyChanged );
    QObject::connect ( pEQCurveWidget, &CEQCurveWidget::bandSelected, this, &CEffectsDlg::OnEQBandSelected );
    QObject::connect ( pEQCurveWidget, &CEQCurveWidget::bandGainReset, this, &CEffectsDlg::OnEQBandGainReset );

    pEdtEQDynThreshold->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pEdtEQDynRatio->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pEdtEQDynAttack->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pEdtEQDynRelease->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
    pEdtEQDynThreshold->setMinimumWidth ( 60 );
    pEdtEQDynRatio->setMinimumWidth ( 60 );
    pEdtEQDynAttack->setMinimumWidth ( 60 );
    pEdtEQDynRelease->setMinimumWidth ( 60 );

    pEdtEQDynThreshold->setValidator ( new QIntValidator ( -60, 0, pEdtEQDynThreshold ) );
    pEdtEQDynThreshold->installEventFilter ( this );
    QObject::connect ( pEdtEQDynThreshold, &QLineEdit::returnPressed, this, &CEffectsDlg::OnEQDynThresholdEditFinished );

    pEdtEQDynRatio->setValidator ( new QIntValidator ( 1, 20, pEdtEQDynRatio ) );
    pEdtEQDynRatio->installEventFilter ( this );
    QObject::connect ( pEdtEQDynRatio, &QLineEdit::returnPressed, this, &CEffectsDlg::OnEQDynRatioEditFinished );

    pEdtEQDynAttack->setValidator ( new QIntValidator ( 1, 100, pEdtEQDynAttack ) );
    pEdtEQDynAttack->installEventFilter ( this );
    QObject::connect ( pEdtEQDynAttack, &QLineEdit::returnPressed, this, &CEffectsDlg::OnEQDynAttackEditFinished );

    pEdtEQDynRelease->setValidator ( new QIntValidator ( 10, 500, pEdtEQDynRelease ) );
    pEdtEQDynRelease->installEventFilter ( this );
    QObject::connect ( pEdtEQDynRelease, &QLineEdit::returnPressed, this, &CEffectsDlg::OnEQDynReleaseEditFinished );

    pKnobEQDynThreshold->setRange ( -60, 0 );
    pKnobEQDynRatio->setRange ( 1, 20 );
    pKnobEQDynAttack->setRange ( 1, 100 );
    pKnobEQDynRelease->setRange ( 10, 500 );

    QObject::connect ( pChbEQDynEnabled, &QCheckBox::toggled, this, &CEffectsDlg::OnEQDynEnabledChanged );
    QObject::connect ( pKnobEQDynThreshold, &CCustomKnob::valueChanged, this, &CEffectsDlg::OnEQDynThresholdChanged );
    QObject::connect ( pKnobEQDynRatio, &CCustomKnob::valueChanged, this, &CEffectsDlg::OnEQDynRatioChanged );
    QObject::connect ( pKnobEQDynAttack, &CCustomKnob::valueChanged, this, &CEffectsDlg::OnEQDynAttackChanged );
    QObject::connect ( pKnobEQDynRelease, &CCustomKnob::valueChanged, this, &CEffectsDlg::OnEQDynReleaseChanged );

    // Q (quality factor / bandwidth) knob — range 3..100 maps to Q 0.3..10.0
    pEdtEQBandQ->setAlignment ( Qt::AlignLeft | Qt::AlignVCenter );
    pEdtEQBandQ->setMinimumWidth ( 30 );
    pEdtEQBandQ->setValidator ( new QDoubleValidator ( 0.3, 10.0, 1, pEdtEQBandQ ) );
    pEdtEQBandQ->installEventFilter ( this );
    QObject::connect ( pEdtEQBandQ, &QLineEdit::returnPressed, this, &CEffectsDlg::OnEQBandQEditFinished );

    pKnobEQBandQ->setRange ( 3, 100 );
    QObject::connect ( pKnobEQBandQ, &CCustomKnob::valueChanged, this, &CEffectsDlg::OnEQBandQChanged );

    // Gain knob — range -120..120 maps to Gain -12.0..+12.0 dB
    pKnobEQBandGain->setRange ( -120, 120 );
    QObject::connect ( pKnobEQBandGain, &CCustomKnob::valueChanged, this, &CEffectsDlg::OnEQBandGainKnobChanged );

    pKnobEQDynThreshold->setFixedSize ( 40, 40 );
    pKnobEQDynRatio->setFixedSize ( 40, 40 );
    pKnobEQDynAttack->setFixedSize ( 40, 40 );
    pKnobEQDynRelease->setFixedSize ( 40, 40 );
    pKnobEQBandQ->setFixedSize ( 40, 40 );
    pKnobEQBandGain->setFixedSize ( 40, 40 );

    // Frequency input: starts read-only, click to edit, commit on Return or focus loss
    pEdtEQDynFreq->setValidator (
        new QIntValidator ( static_cast<int> ( CEQCurveWidget::kFreqMin ), static_cast<int> ( CEQCurveWidget::kFreqMax ), pEdtEQDynFreq ) );
    pEdtEQDynFreq->installEventFilter ( this );
    QObject::connect ( pEdtEQDynFreq, &QLineEdit::returnPressed, this, &CEffectsDlg::OnEQDynFreqEditFinished );

    // Gain input: starts read-only, click to edit, commit on Return or focus loss
    auto* pGainValidator = new QDoubleValidator ( CEQCurveWidget::kGainMinDb, CEQCurveWidget::kGainMaxDb, 1, pEdtEQDynGain );
    pGainValidator->setNotation ( QDoubleValidator::StandardNotation );
    pEdtEQDynGain->setValidator ( pGainValidator );
    pEdtEQDynGain->installEventFilter ( this );
    QObject::connect ( pEdtEQDynGain, &QLineEdit::returnPressed, this, &CEffectsDlg::OnEQDynGainEditFinished );

    iSelectedBand = 0;

    QObject::connect ( pKnobReverb, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblReverbValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbValueChanged ( value );
    } );
    QObject::connect ( pChbReverbEnable, &QCheckBox::toggled, this, [this] ( bool enabled ) {
        emit ReverbBypassChanged ( !enabled );
        if ( pReverbDecayWidget )
        {
            pReverbDecayWidget->SetBypass ( !enabled );
        }
    } );
    QObject::connect ( pKnobReverbPreDelay, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblReverbPreDelayValue->setText ( QString::number ( value ) + tr ( " ms" ) );
        emit ReverbPreDelayChanged ( value );
        if ( pReverbDecayWidget )
        {
            pReverbDecayWidget->SetPreDelayMs ( static_cast<float> ( value ) );
        }
    } );
    QObject::connect ( pKnobReverbRoom, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblReverbRoomValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbRoomSizeChanged ( value );
        if ( pReverbDecayWidget )
        {
            pReverbDecayWidget->SetRoomSize ( static_cast<float> ( value ) / REVERB_ROOM_SIZE_MAX );
        }
    } );
    QObject::connect ( pKnobReverbDamping, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblReverbDampingValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbDampingChanged ( value );
        if ( pReverbDecayWidget )
        {
            pReverbDecayWidget->SetDamping ( static_cast<float> ( value ) / REVERB_DAMPING_MAX );
        }
    } );
    QObject::connect ( pKnobReverbWet, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblReverbWetValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbWetMixChanged ( value );
        if ( pReverbDecayWidget )
        {
            pReverbDecayWidget->SetWetMix ( static_cast<float> ( value ) / REVERB_WET_MIX_MAX );
        }
    } );
    QObject::connect ( pKnobReverbEarly, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblReverbEarlyValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbEarlyLevelChanged ( value );
        if ( pReverbDecayWidget )
        {
            pReverbDecayWidget->SetEarlyLevel ( static_cast<float> ( value ) / REVERB_EARLY_LEVEL_MAX );
        }
    } );
    QObject::connect ( pKnobReverbWidth, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblReverbWidthValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbWidthChanged ( value );
    } );
    QObject::connect ( pRbtReverbSelL, &QRadioButton::clicked, this, &CEffectsDlg::ReverbLeftSelected );
    QObject::connect ( pRbtReverbSelR, &QRadioButton::clicked, this, &CEffectsDlg::ReverbRightSelected );
    QObject::connect ( pChbReverbEarly, &QCheckBox::toggled, this, [this] ( bool enabled ) {
        pKnobReverbEarly->setEnabled ( enabled );
        emit ReverbEarlyEnabledChanged ( enabled );
    } );
    QObject::connect ( pChbReverbFreeze, &QCheckBox::toggled, this, &CEffectsDlg::ReverbFreezeChanged );
    QObject::connect ( pButReverbReset, &QPushButton::clicked, this, &CEffectsDlg::OnResetReverbClicked );

    QObject::connect ( pChbCompressorEnable, &QCheckBox::toggled, this, [this] ( bool enabled ) {
        emit CompressorBypassChanged ( !enabled );
        if ( pCompCurveWidget )
        {
            pCompCurveWidget->SetBypass ( !enabled );
        }
    } );
    QObject::connect ( pChbCompressorLimiter, &QCheckBox::toggled, this, [this] ( bool enabled ) {
        emit CompressorLimiterChanged ( enabled );
        if ( pCompCurveWidget )
        {
            pCompCurveWidget->SetLimiterEnabled ( enabled );
        }
    } );
    QObject::connect ( pKnobCompressorThreshold, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblCompressorThresholdValue->setText ( QString::number ( value ) + tr ( " dB" ) );
        emit CompressorThresholdChanged ( value );
        if ( pCompCurveWidget )
        {
            pCompCurveWidget->SetThreshold ( static_cast<float> ( value ) );
        }
    } );
    QObject::connect ( pKnobCompressorRatio, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblCompressorRatioValue->setText ( QString::number ( value ) + tr ( ":1" ) );
        emit CompressorRatioChanged ( value );
        if ( pCompCurveWidget )
        {
            pCompCurveWidget->SetRatio ( static_cast<float> ( value ) );
        }
    } );
    QObject::connect ( pKnobCompressorAttack, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblCompressorAttackValue->setText ( QString::number ( value ) + tr ( " ms" ) );
        emit CompressorAttackChanged ( value );
    } );
    QObject::connect ( pKnobCompressorRelease, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblCompressorReleaseValue->setText ( QString::number ( value ) + tr ( " ms" ) );
        emit CompressorReleaseChanged ( value );
    } );
    QObject::connect ( pKnobCompressorMakeup, &CCustomKnob::valueChanged, this, [this] ( int value ) {
        pLblCompressorMakeupValue->setText ( QString::number ( value ) + tr ( " dB" ) );
        emit CompressorMakeupChanged ( value );
        if ( pCompCurveWidget )
        {
            pCompCurveWidget->SetMakeup ( static_cast<float> ( value ) );
        }
    } );
    QObject::connect ( pButCompressorReset, &QPushButton::clicked, this, &CEffectsDlg::OnResetCompressorClicked );
    QObject::connect ( pChbEQEnable, &QCheckBox::toggled, this, [this] ( bool enabled ) {
        const bool bBypassed = !enabled;
        if ( pEQCurveWidget )
        {
            pEQCurveWidget->SetBypassed ( bBypassed );
        }
        emit EQBypassChanged ( bBypassed );
        UpdateEQDynControls ( iSelectedBand );
    } );
    QObject::connect ( pCbxEffectsPresets,
                       QOverload<int>::of ( &QComboBox::currentIndexChanged ),
                       this,
                       &CEffectsDlg::ApplyEffectsPresetFromComboIndex );
    QObject::connect ( pButEffectsSavePreset, &QPushButton::clicked, this, &CEffectsDlg::OnSaveEffectsPresetClicked );
    QObject::connect ( pButEffectsSaveAsPreset, &QPushButton::clicked, this, &CEffectsDlg::OnSaveAsEffectsPresetClicked );
    QObject::connect ( pButEffectsDeletePreset, &QPushButton::clicked, this, &CEffectsDlg::OnDeleteEffectsPresetClicked );
    QObject::connect ( pButEQReset, &QPushButton::clicked, this, &CEffectsDlg::OnResetEQClicked );

    QObject::connect ( pButContextInput, &QPushButton::clicked, this, &CEffectsDlg::OnContextInputClicked );
    QObject::connect ( pButContextOutput, &QPushButton::clicked, this, &CEffectsDlg::OnContextOutputClicked );

    SetContext ( EC_INPUT );
}

void CEffectsDlg::UpdateOutputBandLevels ( const CVector<float>& vecOutLevels )
{
    if ( pEQCurveWidget && pEQCurveWidget->isVisible() && pClient )
    {
        if ( GetEQBypass() )
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
            pEQCurveWidget->SetBandGainReduction ( iBand, GetEQBandGainReductionDb ( iBand ) );
        }
    }
}

void CEffectsDlg::UpdateCompressorGainReduction ( const float fGRDb, const float fInputDb )
{
    if ( pGRMeter )
    {
        pGRMeter->SetGainReductionDb ( fGRDb );
    }
    if ( pCompCurveWidget )
    {
        pCompCurveWidget->SetCurrentInputDb ( fInputDb );
    }
}

void CEffectsDlg::UpdateReverbOutputLevel ( const float fLevelDb )
{
    if ( pReverbDecayWidget )
    {
        pReverbDecayWidget->SetOutputLevelDb ( fLevelDb );
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

    QWidget* apThemeWidgets[] = {
        pKnobReverb,
        pKnobReverbPreDelay,
        pKnobReverbRoom,
        pKnobReverbDamping,
        pKnobReverbWet,
        pKnobReverbEarly,
        pKnobReverbWidth,
        pKnobCompressorThreshold,
        pKnobCompressorRatio,
        pKnobCompressorAttack,
        pKnobCompressorRelease,
        pKnobCompressorMakeup,
        pKnobEQDynThreshold,
        pKnobEQDynRatio,
        pKnobEQDynAttack,
        pKnobEQDynRelease,
        pKnobEQBandQ,
        pKnobEQBandGain,
    };

    for ( QWidget* pWidget : apThemeWidgets )
    {
        if ( pWidget )
        {
            if ( auto* pKnob = qobject_cast<CCustomKnob*> ( pWidget ) )
            {
                pKnob->SetDarkTheme ( bDarkTheme );
            }
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

    if ( pCompCurveWidget )
    {
        pCompCurveWidget->SetDarkTheme ( bDarkTheme );
    }

    if ( pReverbDecayWidget )
    {
        pReverbDecayWidget->SetDarkTheme ( bDarkTheme );
    }
}

void CEffectsDlg::UpdateCompressorControls()
{
    pChbCompressorEnable->blockSignals ( true );
    pChbCompressorEnable->setChecked ( !GetCompressorBypass() );
    pChbCompressorEnable->blockSignals ( false );

    pChbCompressorLimiter->blockSignals ( true );
    pChbCompressorLimiter->setChecked ( GetCompressorLimiterEnabled() );
    pChbCompressorLimiter->blockSignals ( false );

    pKnobCompressorThreshold->blockSignals ( true );
    pKnobCompressorThreshold->setValue ( static_cast<int> ( GetCompressorThresholdDb() ) );
    pKnobCompressorThreshold->blockSignals ( false );

    pKnobCompressorRatio->blockSignals ( true );
    pKnobCompressorRatio->setValue ( static_cast<int> ( GetCompressorRatio() ) );
    pKnobCompressorRatio->blockSignals ( false );

    pKnobCompressorAttack->blockSignals ( true );
    pKnobCompressorAttack->setValue ( static_cast<int> ( GetCompressorAttackMs() ) );
    pKnobCompressorAttack->blockSignals ( false );

    pKnobCompressorRelease->blockSignals ( true );
    pKnobCompressorRelease->setValue ( static_cast<int> ( GetCompressorReleaseMs() ) );
    pKnobCompressorRelease->blockSignals ( false );

    pKnobCompressorMakeup->blockSignals ( true );
    pKnobCompressorMakeup->setValue ( static_cast<int> ( GetCompressorMakeupDb() ) );
    pKnobCompressorMakeup->blockSignals ( false );

    pKnobCompressorThreshold->setEnabled ( true );
    pKnobCompressorRatio->setEnabled ( true );
    pKnobCompressorAttack->setEnabled ( true );
    pKnobCompressorRelease->setEnabled ( true );
    pKnobCompressorMakeup->setEnabled ( true );
    pChbCompressorLimiter->setEnabled ( true );
    pButCompressorReset->setEnabled ( true );

    pLblCompressorThreshold->setEnabled ( true );
    pLblCompressorThresholdValue->setEnabled ( true );
    pLblCompressorRatio->setEnabled ( true );
    pLblCompressorRatioValue->setEnabled ( true );
    pLblCompressorAttack->setEnabled ( true );
    pLblCompressorAttackValue->setEnabled ( true );
    pLblCompressorRelease->setEnabled ( true );
    pLblCompressorReleaseValue->setEnabled ( true );
    pLblCompressorMakeup->setEnabled ( true );
    pLblCompressorMakeupValue->setEnabled ( true );

    pLblCompressorThresholdValue->setText ( QString::number ( pKnobCompressorThreshold->value() ) + tr ( " dB" ) );
    pLblCompressorRatioValue->setText ( QString::number ( pKnobCompressorRatio->value() ) + tr ( ":1" ) );
    pLblCompressorAttackValue->setText ( QString::number ( pKnobCompressorAttack->value() ) + tr ( " ms" ) );
    pLblCompressorReleaseValue->setText ( QString::number ( pKnobCompressorRelease->value() ) + tr ( " ms" ) );
    pLblCompressorMakeupValue->setText ( QString::number ( pKnobCompressorMakeup->value() ) + tr ( " dB" ) );

    if ( pCompCurveWidget )
    {
        pCompCurveWidget->SetThreshold ( GetCompressorThresholdDb() );
        pCompCurveWidget->SetRatio ( GetCompressorRatio() );
        pCompCurveWidget->SetMakeup ( GetCompressorMakeupDb() );
        pCompCurveWidget->SetBypass ( GetCompressorBypass() );
        pCompCurveWidget->SetLimiterEnabled ( GetCompressorLimiterEnabled() );
    }
}

void CEffectsDlg::UpdateReverbControls()
{
    pKnobReverb->blockSignals ( true );
    pKnobReverb->setValue ( GetReverbLevel() );
    pKnobReverb->blockSignals ( false );

    pKnobReverbPreDelay->blockSignals ( true );
    pKnobReverbPreDelay->setValue ( GetReverbPreDelayMs() );
    pKnobReverbPreDelay->blockSignals ( false );

    pKnobReverbRoom->blockSignals ( true );
    pKnobReverbRoom->setValue ( GetReverbRoomSize() );
    pKnobReverbRoom->blockSignals ( false );

    pKnobReverbDamping->blockSignals ( true );
    pKnobReverbDamping->setValue ( GetReverbDamping() );
    pKnobReverbDamping->blockSignals ( false );

    pKnobReverbWet->blockSignals ( true );
    pKnobReverbWet->setValue ( GetReverbWetMix() );
    pKnobReverbWet->blockSignals ( false );

    pKnobReverbEarly->blockSignals ( true );
    pKnobReverbEarly->setValue ( GetReverbEarlyLevel() );
    pKnobReverbEarly->blockSignals ( false );

    pKnobReverbWidth->blockSignals ( true );
    pKnobReverbWidth->setValue ( GetReverbWidth() );
    pKnobReverbWidth->blockSignals ( false );

    pChbReverbEnable->blockSignals ( true );
    pChbReverbEnable->setChecked ( !GetReverbBypass() );
    pChbReverbEnable->blockSignals ( false );

    pChbReverbEarly->blockSignals ( true );
    pChbReverbEarly->setChecked ( GetReverbEarlyEnabled() );
    pChbReverbEarly->blockSignals ( false );
    pKnobReverbEarly->setEnabled ( GetReverbEarlyEnabled() );

    pChbReverbFreeze->blockSignals ( true );
    pChbReverbFreeze->setChecked ( GetReverbFreeze() );
    pChbReverbFreeze->blockSignals ( false );

    pKnobReverb->setEnabled ( true );
    pKnobReverbPreDelay->setEnabled ( true );
    pKnobReverbRoom->setEnabled ( true );
    pKnobReverbDamping->setEnabled ( true );
    pKnobReverbWet->setEnabled ( true );
    pKnobReverbEarly->setEnabled ( GetReverbEarlyEnabled() );
    pKnobReverbWidth->setEnabled ( true );
    pChbReverbEarly->setEnabled ( true );
    pChbReverbFreeze->setEnabled ( true );
    pRbtReverbSelL->setEnabled ( true );
    pRbtReverbSelR->setEnabled ( true );
    pButReverbReset->setEnabled ( true );

    pLblReverb->setEnabled ( true );
    pLblReverbValue->setEnabled ( true );
    pLblReverbPreDelay->setEnabled ( true );
    pLblReverbPreDelayValue->setEnabled ( true );
    pLblReverbWet->setEnabled ( true );
    pLblReverbWetValue->setEnabled ( true );
    pLblReverbRoom->setEnabled ( true );
    pLblReverbRoomValue->setEnabled ( true );
    pLblReverbEarly->setEnabled ( true );
    pLblReverbEarlyValue->setEnabled ( true );
    pLblReverbDamping->setEnabled ( true );
    pLblReverbDampingValue->setEnabled ( true );
    pLblReverbWidth->setEnabled ( true );
    pLblReverbWidthValue->setEnabled ( true );
    pLblStereoHint->setEnabled ( true );

    pLblReverbValue->setText ( QString::number ( pKnobReverb->value() ) + tr ( " %" ) );
    pLblReverbPreDelayValue->setText ( QString::number ( pKnobReverbPreDelay->value() ) + tr ( " ms" ) );
    pLblReverbRoomValue->setText ( QString::number ( pKnobReverbRoom->value() ) + tr ( " %" ) );
    pLblReverbDampingValue->setText ( QString::number ( pKnobReverbDamping->value() ) + tr ( " %" ) );
    pLblReverbWetValue->setText ( QString::number ( pKnobReverbWet->value() ) + tr ( " %" ) );
    pLblReverbEarlyValue->setText ( QString::number ( pKnobReverbEarly->value() ) + tr ( " %" ) );
    pLblReverbWidthValue->setText ( QString::number ( pKnobReverbWidth->value() ) + tr ( " %" ) );

    const bool bShowChannelSelection = ( eCurrentContext == EC_INPUT ) && ( pClient->GetAudioChannels() != CC_STEREO );

    pRbtReverbSelL->setVisible ( bShowChannelSelection );
    pRbtReverbSelR->setVisible ( bShowChannelSelection );
    pLblStereoHint->setVisible ( !bShowChannelSelection );

    if ( bShowChannelSelection )
    {
        if ( IsReverbOnLeftChan() )
        {
            pRbtReverbSelL->setChecked ( true );
        }
        else
        {
            pRbtReverbSelR->setChecked ( true );
        }
    }
    if ( pReverbDecayWidget )
    {
        pReverbDecayWidget->SetPreDelayMs ( static_cast<float> ( pKnobReverbPreDelay->value() ) );
        pReverbDecayWidget->SetRoomSize ( static_cast<float> ( pKnobReverbRoom->value() ) / REVERB_ROOM_SIZE_MAX );
        pReverbDecayWidget->SetDamping ( static_cast<float> ( pKnobReverbDamping->value() ) / REVERB_DAMPING_MAX );
        pReverbDecayWidget->SetWetMix ( static_cast<float> ( pKnobReverbWet->value() ) / REVERB_WET_MIX_MAX );
        pReverbDecayWidget->SetEarlyLevel ( static_cast<float> ( pKnobReverbEarly->value() ) / REVERB_EARLY_LEVEL_MAX );
        pReverbDecayWidget->SetBypass ( GetReverbBypass() );
    }
}

void CEffectsDlg::UpdateEQControls()
{
    const bool bBypassed = GetEQBypass();
    pChbEQEnable->blockSignals ( true );
    pChbEQEnable->setChecked ( !bBypassed );
    pChbEQEnable->blockSignals ( false );
    pButEQReset->setEnabled ( true );

    if ( pEQCurveWidget && pClient )
    {
        pEQCurveWidget->SetBypassed ( bBypassed );
        for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
        {
            pEQCurveWidget->SetBandGain ( iBand, GetEQBandGainDb ( iBand ) );
            pEQCurveWidget->SetBandFrequency ( iBand, GetEQBandFrequency ( iBand ) );
            pEQCurveWidget->SetBandQ ( iBand, GetEQBandQ ( iBand ) );
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
        const QString strName = GetEffectsPresetNames()[iPreset];
        if ( strName.isEmpty() )
        {
            continue;
        }

        pCbxEffectsPresets->addItem ( strName );
        const int iCurrentItemIdx = pCbxEffectsPresets->count() - 1;
        pCbxEffectsPresets->setItemData ( iCurrentItemIdx, iPreset, Qt::UserRole );

        if ( GetSelectedEffectsPreset() != INVALID_INDEX && iPreset == GetSelectedEffectsPreset() )
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

void CEffectsDlg::ApplyEffectsPreset ( const int iPresetSlot ) { ApplyEffectsPresetFromSlot ( iPresetSlot ); }

void CEffectsDlg::ApplyEffectsPresetFromSlot ( const int iPresetSlot )
{
    if ( iPresetSlot < 0 || iPresetSlot >= MAX_NUM_EFFECT_PRESETS )
    {
        return;
    }

    GetSelectedEffectsPreset() = iPresetSlot;

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

    const int             ctx    = ( eCurrentContext == EC_OUTPUT ) ? 1 : 0;
    const SEffectsPreset& preset = pSettings->EffectsPresets[ctx][iPresetSlot];

    SetReverbLevel ( preset.iReverbLevel );
    SetReverbOnLeftChan ( preset.bReverbOnLeftChan );
    SetReverbPreDelayMs ( preset.iReverbPreDelayMs );
    SetReverbRoomSize ( preset.iReverbRoomSize );
    SetReverbDamping ( preset.iReverbDamping );
    SetReverbWetMix ( preset.iReverbWetMix );
    SetReverbEarlyLevel ( preset.iReverbEarlyLevel );
    SetReverbWidth ( preset.iReverbWidth );
    SetReverbEarlyEnabled ( preset.bReverbEarlyEnabled );
    SetReverbFreeze ( preset.bReverbFreeze );
    SetReverbBypass ( preset.bReverbBypass );

    SetCompressorBypass ( preset.bCompressorBypass );
    SetCompressorThresholdDb ( static_cast<float> ( preset.iCompressorThresholdDb ) );
    SetCompressorRatio ( static_cast<float> ( preset.iCompressorRatio ) );
    SetCompressorAttackMs ( static_cast<float> ( preset.iCompressorAttackMs ) );
    SetCompressorReleaseMs ( static_cast<float> ( preset.iCompressorReleaseMs ) );
    SetCompressorMakeupDb ( static_cast<float> ( preset.iCompressorMakeupDb ) );
    SetCompressorLimiterEnabled ( preset.bCompressorLimiterEnabled );

    SetEQBypass ( preset.bEQBypass );
    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        SetEQBandGainDb ( iBand, preset.afEQBandGainDb[iBand] );
        SetEQBandFrequency ( iBand, preset.aiEQBandFrequency[iBand] );
        SetEQBandDynEnabled ( iBand, preset.abEQBandDynEnabled[iBand] );
        SetEQBandDynThresholdDb ( iBand, preset.aiEQBandDynThresholdDb[iBand] );
        SetEQBandDynRatio ( iBand, preset.aiEQBandDynRatio[iBand] );
        SetEQBandDynAttackMs ( iBand, preset.aiEQBandDynAttackMs[iBand] );
        SetEQBandDynReleaseMs ( iBand, preset.aiEQBandDynReleaseMs[iBand] );
        SetEQBandQ ( iBand, static_cast<float> ( preset.aiEQBandQ[iBand] ) / 10.0f );
    }

    UpdateReverbControls();
    UpdateCompressorControls();
    UpdateEQControls();
}

int CEffectsDlg::FindEffectsPresetSlotByName ( const QString& strName ) const
{
    for ( int iPreset = 0; iPreset < MAX_NUM_EFFECT_PRESETS; ++iPreset )
    {
        if ( GetEffectsPresetNames()[iPreset].compare ( strName, Qt::CaseInsensitive ) == 0 )
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
        if ( GetEffectsPresetNames()[iPreset].isEmpty() )
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

    const QString strName                = pCbxEffectsPresets->itemText ( iComboIndex );
    GetEffectsPresetNames()[iPresetSlot] = strName;

    pSettings->SaveEffectsPresetFromClient ( iPresetSlot, eCurrentContext == EC_OUTPUT );

    PopulateEffectsPresetCombo();
    const int iUpdatedIndex = pCbxEffectsPresets->findText ( strName );
    if ( iUpdatedIndex >= 0 )
    {
        pCbxEffectsPresets->setCurrentIndex ( iUpdatedIndex );
    }
}

void CEffectsDlg::OnResetReverbClicked()
{
    SetReverbLevel ( 0 );
    SetReverbOnLeftChan ( false );
    SetReverbPreDelayMs ( 0 );
    SetReverbRoomSize ( 60 );
    SetReverbDamping ( 30 );
    SetReverbWetMix ( 25 );
    SetReverbEarlyLevel ( 30 );
    SetReverbWidth ( 100 );
    SetReverbEarlyEnabled ( true );
    SetReverbFreeze ( false );
    SetReverbBypass ( true );

    UpdateReverbControls();
    // Remove focus from the button to prevent blue outline
    pTabs->setFocus();
}

void CEffectsDlg::OnResetCompressorClicked()
{
    SetCompressorBypass ( true );
    SetCompressorThresholdDb ( -12.0f );
    SetCompressorRatio ( 3.0f );
    SetCompressorAttackMs ( 5.0f );
    SetCompressorReleaseMs ( 120.0f );
    SetCompressorMakeupDb ( 3.0f );
    SetCompressorLimiterEnabled ( true );

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

    GetEffectsPresetNames()[iPresetSlot] = strName;

    pSettings->SaveEffectsPresetFromClient ( iPresetSlot, eCurrentContext == EC_OUTPUT );

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

    GetEffectsPresetNames()[iPresetSlot].clear();

    const int ctx                               = ( eCurrentContext == EC_OUTPUT ) ? 1 : 0;
    pSettings->EffectsPresets[ctx][iPresetSlot] = SEffectsPreset();

    if ( iPresetSlot == GetSelectedEffectsPreset() )
    {
        GetSelectedEffectsPreset() = INVALID_INDEX;
    }

    PopulateEffectsPresetCombo();
}

void CEffectsDlg::OnEQBandGainChanged ( int iBand, float fGainDb )
{
    if ( pClient )
    {
        SetEQBandGainDb ( iBand, fGainDb );
        if ( iBand == iSelectedBand )
        {
            UpdateEQDynControls ( iBand );
        }
    }
}

void CEffectsDlg::OnEQBandFrequencyChanged ( int iBand, float fFreqHz )
{
    if ( pClient )
    {
        SetEQBandFrequency ( iBand, fFreqHz );
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
        SetEQBandGainDb ( iBand, 0.0f );
        if ( iBand == iSelectedBand )
        {
            UpdateEQDynControls ( iBand );
        }
    }
}

void CEffectsDlg::OnEQDynEnabledChanged ( bool bEnabled )
{
    if ( pClient )
    {
        SetEQBandDynEnabled ( iSelectedBand, bEnabled );
        UpdateEQDynControls ( iSelectedBand );
    }
}

void CEffectsDlg::OnEQDynThresholdChanged ( int iValue )
{
    if ( pClient )
    {
        SetEQBandDynThresholdDb ( iSelectedBand, static_cast<float> ( iValue ) );
        if ( pEdtEQDynThreshold->isReadOnly() )
        {
            pEdtEQDynThreshold->setText ( QString ( "%1 dB" ).arg ( iValue ) );
        }
    }
}

void CEffectsDlg::OnEQDynRatioChanged ( int iValue )
{
    if ( pClient )
    {
        SetEQBandDynRatio ( iSelectedBand, static_cast<float> ( iValue ) );
        if ( pEdtEQDynRatio->isReadOnly() )
        {
            pEdtEQDynRatio->setText ( QString ( "%1:1" ).arg ( iValue ) );
        }
    }
}

void CEffectsDlg::OnEQDynAttackChanged ( int iValue )
{
    if ( pClient )
    {
        SetEQBandDynAttackMs ( iSelectedBand, static_cast<float> ( iValue ) );
        if ( pEdtEQDynAttack->isReadOnly() )
        {
            pEdtEQDynAttack->setText ( QString ( "%1 ms" ).arg ( iValue ) );
        }
    }
}

void CEffectsDlg::OnEQDynReleaseChanged ( int iValue )
{
    if ( pClient )
    {
        SetEQBandDynReleaseMs ( iSelectedBand, static_cast<float> ( iValue ) );
        if ( pEdtEQDynRelease->isReadOnly() )
        {
            pEdtEQDynRelease->setText ( QString ( "%1 ms" ).arg ( iValue ) );
        }
    }
}

void CEffectsDlg::OnEQBandQChanged ( int iValue )
{
    if ( pClient )
    {
        const float fQ = static_cast<float> ( iValue ) / 10.0f;
        SetEQBandQ ( iSelectedBand, fQ );
        if ( pEdtEQBandQ->isReadOnly() )
        {
            pEdtEQBandQ->setText ( QString::number ( fQ, 'f', 1 ) );
        }
        if ( pEQCurveWidget )
        {
            pEQCurveWidget->SetBandQ ( iSelectedBand, fQ );
        }
    }
}

void CEffectsDlg::OnEQBandGainKnobChanged ( int iValue )
{
    if ( pClient )
    {
        const float fGainDb = static_cast<float> ( iValue ) / 10.0f;
        SetEQBandGainDb ( iSelectedBand, fGainDb );
        if ( pEQCurveWidget )
        {
            pEQCurveWidget->SetBandGain ( iSelectedBand, fGainDb );
        }
        UpdateEQDynControls ( iSelectedBand );
    }
}

void CEffectsDlg::OnResetEQClicked()
{
    if ( pClient )
    {
        pClient->GetEQ ( eCurrentContext == EC_OUTPUT ).Reset();
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

    const bool bEnabled    = GetEQBandDynEnabled ( iBand );
    const bool bDynEnabled = bEnabled;

    // Apply per-band accent color to all EQ knobs
    const QColor colBand = GetBandColor ( iBand );
    pKnobEQDynThreshold->SetAccentColor ( colBand );
    pKnobEQDynRatio->SetAccentColor ( colBand );
    pKnobEQDynAttack->SetAccentColor ( colBand );
    pKnobEQDynRelease->SetAccentColor ( colBand );
    pKnobEQBandQ->SetAccentColor ( colBand );
    pKnobEQBandGain->SetAccentColor ( colBand );

    pChbEQDynEnabled->blockSignals ( true );
    pKnobEQDynThreshold->blockSignals ( true );
    pKnobEQDynRatio->blockSignals ( true );
    pKnobEQDynAttack->blockSignals ( true );
    pKnobEQDynRelease->blockSignals ( true );
    pKnobEQBandQ->blockSignals ( true );
    pKnobEQBandGain->blockSignals ( true );

    pChbEQDynEnabled->setChecked ( bEnabled );

    const float fFreq = GetEQBandFrequency ( iBand );
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

    const float fGainDb = GetEQBandGainDb ( iBand );
    if ( pEdtEQDynGain->isReadOnly() )
    {
        QString strGain = ( fGainDb > 0.0f ) ? QString ( "+%1 dB" ).arg ( QString::number ( fGainDb, 'f', 1 ) )
                                             : QString ( "%1 dB" ).arg ( QString::number ( fGainDb, 'f', 1 ) );
        pEdtEQDynGain->blockSignals ( true );
        pEdtEQDynGain->setReadOnly ( true );
        pEdtEQDynGain->setFrame ( false );
        pEdtEQDynGain->setText ( strGain );
        pEdtEQDynGain->blockSignals ( false );
    }
    pKnobEQBandGain->setValue ( static_cast<int> ( std::round ( fGainDb * 10.0f ) ) );

    const float fThreshold = GetEQBandDynThresholdDb ( iBand );
    pKnobEQDynThreshold->setValue ( static_cast<int> ( std::round ( fThreshold ) ) );
    if ( pEdtEQDynThreshold->isReadOnly() || !bDynEnabled )
    {
        pEdtEQDynThreshold->blockSignals ( true );
        pEdtEQDynThreshold->setReadOnly ( true );
        pEdtEQDynThreshold->setFrame ( false );
        pEdtEQDynThreshold->setText ( QString ( "%1 dB" ).arg ( static_cast<int> ( std::round ( fThreshold ) ) ) );
        pEdtEQDynThreshold->blockSignals ( false );
    }

    const float fRatio = GetEQBandDynRatio ( iBand );
    pKnobEQDynRatio->setValue ( static_cast<int> ( std::round ( fRatio ) ) );
    if ( pEdtEQDynRatio->isReadOnly() || !bDynEnabled )
    {
        pEdtEQDynRatio->blockSignals ( true );
        pEdtEQDynRatio->setReadOnly ( true );
        pEdtEQDynRatio->setFrame ( false );
        pEdtEQDynRatio->setText ( QString ( "%1:1" ).arg ( static_cast<int> ( std::round ( fRatio ) ) ) );
        pEdtEQDynRatio->blockSignals ( false );
    }

    const float fAttack = GetEQBandDynAttackMs ( iBand );
    pKnobEQDynAttack->setValue ( static_cast<int> ( std::round ( fAttack ) ) );
    if ( pEdtEQDynAttack->isReadOnly() || !bDynEnabled )
    {
        pEdtEQDynAttack->blockSignals ( true );
        pEdtEQDynAttack->setReadOnly ( true );
        pEdtEQDynAttack->setFrame ( false );
        pEdtEQDynAttack->setText ( QString ( "%1 ms" ).arg ( static_cast<int> ( std::round ( fAttack ) ) ) );
        pEdtEQDynAttack->blockSignals ( false );
    }

    const float fRelease = GetEQBandDynReleaseMs ( iBand );
    pKnobEQDynRelease->setValue ( static_cast<int> ( std::round ( fRelease ) ) );
    if ( pEdtEQDynRelease->isReadOnly() || !bDynEnabled )
    {
        pEdtEQDynRelease->blockSignals ( true );
        pEdtEQDynRelease->setReadOnly ( true );
        pEdtEQDynRelease->setFrame ( false );
        pEdtEQDynRelease->setText ( QString ( "%1 ms" ).arg ( static_cast<int> ( std::round ( fRelease ) ) ) );
        pEdtEQDynRelease->blockSignals ( false );
    }

    const float fQ    = GetEQBandQ ( iBand );
    const int   iQInt = static_cast<int> ( std::round ( fQ * 10.0f ) );
    pKnobEQBandQ->setValue ( iQInt );
    if ( pEdtEQBandQ->isReadOnly() )
    {
        pEdtEQBandQ->blockSignals ( true );
        pEdtEQBandQ->setReadOnly ( true );
        pEdtEQBandQ->setFrame ( false );
        pEdtEQBandQ->setText ( QString::number ( fQ, 'f', 1 ) );
        pEdtEQBandQ->blockSignals ( false );
    }

    pChbEQDynEnabled->blockSignals ( false );
    pKnobEQDynThreshold->blockSignals ( false );
    pKnobEQDynRatio->blockSignals ( false );
    pKnobEQDynAttack->blockSignals ( false );
    pKnobEQDynRelease->blockSignals ( false );
    pKnobEQBandQ->blockSignals ( false );
    pKnobEQBandGain->blockSignals ( false );

    pChbEQDynEnabled->setEnabled ( true );
    pKnobEQBandQ->setEnabled ( true );
    pKnobEQBandGain->setEnabled ( true );
    pEdtEQDynFreq->setEnabled ( true );
    pEdtEQDynGain->setEnabled ( true );
    pEdtEQBandQ->setEnabled ( true );
    pLblEQDynFreqPrefix->setEnabled ( true );
    pLblEQDynGainPrefix->setEnabled ( true );
    pLblEQBandQ->setEnabled ( true );

    pKnobEQDynThreshold->setEnabled ( bDynEnabled );
    pKnobEQDynRatio->setEnabled ( bDynEnabled );
    pKnobEQDynAttack->setEnabled ( bDynEnabled );
    pKnobEQDynRelease->setEnabled ( bDynEnabled );
    pEdtEQDynThreshold->setEnabled ( bDynEnabled );
    pEdtEQDynRatio->setEnabled ( bDynEnabled );
    pEdtEQDynAttack->setEnabled ( bDynEnabled );
    pEdtEQDynRelease->setEnabled ( bDynEnabled );
    pLblEQDynThreshold->setEnabled ( bDynEnabled );
    pLblEQDynRatio->setEnabled ( bDynEnabled );
    pLblEQDynAttack->setEnabled ( bDynEnabled );
    pLblEQDynRelease->setEnabled ( bDynEnabled );
}

bool CEffectsDlg::eventFilter ( QObject* pObj, QEvent* pEvent )
{
    if ( pObj == pEdtEQDynFreq )
    {
        if ( pEvent->type() == QEvent::MouseButtonPress || pEvent->type() == QEvent::MouseButtonDblClick )
        {
            if ( pEdtEQDynFreq->isEnabled() && pEdtEQDynFreq->isReadOnly() )
            {
                // Enter edit mode: show raw Hz value so user can type precisely
                const float fFreq = pClient ? GetEQBandFrequency ( iSelectedBand ) : 0.0f;
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
    else if ( pObj == pEdtEQDynGain )
    {
        if ( pEvent->type() == QEvent::MouseButtonPress || pEvent->type() == QEvent::MouseButtonDblClick )
        {
            if ( pEdtEQDynGain->isEnabled() && pEdtEQDynGain->isReadOnly() )
            {
                // Enter edit mode: show raw dB value so user can type precisely
                const float fGain = pClient ? GetEQBandGainDb ( iSelectedBand ) : 0.0f;
                pEdtEQDynGain->setReadOnly ( false );
                pEdtEQDynGain->setFrame ( true );
                pEdtEQDynGain->setText ( QString::number ( fGain, 'f', 1 ) );
                pEdtEQDynGain->selectAll();
                pEdtEQDynGain->setFocus();
                return true;
            }
        }
        else if ( pEvent->type() == QEvent::FocusOut )
        {
            if ( !pEdtEQDynGain->isReadOnly() )
            {
                OnEQDynGainEditFinished();
                return true;
            }
        }
    }
    else if ( pObj == pEdtEQBandQ )
    {
        if ( pEvent->type() == QEvent::MouseButtonPress || pEvent->type() == QEvent::MouseButtonDblClick )
        {
            if ( pEdtEQBandQ->isEnabled() && pEdtEQBandQ->isReadOnly() )
            {
                const float fQ = pClient ? GetEQBandQ ( iSelectedBand ) : 1.0f;
                pEdtEQBandQ->setReadOnly ( false );
                pEdtEQBandQ->setFrame ( true );
                pEdtEQBandQ->setText ( QString::number ( fQ, 'f', 1 ) );
                pEdtEQBandQ->selectAll();
                pEdtEQBandQ->setFocus();
                return true;
            }
        }
        else if ( pEvent->type() == QEvent::FocusOut )
        {
            if ( !pEdtEQBandQ->isReadOnly() )
            {
                OnEQBandQEditFinished();
                return true;
            }
        }
    }
    else if ( pObj == pEdtEQDynThreshold )
    {
        if ( pEvent->type() == QEvent::MouseButtonPress || pEvent->type() == QEvent::MouseButtonDblClick )
        {
            if ( pEdtEQDynThreshold->isEnabled() && pEdtEQDynThreshold->isReadOnly() )
            {
                const float fThreshold = pClient ? GetEQBandDynThresholdDb ( iSelectedBand ) : 0.0f;
                pEdtEQDynThreshold->setReadOnly ( false );
                pEdtEQDynThreshold->setFrame ( true );
                pEdtEQDynThreshold->setText ( QString::number ( static_cast<int> ( std::round ( fThreshold ) ) ) );
                pEdtEQDynThreshold->selectAll();
                pEdtEQDynThreshold->setFocus();
                return true;
            }
        }
        else if ( pEvent->type() == QEvent::FocusOut )
        {
            if ( !pEdtEQDynThreshold->isReadOnly() )
            {
                OnEQDynThresholdEditFinished();
                return true;
            }
        }
    }
    else if ( pObj == pEdtEQDynRatio )
    {
        if ( pEvent->type() == QEvent::MouseButtonPress || pEvent->type() == QEvent::MouseButtonDblClick )
        {
            if ( pEdtEQDynRatio->isEnabled() && pEdtEQDynRatio->isReadOnly() )
            {
                const float fRatio = pClient ? GetEQBandDynRatio ( iSelectedBand ) : 1.0f;
                pEdtEQDynRatio->setReadOnly ( false );
                pEdtEQDynRatio->setFrame ( true );
                pEdtEQDynRatio->setText ( QString::number ( static_cast<int> ( std::round ( fRatio ) ) ) );
                pEdtEQDynRatio->selectAll();
                pEdtEQDynRatio->setFocus();
                return true;
            }
        }
        else if ( pEvent->type() == QEvent::FocusOut )
        {
            if ( !pEdtEQDynRatio->isReadOnly() )
            {
                OnEQDynRatioEditFinished();
                return true;
            }
        }
    }
    else if ( pObj == pEdtEQDynAttack )
    {
        if ( pEvent->type() == QEvent::MouseButtonPress || pEvent->type() == QEvent::MouseButtonDblClick )
        {
            if ( pEdtEQDynAttack->isEnabled() && pEdtEQDynAttack->isReadOnly() )
            {
                const float fAttack = pClient ? GetEQBandDynAttackMs ( iSelectedBand ) : 1.0f;
                pEdtEQDynAttack->setReadOnly ( false );
                pEdtEQDynAttack->setFrame ( true );
                pEdtEQDynAttack->setText ( QString::number ( static_cast<int> ( std::round ( fAttack ) ) ) );
                pEdtEQDynAttack->selectAll();
                pEdtEQDynAttack->setFocus();
                return true;
            }
        }
        else if ( pEvent->type() == QEvent::FocusOut )
        {
            if ( !pEdtEQDynAttack->isReadOnly() )
            {
                OnEQDynAttackEditFinished();
                return true;
            }
        }
    }
    else if ( pObj == pEdtEQDynRelease )
    {
        if ( pEvent->type() == QEvent::MouseButtonPress || pEvent->type() == QEvent::MouseButtonDblClick )
        {
            if ( pEdtEQDynRelease->isEnabled() && pEdtEQDynRelease->isReadOnly() )
            {
                const float fRelease = pClient ? GetEQBandDynReleaseMs ( iSelectedBand ) : 10.0f;
                pEdtEQDynRelease->setReadOnly ( false );
                pEdtEQDynRelease->setFrame ( true );
                pEdtEQDynRelease->setText ( QString::number ( static_cast<int> ( std::round ( fRelease ) ) ) );
                pEdtEQDynRelease->selectAll();
                pEdtEQDynRelease->setFocus();
                return true;
            }
        }
        else if ( pEvent->type() == QEvent::FocusOut )
        {
            if ( !pEdtEQDynRelease->isReadOnly() )
            {
                OnEQDynReleaseEditFinished();
                return true;
            }
        }
    }

    return CBaseDlg::eventFilter ( pObj, pEvent );
}

void CEffectsDlg::mousePressEvent ( QMouseEvent* pEvent )
{
    QWidget* pFocusWidget = focusWidget();
    if ( pFocusWidget && qobject_cast<QLineEdit*> ( pFocusWidget ) )
    {
        pFocusWidget->clearFocus();
    }
    CBaseDlg::mousePressEvent ( pEvent );
}

void CEffectsDlg::OnEQDynFreqEditFinished()
{
    pEdtEQDynFreq->setReadOnly ( true );
    pEdtEQDynFreq->setFrame ( false );
    pEdtEQDynFreq->clearFocus();

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
        const float fMinFreq = GetEQBandFrequency ( iSelectedBand - 1 ) * 1.10f;
        fFreqHz              = std::max ( fFreqHz, fMinFreq );
    }

    if ( iSelectedBand < CAudioEqualizer::NUM_BANDS - 1 )
    {
        const float fMaxFreq = GetEQBandFrequency ( iSelectedBand + 1 ) * 0.90f;
        fFreqHz              = std::min ( fFreqHz, fMaxFreq );
    }

    SetEQBandFrequency ( iSelectedBand, fFreqHz );

    if ( pEQCurveWidget )
    {
        pEQCurveWidget->SetBandFrequency ( iSelectedBand, fFreqHz );
    }

    UpdateEQDynControls ( iSelectedBand );
}

void CEffectsDlg::OnEQDynGainEditFinished()
{
    pEdtEQDynGain->setReadOnly ( true );
    pEdtEQDynGain->setFrame ( false );
    pEdtEQDynGain->clearFocus();

    if ( !pClient || iSelectedBand < 0 || iSelectedBand >= CAudioEqualizer::NUM_BANDS )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    bool  bOk     = false;
    float fGainDb = pEdtEQDynGain->text().toFloat ( &bOk );

    if ( !bOk )
    {
        // Invalid input – revert display
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    // Clamp to valid range (kGainMinDb to kGainMaxDb)
    fGainDb = std::max ( CEQCurveWidget::kGainMinDb, std::min ( CEQCurveWidget::kGainMaxDb, fGainDb ) );

    SetEQBandGainDb ( iSelectedBand, fGainDb );

    if ( pEQCurveWidget )
    {
        pEQCurveWidget->SetBandGain ( iSelectedBand, fGainDb );
    }

    UpdateEQDynControls ( iSelectedBand );
}

void CEffectsDlg::OnEQBandQEditFinished()
{
    pEdtEQBandQ->setReadOnly ( true );
    pEdtEQBandQ->setFrame ( false );
    pEdtEQBandQ->clearFocus();

    if ( !pClient || iSelectedBand < 0 || iSelectedBand >= CAudioEqualizer::NUM_BANDS )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    bool  bOk = false;
    float fQ  = pEdtEQBandQ->text().toFloat ( &bOk );

    if ( !bOk )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    fQ = std::max ( 0.3f, std::min ( 10.0f, fQ ) );

    SetEQBandQ ( iSelectedBand, fQ );

    if ( pEQCurveWidget )
    {
        pEQCurveWidget->SetBandQ ( iSelectedBand, fQ );
    }

    UpdateEQDynControls ( iSelectedBand );
}

void CEffectsDlg::OnEQDynThresholdEditFinished()
{
    pEdtEQDynThreshold->setReadOnly ( true );
    pEdtEQDynThreshold->setFrame ( false );
    pEdtEQDynThreshold->clearFocus();

    if ( !pClient || iSelectedBand < 0 || iSelectedBand >= CAudioEqualizer::NUM_BANDS )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    bool  bOk        = false;
    float fThreshold = pEdtEQDynThreshold->text().toFloat ( &bOk );

    if ( !bOk )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    fThreshold = std::max ( -60.0f, std::min ( 0.0f, fThreshold ) );

    SetEQBandDynThresholdDb ( iSelectedBand, fThreshold );

    UpdateEQDynControls ( iSelectedBand );
}

void CEffectsDlg::OnEQDynRatioEditFinished()
{
    pEdtEQDynRatio->setReadOnly ( true );
    pEdtEQDynRatio->setFrame ( false );
    pEdtEQDynRatio->clearFocus();

    if ( !pClient || iSelectedBand < 0 || iSelectedBand >= CAudioEqualizer::NUM_BANDS )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    bool  bOk    = false;
    float fRatio = pEdtEQDynRatio->text().toFloat ( &bOk );

    if ( !bOk )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    fRatio = std::max ( 1.0f, std::min ( 20.0f, fRatio ) );

    SetEQBandDynRatio ( iSelectedBand, fRatio );

    UpdateEQDynControls ( iSelectedBand );
}

void CEffectsDlg::OnEQDynAttackEditFinished()
{
    pEdtEQDynAttack->setReadOnly ( true );
    pEdtEQDynAttack->setFrame ( false );
    pEdtEQDynAttack->clearFocus();

    if ( !pClient || iSelectedBand < 0 || iSelectedBand >= CAudioEqualizer::NUM_BANDS )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    bool  bOk     = false;
    float fAttack = pEdtEQDynAttack->text().toFloat ( &bOk );

    if ( !bOk )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    fAttack = std::max ( 1.0f, std::min ( 100.0f, fAttack ) );

    SetEQBandDynAttackMs ( iSelectedBand, fAttack );

    UpdateEQDynControls ( iSelectedBand );
}

void CEffectsDlg::OnEQDynReleaseEditFinished()
{
    pEdtEQDynRelease->setReadOnly ( true );
    pEdtEQDynRelease->setFrame ( false );
    pEdtEQDynRelease->clearFocus();

    if ( !pClient || iSelectedBand < 0 || iSelectedBand >= CAudioEqualizer::NUM_BANDS )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    bool  bOk      = false;
    float fRelease = pEdtEQDynRelease->text().toFloat ( &bOk );

    if ( !bOk )
    {
        UpdateEQDynControls ( iSelectedBand );
        return;
    }

    fRelease = std::max ( 10.0f, std::min ( 500.0f, fRelease ) );

    pClient->SetEQBandDynReleaseMs ( iSelectedBand, fRelease );

    UpdateEQDynControls ( iSelectedBand );
}

void CEffectsDlg::OnContextInputClicked() { SetContext ( EC_INPUT ); }

void CEffectsDlg::OnContextOutputClicked() { SetContext ( EC_OUTPUT ); }

void CEffectsDlg::SetContext ( EEffectsContext eContext )
{
    eCurrentContext = eContext;

    pButContextInput->blockSignals ( true );
    pButContextOutput->blockSignals ( true );
    pButContextInput->setChecked ( eCurrentContext == EC_INPUT );
    pButContextOutput->setChecked ( eCurrentContext == EC_OUTPUT );
    pButContextInput->blockSignals ( false );
    pButContextOutput->blockSignals ( false );

    const bool bIsInput = ( eCurrentContext == EC_INPUT );
    pLblStereoHint->setVisible ( !bIsInput || pClient->GetAudioChannels() == CC_STEREO );
    pRbtReverbSelL->setVisible ( bIsInput && pClient->GetAudioChannels() != CC_STEREO );
    pRbtReverbSelR->setVisible ( bIsInput && pClient->GetAudioChannels() != CC_STEREO );

    if ( pClient )
    {
        pClient->SetInputBandLevelsEnabled ( bIsInput && this->isVisible() );
        pClient->SetOutputBandLevelsEnabled ( !bIsInput && this->isVisible() );
    }

    UpdateReverbControls();
    UpdateCompressorControls();
    UpdateEQControls();
    PopulateEffectsPresetCombo();
}
