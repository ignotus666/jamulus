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

namespace
{
struct SEQBuiltInPreset
{
    const char* strName;
    int         aiGains[CAudioEqualizer::NUM_BANDS];
};

const SEQBuiltInPreset acBuiltInPresets[] = {
    { "Flat", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { "Vocal Clarity", { -2, -2, -1, -1, 1, 1, 3, 3, 4, 4, 3, 3, 1, 1, -1, -1 } },
    { "Bright", { -2, -2, -1, -1, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 4 } },
    { "Warm", { 2, 2, 2, 2, 1, 1, 0, 0, -1, -1, -1, -1, -2, -2, -2, -2 } },
    { "Low Cut", { -6, -6, -5, -5, -3, -3, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }
};

constexpr int NUM_BUILTIN_EQ_PRESETS = static_cast<int> ( sizeof ( acBuiltInPresets ) / sizeof ( acBuiltInPresets[0] ) );

bool IsBuiltInPresetName ( const QString& strName )
{
    for ( int iBuiltIn = 0; iBuiltIn < NUM_BUILTIN_EQ_PRESETS; ++iBuiltIn )
    {
        if ( strName.compare ( QString::fromUtf8 ( acBuiltInPresets[iBuiltIn].strName ), Qt::CaseInsensitive ) == 0 )
        {
            return true;
        }
    }

    return false;
}
} // namespace

CEffectsDlg::CEffectsDlg ( CClient* pNCliP, CClientSettings* pNSetP, QWidget* parent ) :
    CBaseDlg ( parent, Qt::Window ),
    pClient ( pNCliP ),
    pSettings ( pNSetP )
{
    setWindowTitle ( tr ( "Effects" ) );

    pTabs = new QTabWidget ( this );

    QWidget*     pReverbTab    = new QWidget ( pTabs );
    QVBoxLayout* pReverbLayout = new QVBoxLayout ( pReverbTab );

    QLabel* pLblReverb = new QLabel ( tr ( "Reverb Level" ), pReverbTab );
    pSldReverb         = new QSlider ( Qt::Horizontal, pReverbTab );
    pSldReverb->setRange ( 0, AUD_REVERB_MAX );
    pSldReverb->setTickInterval ( AUD_REVERB_MAX / 5 );
    pSldReverb->setTickPosition ( QSlider::TicksBothSides );

    QGridLayout* pReverbGrid = new QGridLayout();
    pReverbGrid->setColumnStretch ( 1, 1 );
    int iReverbRow = 0;

    auto addReverbRow = [pReverbGrid, pReverbTab, &iReverbRow] ( const QString& label, int minVal, int maxVal, int tick,
                                                                QSlider*& outSlider, QLabel*& outValue, const QString& suffix = QString() ) {
        QLabel* pLbl = new QLabel ( label, pReverbTab );
        outSlider    = new QSlider ( Qt::Horizontal, pReverbTab );
        outSlider->setRange ( minVal, maxVal );
        outSlider->setTickInterval ( tick );
        outSlider->setTickPosition ( QSlider::TicksBothSides );
        const QString strValue = suffix.isEmpty() ? QStringLiteral ( "0" ) : QStringLiteral ( "0" ) + suffix;
        outValue = new QLabel ( strValue, pReverbTab );
        outValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
        outValue->setMinimumWidth ( 32 );

        pReverbGrid->addWidget ( pLbl, iReverbRow, 0 );
        pReverbGrid->addWidget ( outSlider, iReverbRow, 1 );
        pReverbGrid->addWidget ( outValue, iReverbRow, 2 );
        ++iReverbRow;
    };

    addReverbRow ( tr ( "Pre-Delay" ), 0, REVERB_PRE_DELAY_MAX_MS, 10, pSldReverbPreDelay, pLblReverbPreDelayValue, tr ( " ms" ) );
    addReverbRow ( tr ( "Room Size" ), 0, REVERB_ROOM_SIZE_MAX, 10, pSldReverbRoom, pLblReverbRoomValue, tr ( " %" ) );
    addReverbRow ( tr ( "Damping" ), 0, REVERB_DAMPING_MAX, 10, pSldReverbDamping, pLblReverbDampingValue, tr ( " %" ) );
    addReverbRow ( tr ( "Wet Mix" ), 0, REVERB_WET_MIX_MAX, 10, pSldReverbWet, pLblReverbWetValue, tr ( " %" ) );
    addReverbRow ( tr ( "Early Level" ), 0, REVERB_EARLY_LEVEL_MAX, 10, pSldReverbEarly, pLblReverbEarlyValue, tr ( " %" ) );
    addReverbRow ( tr ( "Width" ), 0, REVERB_WIDTH_MAX, 10, pSldReverbWidth, pLblReverbWidthValue, tr ( " %" ) );

    pLblStereoHint = new QLabel ( tr ( "Stereo mode applies reverb to both channels." ), pReverbTab );

    pRbtReverbSelL = new QRadioButton ( tr ( "Left" ), pReverbTab );
    pRbtReverbSelR = new QRadioButton ( tr ( "Right" ), pReverbTab );
    pChbReverbBypass = new QCheckBox ( tr ( "Bypass Reverb" ), pReverbTab );
    pChbReverbEarly = new QCheckBox ( tr ( "Early Reflections" ), pReverbTab );
    pChbReverbFreeze = new QCheckBox ( tr ( "Freeze" ), pReverbTab );

    pReverbLayout->addWidget ( pChbReverbBypass );
    pReverbLayout->addWidget ( pLblReverb );
    pReverbLayout->addWidget ( pSldReverb );
    pReverbLayout->addLayout ( pReverbGrid );
    pReverbLayout->addWidget ( pChbReverbEarly );
    pReverbLayout->addWidget ( pChbReverbFreeze );
    pReverbLayout->addWidget ( pLblStereoHint );
    pReverbLayout->addWidget ( pRbtReverbSelL );
    pReverbLayout->addWidget ( pRbtReverbSelR );
    pReverbLayout->addStretch();

    pTabs->addTab ( pReverbTab, tr ( "Reverb" ) );

    QWidget*     pFilterTab    = new QWidget ( pTabs );
    QVBoxLayout* pFilterLayout = new QVBoxLayout ( pFilterTab );

    pChbFilterBypass = new QCheckBox ( tr ( "Bypass Filters" ), pFilterTab );
    pChbHighPass = new QCheckBox ( tr ( "High-Pass" ), pFilterTab );
    pChbLowPass = new QCheckBox ( tr ( "Low-Pass" ), pFilterTab );

    QGridLayout* pFilterGrid = new QGridLayout();
    pFilterGrid->setColumnStretch ( 1, 1 );
    int iFilterRow = 0;

    auto addFilterRow = [pFilterGrid, pFilterTab, &iFilterRow] ( const QString& label, int minVal, int maxVal, int tick,
                                                                QSlider*& outSlider, QLabel*& outValue, const QString& suffix = QString() ) {
        QLabel* pLbl = new QLabel ( label, pFilterTab );
        outSlider    = new QSlider ( Qt::Horizontal, pFilterTab );
        outSlider->setRange ( minVal, maxVal );
        outSlider->setTickInterval ( tick );
        outSlider->setTickPosition ( QSlider::TicksBothSides );
        const QString strValue = suffix.isEmpty() ? QStringLiteral ( "0" ) : QStringLiteral ( "0" ) + suffix;
        outValue = new QLabel ( strValue, pFilterTab );
        outValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
        outValue->setMinimumWidth ( 36 );

        pFilterGrid->addWidget ( pLbl, iFilterRow, 0 );
        pFilterGrid->addWidget ( outSlider, iFilterRow, 1 );
        pFilterGrid->addWidget ( outValue, iFilterRow, 2 );
        ++iFilterRow;
    };

    addFilterRow ( tr ( "High-Pass Cutoff" ), 20, 1000, 50, pSldHighPassCutoff, pLblHighPassValue, tr ( " Hz" ) );
    addFilterRow ( tr ( "Low-Pass Cutoff" ), 1000, 20000, 1000, pSldLowPassCutoff, pLblLowPassValue, tr ( " Hz" ) );

    pFilterLayout->addWidget ( pChbFilterBypass );
    pFilterLayout->addWidget ( pChbHighPass );
    pFilterLayout->addWidget ( pChbLowPass );
    pFilterLayout->addLayout ( pFilterGrid );
    pFilterLayout->addStretch();

    pTabs->addTab ( pFilterTab, tr ( "Filters" ) );

    QWidget*     pCompressorTab    = new QWidget ( pTabs );
    QVBoxLayout* pCompressorLayout = new QVBoxLayout ( pCompressorTab );

    pChbCompressorBypass = new QCheckBox ( tr ( "Bypass Compressor" ), pCompressorTab );
    pChbCompressorLimiter = new QCheckBox ( tr ( "Limiter" ), pCompressorTab );

    QGridLayout* pCompressorGrid = new QGridLayout();
    pCompressorGrid->setColumnStretch ( 1, 1 );
    int iCompRow = 0;

    auto addCompressorRow = [pCompressorGrid, pCompressorTab, &iCompRow] ( const QString& label, int minVal, int maxVal, int tick,
                                                                          QSlider*& outSlider, QLabel*& outValue, const QString& suffix = QString() ) {
        QLabel* pLbl = new QLabel ( label, pCompressorTab );
        outSlider    = new QSlider ( Qt::Horizontal, pCompressorTab );
        outSlider->setRange ( minVal, maxVal );
        outSlider->setTickInterval ( tick );
        outSlider->setTickPosition ( QSlider::TicksBothSides );
        const QString strValue = suffix.isEmpty() ? QStringLiteral ( "0" ) : QStringLiteral ( "0" ) + suffix;
        outValue = new QLabel ( strValue, pCompressorTab );
        outValue->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );
        outValue->setMinimumWidth ( 36 );

        pCompressorGrid->addWidget ( pLbl, iCompRow, 0 );
        pCompressorGrid->addWidget ( outSlider, iCompRow, 1 );
        pCompressorGrid->addWidget ( outValue, iCompRow, 2 );
        ++iCompRow;
    };

    addCompressorRow ( tr ( "Threshold" ), -60, 0, 6, pSldCompressorThreshold, pLblCompressorThresholdValue, tr ( " dB" ) );
    addCompressorRow ( tr ( "Ratio" ), 1, 20, 1, pSldCompressorRatio, pLblCompressorRatioValue, tr ( ":1" ) );
    addCompressorRow ( tr ( "Attack" ), 1, 50, 5, pSldCompressorAttack, pLblCompressorAttackValue, tr ( " ms" ) );
    addCompressorRow ( tr ( "Release" ), 10, 400, 20, pSldCompressorRelease, pLblCompressorReleaseValue, tr ( " ms" ) );
    addCompressorRow ( tr ( "Makeup" ), 0, 24, 3, pSldCompressorMakeup, pLblCompressorMakeupValue, tr ( " dB" ) );

    pCompressorLayout->addWidget ( pChbCompressorBypass );
    pCompressorLayout->addLayout ( pCompressorGrid );
    pCompressorLayout->addWidget ( pChbCompressorLimiter );
    pCompressorLayout->addStretch();

    pTabs->addTab ( pCompressorTab, tr ( "Compressor" ) );

    QWidget*     pEQTab    = new QWidget ( pTabs );
    QVBoxLayout* pEQLayout = new QVBoxLayout ( pEQTab );

    pChbEQBypass = new QCheckBox ( tr ( "Bypass Equalizer" ), pEQTab );
    pEQLayout->addWidget ( pChbEQBypass );

    QHBoxLayout* pPresetRow = new QHBoxLayout();
    pCbxEQPresets           = new QComboBox ( pEQTab );
    pButEQSavePreset        = new QPushButton ( tr ( "Save" ), pEQTab );
    pButEQSaveAsPreset      = new QPushButton ( tr ( "Save As..." ), pEQTab );
    pButEQDeletePreset      = new QPushButton ( tr ( "Delete" ), pEQTab );

    pPresetRow->addWidget ( pCbxEQPresets, 1 );
    pPresetRow->addWidget ( pButEQSavePreset );
    pPresetRow->addWidget ( pButEQSaveAsPreset );
    pPresetRow->addWidget ( pButEQDeletePreset );
    pEQLayout->addLayout ( pPresetRow );

    QVBoxLayout* pOutputMeterLayout = new QVBoxLayout();
    pOutputMeterLayout->setContentsMargins ( 0, 0, 0, 0 );
    pOutputMeterLayout->setSpacing ( 4 );
    pLblOutputBandTitle = new QLabel ( tr ( "Output" ), pEQTab );
    pLblOutputBandTitle->setAlignment ( Qt::AlignHCenter );
    pLblOutputBandTitle->setObjectName ( "pOutputBandMeterTitle" );
    pOutputBandMeter = new COutputBandMeter ( pEQTab );
    pOutputBandMeter->setMinimumHeight ( 72 );
    pOutputBandMeter->setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );
    pOutputMeterLayout->addWidget ( pLblOutputBandTitle );
    pOutputMeterLayout->addWidget ( pOutputBandMeter );
    const int iOutputMeterIndex = pEQLayout->count();
    pEQLayout->addLayout ( pOutputMeterLayout );

