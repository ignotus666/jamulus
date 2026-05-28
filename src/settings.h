/******************************************************************************\
 * Copyright (c) 2004-2026
 *
 * Author(s):
 *  Volker Fischer
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

#include <QDomDocument>
#include <QFile>
#include <QSettings>
#include <QDir>
#ifndef HEADLESS
#    include <QApplication>
#    include <QMessageBox>
#endif
#include "global.h"
#ifndef SERVER_ONLY
#    include "client.h"
#endif
#include "plugins/audioequalizer.h"
#include "server.h"
#include "util.h"

#define NUM_EQ_BANDS 16
#define MAX_NUM_EQ_USER_PRESETS 12
#define MAX_NUM_EFFECT_PRESETS  12

/* Classes ********************************************************************/
class CSettings : public QObject
{
    Q_OBJECT

public:
    CSettings() :
        vecWindowPosMain(), // empty array
        strLanguage ( "" ),
        strFileName ( "" )
    {
        QObject::connect ( QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &CSettings::OnAboutToQuit );
#ifndef HEADLESS

        // The Jamulus App will be created as either a QCoreApplication or QApplication (a subclass of QGuiApplication).
        // State signals are only delivered to QGuiApplications, so we determine here whether we instantiated the GUI.
        const QGuiApplication* pGApp = dynamic_cast<const QGuiApplication*> ( QCoreApplication::instance() );

        if ( pGApp != nullptr )
        {
#    ifndef QT_NO_SESSIONMANAGER
            QObject::connect (
                pGApp,
                &QGuiApplication::saveStateRequest,
                this,
                [=] ( QSessionManager& ) { Save ( false ); },
                Qt::DirectConnection );

#    endif
            QObject::connect ( pGApp, &QGuiApplication::applicationStateChanged, this, [=] ( Qt::ApplicationState state ) {
                if ( Qt::ApplicationActive != state )
                {
                    Save ( false );
                }
            } );
        }
#endif
    }

    void Load ( const QList<QString>& CommandLineOptions );
    void Save ( bool isAboutToQuit );

    // common settings
    QByteArray vecWindowPosMain;
    QString    strLanguage;
    QByteArray vecPluginBrowserSplitter;
    QByteArray vecPluginBrowserScannedHeader;
    QByteArray vecPluginBrowserLoadedHeader;

protected:
    virtual void WriteSettingsToXML ( QDomDocument& IniXMLDocument, bool isAboutToQuit )                              = 0;
    virtual void ReadSettingsFromXML ( const QDomDocument& IniXMLDocument, const QList<QString>& CommandLineOptions ) = 0;

    void ReadFromFile ( const QString& strCurFileName, QDomDocument& XMLDocument );

    void WriteToFile ( const QString& strCurFileName, const QDomDocument& XMLDocument );

    void SetFileName ( const QString& sNFiName, const QString& sDefaultFileName );

    // The following functions implement the conversion from the general string
    // to base64 (which should be used for binary data in XML files). This
    // enables arbitrary utf8 characters to be used as the names in the GUI.
    //
    // ATTENTION: The "FromBase64[...]" functions must be used with caution!
    //            The reason is that if the FromBase64ToByteArray() is used to
    //            assign the stored value to a QString, this is incorrect but
    //            will not generate a compile error since there is a default
    //            conversion available for QByteArray to QString.
    QString    ToBase64 ( const QByteArray strIn ) const { return QString::fromLatin1 ( strIn.toBase64() ); }
    QString    ToBase64 ( const QString strIn ) const { return ToBase64 ( strIn.toUtf8() ); }
    QByteArray FromBase64ToByteArray ( const QString strIn ) const { return QByteArray::fromBase64 ( strIn.toLatin1() ); }
    QString    FromBase64ToString ( const QString strIn ) const { return QString::fromUtf8 ( FromBase64ToByteArray ( strIn ) ); }