    QHBoxLayout* pBandsLayout = new QHBoxLayout();
    pBandsLayout->setSpacing ( 4 );
    const char*  acBandLabels[CAudioEqualizer::NUM_BANDS] = { "63", "89", "125", "177", "250", "354", "500", "707",
                                                             "1k", "1.4k", "2k", "2.8k", "4k", "5.6k", "8k", "11.2k" };

    for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
    {
        QVBoxLayout* pBandLayout = new QVBoxLayout();
        pBandLayout->setContentsMargins ( 0, 0, 0, 0 );
        pBandLayout->setSpacing ( 2 );

        QLabel* pLblFreq = new QLabel ( QString::fromUtf8 ( acBandLabels[iBand] ), pEQTab );
        pLblFreq->setAlignment ( Qt::AlignHCenter );

        pSldEQBands[iBand] = new QSlider ( Qt::Vertical, pEQTab );
        pSldEQBands[iBand]->setRange ( -12, 12 );
        pSldEQBands[iBand]->setValue ( 0 );
        pSldEQBands[iBand]->setTickInterval ( 3 );
        pSldEQBands[iBand]->setTickPosition ( QSlider::TicksBothSides );
        pSldEQBands[iBand]->setMinimumHeight ( 120 );
        pSldEQBands[iBand]->setMinimumWidth ( 18 );
        pSldEQBands[iBand]->setMaximumWidth ( 24 );
        pSldEQBands[iBand]->setSizePolicy ( QSizePolicy::Fixed, QSizePolicy::Expanding );

        pLblEQBandValues[iBand] = new QLabel ( QStringLiteral ( "0" ), pEQTab );
        pLblEQBandValues[iBand]->setAlignment ( Qt::AlignHCenter );

        pBandLayout->addWidget ( pLblFreq );
        pBandLayout->addWidget ( pSldEQBands[iBand], 1 );
        pBandLayout->addWidget ( pLblEQBandValues[iBand] );
        pBandsLayout->addLayout ( pBandLayout );

        QObject::connect ( pSldEQBands[iBand], &QSlider::valueChanged, this, [this, iBand] ( int value )
        {
            pLblEQBandValues[iBand]->setText ( QString::number ( value ) );
            emit EQBandGainChanged ( iBand, value );
        } );
    }