    // init file access function for read/write
    void SetNumericIniSet ( QDomDocument& xmlFile, const QString& strSection, const QString& strKey, const int iValue = 0 );

    bool GetNumericIniSet ( const QDomDocument& xmlFile,
                            const QString&      strSection,
                            const QString&      strKey,
                            const int           iRangeStart,
                            const int           iRangeStop,
                            int&                iValue );

    void SetFlagIniSet ( QDomDocument& xmlFile, const QString& strSection, const QString& strKey, const bool bValue = false );

    bool GetFlagIniSet ( const QDomDocument& xmlFile, const QString& strSection, const QString& strKey, bool& bValue );

    // actual working function for init-file access
    QString GetIniSetting ( const QDomDocument& xmlFile, const QString& sSection, const QString& sKey, const QString& sDefaultVal = "" );

    void PutIniSetting ( QDomDocument& xmlFile, const QString& sSection, const QString& sKey, const QString& sValue = "" );

    QString strFileName;

public slots:
    void OnAboutToQuit() { Save ( true ); }
};

#ifndef SERVER_ONLY
class CClientSettings : public CSettings
{
public:
    CClientSettings ( CClient* pNCliP, const QString& sNFiName ) :
        CSettings(),
        vecStoredFaderTags ( MAX_NUM_STORED_FADER_SETTINGS, "" ),
        vecStoredFaderLevels ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_FADER_MAX ),
        vecStoredPanValues ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_PAN_MAX / 2 ),
        vecStoredFaderIsSolo ( MAX_NUM_STORED_FADER_SETTINGS, false ),
        vecStoredFaderIsMute ( MAX_NUM_STORED_FADER_SETTINGS, false ),
        vecStoredFaderGroupID ( MAX_NUM_STORED_FADER_SETTINGS, INVALID_INDEX ),
        vstrIPAddress ( MAX_NUM_SERVER_ADDR_ITEMS, "" ),
        iNewClientFaderLevel ( 100 ),
        iInputBoost ( 1 ),
        iInputGainL ( 100 ),
        iInputGainR ( 100 ),
        iInputGainLMidiCC ( -1 ),
        iInputGainRMidiCC ( -1 ),
        bInputGainLink ( true ),
        iSettingsTab ( SETTING_TAB_AUDIONET ),
        iEffectsTab ( 0 ),
        bConnectDlgShowAllMusicians ( true ),
        eChannelSortType ( ST_NO_SORT ),
        iNumMixerPanelRows ( 1 ),
        vstrDirectoryAddress ( MAX_NUM_SERVER_ADDR_ITEMS, "" ),
        vstrEQPresetNames ( MAX_NUM_EQ_USER_PRESETS, "" ),
        vstrEffectsPresetNames ( MAX_NUM_EFFECT_PRESETS, "" ),
        vstrEffectsPresetCarlaStateBase64 ( MAX_NUM_EFFECT_PRESETS, "" ),
        eDirectoryType ( AT_DEFAULT ),
        bEnableFeedbackDetection ( true ),
        bEnableAudioAlerts ( false ),
        bEQBypass ( true ),
        aiEQBandGainDb{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        iReverbPreDelayMs ( 0 ),
        iReverbRoomSize ( 60 ),
        iReverbDamping ( 30 ),
        iReverbWetMix ( 25 ),
        iReverbEarlyLevel ( 30 ),
        iReverbWidth ( 100 ),
        bReverbEarlyEnabled ( true ),
        bReverbFreeze ( false ),
        bReverbBypass ( true ),
        bCompressorBypass ( true ),
        fCompressorThresholdDb ( -12.0f ),
        fCompressorRatio ( 3.0f ),
        fCompressorAttackMs ( 5.0f ),
        fCompressorReleaseMs ( 120.0f ),
        fCompressorMakeupDb ( 3.0f ),
        bCompressorLimiterEnabled ( true ),
        bFilterBypass ( true ),
        bHighPassEnabled ( false ),
        bLowPassEnabled ( false ),
        iHighPassCutoffHz ( 80 ),
        iLowPassCutoffHz ( 12000 ),
        eUITheme ( UIT_SYSTEM ),
        vecWindowPosSettings(), // empty array
        vecWindowPosChat(),     // empty array
        vecWindowPosEffects(),  // empty array
        vecWindowPosConnect(),  // empty array
        bWindowWasShownSettings ( false ),
        bWindowWasShownChat ( false ),
        bWindowWasShownEffects ( false ),
        bWindowWasShownConnect ( false ),
        bOwnFaderFirst ( false ),
        iMidiChannel ( 0 ),
        iMidiMuteMyself ( 0 ),
        iMidiFaderOffset ( 0 ),
        iMidiFaderCount ( 0 ),
        iMidiPanOffset ( 0 ),
        iMidiPanCount ( 0 ),
        iMidiSoloOffset ( 0 ),
        iMidiSoloCount ( 0 ),
        iMidiMuteOffset ( 0 ),
        iMidiMuteCount ( 0 ),
        bMidiFaderEnabled ( false ),
        bMidiPanEnabled ( false ),
        bMidiSoloEnabled ( false ),
        bMidiMuteEnabled ( false ),
        bMidiMuteMyselfEnabled ( false ),
        bUseMIDIController ( false ),
        bMIDIPickupMode ( false ),
        strMidiDevice ( "" ),
        pClient ( pNCliP )
    {
        for ( int iPreset = 0; iPreset < MAX_NUM_EQ_USER_PRESETS; ++iPreset )
        {
            for ( int iBand = 0; iBand < NUM_EQ_BANDS; ++iBand )
            {
                aiEQPresetBandGainDb[iPreset][iBand] = 0;
            }
        }

        for ( int iPreset = 0; iPreset < MAX_NUM_EFFECT_PRESETS; ++iPreset )
        {
            vstrEffectsPresetCarlaStateBase64[iPreset].clear();
            bEffectsPresetEQBypass[iPreset] = true;
            for ( int iBand = 0; iBand < NUM_EQ_BANDS; ++iBand )
            {
                aiEffectsPresetEQBandGainDb[iPreset][iBand] = 0;
            }

            iEffectsPresetReverbLevel[iPreset]        = AUD_REVERB_DEFAULT;
            iEffectsPresetReverbPreDelayMs[iPreset]   = 0;
            iEffectsPresetReverbRoomSize[iPreset]     = 60;
            iEffectsPresetReverbDamping[iPreset]      = 30;
            iEffectsPresetReverbWetMix[iPreset]       = 25;
            iEffectsPresetReverbEarlyLevel[iPreset]   = 30;
            iEffectsPresetReverbWidth[iPreset]        = 100;
            bEffectsPresetReverbEarlyEnabled[iPreset] = true;
            bEffectsPresetReverbFreeze[iPreset]       = false;
            bEffectsPresetReverbBypass[iPreset]       = true;
            bEffectsPresetReverbOnLeftChan[iPreset]   = false;

            bEffectsPresetCompressorBypass[iPreset]         = true;
            iEffectsPresetCompressorThresholdDb[iPreset]    = -12;
            iEffectsPresetCompressorRatio[iPreset]          = 3;
            iEffectsPresetCompressorAttackMs[iPreset]       = 5;
            iEffectsPresetCompressorReleaseMs[iPreset]      = 120;
            iEffectsPresetCompressorMakeupDb[iPreset]       = 3;
            bEffectsPresetCompressorLimiterEnabled[iPreset] = true;

            bEffectsPresetFilterBypass[iPreset]     = true;
            bEffectsPresetHighPassEnabled[iPreset]  = false;
            bEffectsPresetLowPassEnabled[iPreset]   = false;
            iEffectsPresetHighPassCutoffHz[iPreset] = 80;
            iEffectsPresetLowPassCutoffHz[iPreset]  = 12000;
        }

        SetFileName ( sNFiName, DEFAULT_INI_FILE_NAME );
    }

    void LoadFaderSettings ( const QString& strCurFileName );
    void SaveFaderSettings ( const QString& strCurFileName );

    // Parse a --ctrlmidich MIDI mapping string and update MIDI variables
    static void ParseCtrlMidiCh ( const QString& strMidiMap,
                                  int&           iMidiChannel,
                                  int&           iMidiFaderOffset,
                                  int&           iMidiFaderCount,
                                  int&           iMidiPanOffset,
                                  int&           iMidiPanCount,
                                  int&           iMidiSoloOffset,
                                  int&           iMidiSoloCount,
                                  int&           iMidiMuteOffset,
                                  int&           iMidiMuteCount,
                                  int&           iMidiMuteMyself,
                                  bool&          bMidiFaderEnabled,
                                  bool&          bMidiPanEnabled,
                                  bool&          bMidiSoloEnabled,
                                  bool&          bMidiMuteEnabled,
                                  bool&          bMidiMuteMyselfEnabled,
                                  bool&          bUseMIDIController,
                                  bool&          bMIDIPickupMode,
                                  QString*       strMIDIDevice = nullptr );

    // general settings
    CVector<QString> vecStoredFaderTags;
    CVector<int>     vecStoredFaderLevels;
    CVector<int>     vecStoredPanValues;
    CVector<int>     vecStoredFaderIsSolo;
    CVector<int>     vecStoredFaderIsMute;
    CVector<int>     vecStoredFaderGroupID;
    CVector<QString> vstrIPAddress;
    int              iNewClientFaderLevel;
    int              iInputBoost;
    int              iInputGainL;
    int              iInputGainR;
    int              iInputGainLMidiCC;
    int              iInputGainRMidiCC;
    bool             bInputGainLink;
    int              iSettingsTab;
    int              iEffectsTab;
    bool             bConnectDlgShowAllMusicians;
    EChSortType      eChannelSortType;
    int              iNumMixerPanelRows;
    CVector<QString> vstrDirectoryAddress;
    CVector<QString> vstrEQPresetNames;
    int              aiEQPresetBandGainDb[MAX_NUM_EQ_USER_PRESETS][NUM_EQ_BANDS];
    CVector<QString> vstrEffectsPresetNames;
    CVector<QString> vstrEffectsPresetCarlaStateBase64;
    bool             bEffectsPresetEQBypass[MAX_NUM_EFFECT_PRESETS];
    int              aiEffectsPresetEQBandGainDb[MAX_NUM_EFFECT_PRESETS][NUM_EQ_BANDS];
    int              iEffectsPresetReverbLevel[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetReverbPreDelayMs[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetReverbRoomSize[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetReverbDamping[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetReverbWetMix[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetReverbEarlyLevel[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetReverbWidth[MAX_NUM_EFFECT_PRESETS];
    bool             bEffectsPresetReverbEarlyEnabled[MAX_NUM_EFFECT_PRESETS];
    bool             bEffectsPresetReverbFreeze[MAX_NUM_EFFECT_PRESETS];
    bool             bEffectsPresetReverbBypass[MAX_NUM_EFFECT_PRESETS];
    bool             bEffectsPresetReverbOnLeftChan[MAX_NUM_EFFECT_PRESETS];
    bool             bEffectsPresetCompressorBypass[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetCompressorThresholdDb[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetCompressorRatio[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetCompressorAttackMs[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetCompressorReleaseMs[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetCompressorMakeupDb[MAX_NUM_EFFECT_PRESETS];
    bool             bEffectsPresetCompressorLimiterEnabled[MAX_NUM_EFFECT_PRESETS];
    bool             bEffectsPresetFilterBypass[MAX_NUM_EFFECT_PRESETS];
    bool             bEffectsPresetHighPassEnabled[MAX_NUM_EFFECT_PRESETS];
    bool             bEffectsPresetLowPassEnabled[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetHighPassCutoffHz[MAX_NUM_EFFECT_PRESETS];
    int              iEffectsPresetLowPassCutoffHz[MAX_NUM_EFFECT_PRESETS];
    EDirectoryType   eDirectoryType;
    int              iCustomDirectoryIndex; // index of selected custom directory
    bool             bEnableFeedbackDetection;
    bool             bEnableAudioAlerts;
    bool             bEQBypass;
    int              aiEQBandGainDb[NUM_EQ_BANDS];
    int              iReverbPreDelayMs;
    int              iReverbRoomSize;
    int              iReverbDamping;
    int              iReverbWetMix;
    int              iReverbEarlyLevel;
    int              iReverbWidth;
    bool             bReverbEarlyEnabled;
    bool             bReverbFreeze;
    bool             bReverbBypass;
    bool             bCompressorBypass;
    float            fCompressorThresholdDb;
    float            fCompressorRatio;
    float            fCompressorAttackMs;
    float            fCompressorReleaseMs;
    float            fCompressorMakeupDb;
    bool             bCompressorLimiterEnabled;
    bool             bFilterBypass;
    bool             bHighPassEnabled;
    bool             bLowPassEnabled;
    int              iHighPassCutoffHz;
    int              iLowPassCutoffHz;
    EUITheme         eUITheme;

    // window position/state settings
    QByteArray vecWindowPosSettings;
    QByteArray vecWindowPosChat;
    QByteArray vecWindowPosEffects;
    QByteArray vecWindowPosConnect;
    bool       bWindowWasShownSettings;
    bool       bWindowWasShownChat;
    bool       bWindowWasShownEffects;
    bool       bWindowWasShownConnect;
    bool       bOwnFaderFirst;

    // MIDI settings
    int     iMidiChannel;
    int     iMidiMuteMyself;
    int     iMidiFaderOffset;
    int     iMidiFaderCount;
    int     iMidiPanOffset;
    int     iMidiPanCount;
    int     iMidiSoloOffset;
    int     iMidiSoloCount;
    int     iMidiMuteOffset;
    int     iMidiMuteCount;
    bool    bMidiFaderEnabled;
    bool    bMidiPanEnabled;
    bool    bMidiSoloEnabled;
    bool    bMidiMuteEnabled;
    bool    bMidiMuteMyselfEnabled;
    bool    bUseMIDIController;
    bool    bMIDIPickupMode;
    QString strMidiDevice;
    QString strCarlaPath;
    QString strCarlaPresetPath;
    QString strCarlaStateBase64;
    QStringList vstrFavoritePlugins;
    QString strCarlaPresetsDir;
    bool    bCarlaWasActive{ false };

protected:
    virtual void WriteSettingsToXML ( QDomDocument& IniXMLDocument, bool isAboutToQuit ) override;
    virtual void ReadSettingsFromXML ( const QDomDocument& IniXMLDocument, const QList<QString>& ) override;

    void ReadFaderSettingsFromXML ( const QDomDocument& IniXMLDocument );
    void WriteFaderSettingsToXML ( QDomDocument& IniXMLDocument );

    CClient* pClient;
};
#endif

class CServerSettings : public CSettings
{
public:
    CServerSettings ( CServer* pNSerP, const QString& sNFiName ) : CSettings(), eUITheme ( UIT_SYSTEM ), pServer ( pNSerP )
    {
        SetFileName ( sNFiName, DEFAULT_INI_FILE_NAME_SERVER );
    }

    EUITheme eUITheme;

protected:
    virtual void WriteSettingsToXML ( QDomDocument& IniXMLDocument, bool isAboutToQuit ) override;
    virtual void ReadSettingsFromXML ( const QDomDocument& IniXMLDocument, const QList<QString>& CommandLineOptions ) override;

    CServer* pServer;
};