    const int iBandsIndex = pEQLayout->count();
    pEQLayout->addLayout ( pBandsLayout );
    pEQLayout->setStretch ( iOutputMeterIndex, 1 );
    pEQLayout->setStretch ( iBandsIndex, 3 );

    pButEQReset = new QPushButton ( tr ( "Reset EQ" ), pEQTab );
    pEQLayout->addWidget ( pButEQReset, 0, Qt::AlignRight );
    pEQLayout->addStretch();

    pTabs->addTab ( pEQTab, tr ( "Equalizer" ) );

    setMinimumHeight ( 380 );

    QVBoxLayout* pMainLayout = new QVBoxLayout ( this );
    pMainLayout->addWidget ( pTabs );

    QObject::connect ( pSldReverb, &QSlider::valueChanged, this, &CEffectsDlg::ReverbValueChanged );
    QObject::connect ( pChbReverbBypass, &QCheckBox::toggled, this, &CEffectsDlg::ReverbBypassChanged );
    QObject::connect ( pSldReverbPreDelay, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbPreDelayValue->setText ( QString::number ( value ) + tr ( " ms" ) );
        emit ReverbPreDelayChanged ( value );
    } );
    QObject::connect ( pSldReverbRoom, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbRoomValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbRoomSizeChanged ( value );
    } );
    QObject::connect ( pSldReverbDamping, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbDampingValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbDampingChanged ( value );
    } );
    QObject::connect ( pSldReverbWet, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbWetValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbWetMixChanged ( value );
    } );
    QObject::connect ( pSldReverbEarly, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblReverbEarlyValue->setText ( QString::number ( value ) + tr ( " %" ) );
        emit ReverbEarlyLevelChanged ( value );
    } );
    QObject::connect ( pSldReverbWidth, &QSlider::valueChanged, this, [this] ( int value ) {
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
    QObject::connect ( pChbFilterBypass, &QCheckBox::toggled, this, [this] ( bool bypassed ) {
        emit FilterBypassChanged ( bypassed );
        UpdateFilterControls();
    } );
    QObject::connect ( pChbHighPass, &QCheckBox::toggled, this, [this] ( bool enabled ) {
        pSldHighPassCutoff->setEnabled ( enabled && !pChbFilterBypass->isChecked() );
        emit HighPassEnabledChanged ( enabled );
    } );
    QObject::connect ( pChbLowPass, &QCheckBox::toggled, this, [this] ( bool enabled ) {
        pSldLowPassCutoff->setEnabled ( enabled && !pChbFilterBypass->isChecked() );
        emit LowPassEnabledChanged ( enabled );
    } );
    QObject::connect ( pSldHighPassCutoff, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblHighPassValue->setText ( QString::number ( value ) + tr ( " Hz" ) );
        emit HighPassCutoffChanged ( value );
    } );
    QObject::connect ( pSldLowPassCutoff, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblLowPassValue->setText ( QString::number ( value ) + tr ( " Hz" ) );
        emit LowPassCutoffChanged ( value );
    } );
    QObject::connect ( pChbCompressorBypass, &QCheckBox::toggled, this, &CEffectsDlg::CompressorBypassChanged );
    QObject::connect ( pChbCompressorLimiter, &QCheckBox::toggled, this, &CEffectsDlg::CompressorLimiterChanged );
    QObject::connect ( pSldCompressorThreshold, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblCompressorThresholdValue->setText ( QString::number ( value ) + tr ( " dB" ) );
        emit CompressorThresholdChanged ( value );
    } );
    QObject::connect ( pSldCompressorRatio, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblCompressorRatioValue->setText ( QString::number ( value ) + tr ( ":1" ) );
        emit CompressorRatioChanged ( value );
    } );
    QObject::connect ( pSldCompressorAttack, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblCompressorAttackValue->setText ( QString::number ( value ) + tr ( " ms" ) );
        emit CompressorAttackChanged ( value );
    } );
    QObject::connect ( pSldCompressorRelease, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblCompressorReleaseValue->setText ( QString::number ( value ) + tr ( " ms" ) );
        emit CompressorReleaseChanged ( value );
    } );
    QObject::connect ( pSldCompressorMakeup, &QSlider::valueChanged, this, [this] ( int value ) {
        pLblCompressorMakeupValue->setText ( QString::number ( value ) + tr ( " dB" ) );
        emit CompressorMakeupChanged ( value );
    } );
    QObject::connect ( pChbEQBypass, &QCheckBox::toggled, this, &CEffectsDlg::EQBypassChanged );
    QObject::connect ( pCbxEQPresets,
                       QOverload<int>::of ( &QComboBox::currentIndexChanged ),
                       this,
                       &CEffectsDlg::ApplyPresetFromComboIndex );
    QObject::connect ( pButEQReset, &QPushButton::clicked, this, &CEffectsDlg::OnResetEQClicked );
    QObject::connect ( pButEQSavePreset, &QPushButton::clicked, this, &CEffectsDlg::OnSaveEQPresetClicked );
    QObject::connect ( pButEQSaveAsPreset, &QPushButton::clicked, this, &CEffectsDlg::OnSaveAsEQPresetClicked );
    QObject::connect ( pButEQDeletePreset, &QPushButton::clicked, this, &CEffectsDlg::OnDeleteEQPresetClicked );

    UpdateReverbControls();
    UpdateFilterControls();
    UpdateCompressorControls();
    UpdateEQReadouts();
    PopulateEQPresetCombo();
    UpdateEQPresetSelection();
}

void CEffectsDlg::UpdateOutputBandLevels ( const CVector<float>& vecOutLevels )
{
    if ( pOutputBandMeter != nullptr )
    {
        pOutputBandMeter->SetLevels ( vecOutLevels );
    }
}

void CEffectsDlg::showEvent ( QShowEvent* Event )
{
    UpdateReverbControls();
    UpdateFilterControls();
    UpdateCompressorControls();
    UpdateEQControls();
    UpdateEQPresetSelection();
    CBaseDlg::showEvent ( Event );
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
    pChbHighPass->setEnabled ( !bBypass );
    pChbLowPass->setEnabled ( !bBypass );
    pSldHighPassCutoff->setEnabled ( !bBypass && pClient->GetHighPassEnabled() );
    pSldLowPassCutoff->setEnabled ( !bBypass && pClient->GetLowPassEnabled() );

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
    pSldCompressorThreshold->setEnabled ( !bBypass );
    pSldCompressorRatio->setEnabled ( !bBypass );
    pSldCompressorAttack->setEnabled ( !bBypass );
    pSldCompressorRelease->setEnabled ( !bBypass );
    pSldCompressorMakeup->setEnabled ( !bBypass );
    pChbCompressorLimiter->setEnabled ( !bBypass );

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
    pSldReverb->setEnabled ( !bBypass );
    pSldReverbPreDelay->setEnabled ( !bBypass );
    pSldReverbRoom->setEnabled ( !bBypass );
    pSldReverbDamping->setEnabled ( !bBypass );
    pSldReverbWet->setEnabled ( !bBypass );
    pSldReverbEarly->setEnabled ( !bBypass && pClient->GetReverbEarlyEnabled() );
    pSldReverbWidth->setEnabled ( !bBypass );
    pChbReverbEarly->setEnabled ( !bBypass );
    pChbReverbFreeze->setEnabled ( !bBypass );
    pRbtReverbSelL->setEnabled ( !bBypass );
    pRbtReverbSelR->setEnabled ( !bBypass );

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

void CEffectsDlg::PopulateEQPresetCombo()
{
    pCbxEQPresets->clear();

    for ( int iBuiltIn = 0; iBuiltIn < NUM_BUILTIN_EQ_PRESETS; ++iBuiltIn )
    {
        const auto& cPreset = acBuiltInPresets[iBuiltIn];
        const int   iOverrideSlot = FindPresetSlotByName ( QString::fromUtf8 ( cPreset.strName ) );

        QString strGains;
        for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
        {
            if ( !strGains.isEmpty() )
            {
                strGains += ",";
            }

            if ( iOverrideSlot != INVALID_INDEX )
            {
                strGains += QString::number ( pSettings->aiEQPresetBandGainDb[iOverrideSlot][iBand] );
            }
            else
            {
                strGains += QString::number ( cPreset.aiGains[iBand] );
            }
        }

        pCbxEQPresets->addItem ( tr ( cPreset.strName ) );
        pCbxEQPresets->setItemData ( pCbxEQPresets->count() - 1, strGains, Qt::UserRole );
        pCbxEQPresets->setItemData ( pCbxEQPresets->count() - 1, iOverrideSlot, Qt::UserRole + 1 );
    }

    for ( int iPreset = 0; iPreset < MAX_NUM_EQ_USER_PRESETS; ++iPreset )
    {
        const QString strName = pSettings->vstrEQPresetNames[iPreset];
        if ( strName.isEmpty() || IsBuiltInPresetName ( strName ) )
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
}

void CEffectsDlg::OnSaveEQPresetClicked()
{
    const int iComboIndex = pCbxEQPresets->currentIndex();
    if ( iComboIndex < 0 )
    {
        return;
    }

    int     iPresetSlot = pCbxEQPresets->itemData ( iComboIndex, Qt::UserRole + 1 ).toInt();
    QString strName     = pCbxEQPresets->itemText ( iComboIndex );

    // Built-in presets are stored as user overrides with the same name.
    if ( iComboIndex < NUM_BUILTIN_EQ_PRESETS )
    {
        iPresetSlot = FindPresetSlotByName ( strName );
        if ( iPresetSlot == INVALID_INDEX )
        {
            iPresetSlot = FindFreePresetSlot();
        }
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
