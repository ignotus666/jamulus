/******************************************************************************\
 * Copyright (c) 2004-2026
 *
 * Author(s):
 *  Volker Fischer
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
\******************************************************************************/

#include "settings.h"
#include <cmath>

/* Implementation *************************************************************/
void CSettings::Load ( const QList<QString>& CommandLineOptions )
{
    // prepare file name for loading initialization data from XML file and read
    // data from file if possible
    QDomDocument IniXMLDocument;
    ReadFromFile ( strFileName, IniXMLDocument );

    // read the settings from the given XML file
    ReadSettingsFromXML ( IniXMLDocument, CommandLineOptions );
}

void CSettings::Save ( bool isAboutToQuit )
{
    // create XML document for storing initialization parameters
    QDomDocument IniXMLDocument;

    // write the settings in the XML file
    WriteSettingsToXML ( IniXMLDocument, isAboutToQuit );

    // prepare file name for storing initialization data in XML file and store
    // XML data in file
    WriteToFile ( strFileName, IniXMLDocument );
}

void CSettings::ReadFromFile ( const QString& strCurFileName, QDomDocument& XMLDocument )
{
    QFile file ( strCurFileName );

    if ( file.open ( QIODevice::ReadOnly ) )
    {
        XMLDocument.setContent ( QTextStream ( &file ).readAll(), false );
        file.close();
    }
}

void CSettings::WriteToFile ( const QString& strCurFileName, const QDomDocument& XMLDocument )
{
    QFile file ( strCurFileName );

    if ( file.open ( QIODevice::WriteOnly ) )
    {
        QTextStream ( &file ) << XMLDocument.toString();
        file.close();
    }
}

void CSettings::SetFileName ( const QString& sNFiName, const QString& sDefaultFileName )
{
    // return the file name with complete path, take care if given file name is empty
    strFileName = sNFiName;

    if ( strFileName.isEmpty() )
    {
        // we use the Qt default setting file paths for the different OSs by
        // utilizing the QSettings class
        const QString sConfigDir =
            QFileInfo ( QSettings ( QSettings::IniFormat, QSettings::UserScope, APP_NAME, APP_NAME ).fileName() ).absolutePath();

        // make sure the directory exists
        if ( !QFile::exists ( sConfigDir ) )
        {
            QDir().mkpath ( sConfigDir );
        }

        // append the actual file name
        strFileName = sConfigDir + "/" + sDefaultFileName;
    }
}

void CSettings::SetNumericIniSet ( QDomDocument& xmlFile, const QString& strSection, const QString& strKey, const int iValue )
{
    // convert input parameter which is an integer to string and store
    PutIniSetting ( xmlFile, strSection, strKey, QString::number ( iValue ) );
}

bool CSettings::GetNumericIniSet ( const QDomDocument& xmlFile,
                                   const QString&      strSection,
                                   const QString&      strKey,
                                   const int           iRangeStart,
                                   const int           iRangeStop,
                                   int&                iValue )
{
    // init return value
    bool bReturn = false;

    const QString strGetIni = GetIniSetting ( xmlFile, strSection, strKey );

    // check if it is a valid parameter
    if ( !strGetIni.isEmpty() )
    {
        // convert string from init file to integer
        iValue = strGetIni.toInt();

        // check range
        if ( ( iValue >= iRangeStart ) && ( iValue <= iRangeStop ) )
        {
            bReturn = true;
        }
    }

    return bReturn;
}

void CSettings::SetFlagIniSet ( QDomDocument& xmlFile, const QString& strSection, const QString& strKey, const bool bValue )
{
    // we encode true -> "1" and false -> "0"
    PutIniSetting ( xmlFile, strSection, strKey, bValue ? "1" : "0" );
}

bool CSettings::GetFlagIniSet ( const QDomDocument& xmlFile, const QString& strSection, const QString& strKey, bool& bValue )
{
    // init return value
    bool bReturn = false;

    const QString strGetIni = GetIniSetting ( xmlFile, strSection, strKey );

    if ( !strGetIni.isEmpty() )
    {
        bValue  = ( strGetIni.toInt() != 0 );
        bReturn = true;
    }

    return bReturn;
}

// Init-file routines using XML ***********************************************
QString CSettings::GetIniSetting ( const QDomDocument& xmlFile, const QString& sSection, const QString& sKey, const QString& sDefaultVal )
{
    // init return parameter with default value
    QString sResult ( sDefaultVal );

    // get section
    QDomElement xmlSection = xmlFile.firstChildElement ( sSection );

    if ( !xmlSection.isNull() )
    {
        // get key
        QDomElement xmlKey = xmlSection.firstChildElement ( sKey );

        if ( !xmlKey.isNull() )
        {
            // get value
            sResult = xmlKey.text();
        }
    }

    return sResult;
}

void CSettings::PutIniSetting ( QDomDocument& xmlFile, const QString& sSection, const QString& sKey, const QString& sValue )
{
    // check if section is already there, if not then create it
    QDomElement xmlSection = xmlFile.firstChildElement ( sSection );

    if ( xmlSection.isNull() )
    {
        // create new root element and add to document
        xmlSection = xmlFile.createElement ( sSection );
        xmlFile.appendChild ( xmlSection );
    }

    // check if key is already there, if not then create it
    QDomElement xmlKey = xmlSection.firstChildElement ( sKey );

    if ( xmlKey.isNull() )
    {
        xmlKey = xmlFile.createElement ( sKey );
        xmlSection.appendChild ( xmlKey );
    }

    // add actual data to the key
    QDomText currentValue = xmlFile.createTextNode ( sValue );
    xmlKey.appendChild ( currentValue );
}

#ifndef SERVER_ONLY

// Parse MIDI commmand line parameters and update MIDI variables
void CClientSettings::ParseCtrlMidiCh ( const QString& strMidiMap,
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
                                        QString*       strMIDIDevice )
{
    if ( strMidiMap.isEmpty() )
    {
        // Empty string explicitly disables MIDI, but preserves section settings
        bUseMIDIController = false;
        return;
    }

    QStringList parts = strMidiMap.split ( ';' );
    if ( parts.isEmpty() )
    {
        bUseMIDIController = false;
        return;
    }

    // Parse MIDI channel (first parameter) - must be a valid number
    bool bIsNumber = false;
    iMidiChannel   = parts[0].trimmed().toInt ( &bIsNumber );

    // Validate MIDI channel (0 = all channels, 1-16 = specific channel)
    if ( !bIsNumber || iMidiChannel < 0 || iMidiChannel > 16 )
    {
        // Invalid channel disables MIDI, but preserves section settings
        bUseMIDIController = false;
        return;
    }

    // Check for legacy format: [channel];[offset]
    // If second parameter is a plain number (no prefix), treat as legacy format
    if ( parts.size() >= 2 )
    {
        bool    bIsNumber = false;
        QString sParm     = parts[1].trimmed();
        int     iOffset   = sParm.toInt ( &bIsNumber );

        if ( bIsNumber && !sParm.isEmpty() )
        {
            // Legacy format: set up faders from offset to 127 or MAX_NUM_CHANNELS
            iMidiFaderOffset   = iOffset;
            iMidiFaderCount    = qMin ( MAX_NUM_CHANNELS, 128 - iOffset );
            bUseMIDIController = true;
            return;
        }
    }

    // Parse named controllers (new format)
    for ( int i = 1; i < parts.size(); ++i )
    {
        QString sParm = parts[i].trimmed();
        if ( sParm.isEmpty() )
        {
            continue;
        }

        QChar cType = sParm[0];

        // Handle device selection
        if ( cType == 'd' )
        {
            if ( strMIDIDevice != nullptr )
            {
                *strMIDIDevice = sParm.mid ( 1 );
            }
            continue;
        }

        // Handle MIDI pickup mode (u)
        if ( sParm == "u" )
        {
            bMIDIPickupMode = true;
            continue;
        }

        // Parse controller specification: [type][offset]*[count]
        // where [type] is f, p, s, m, or o
        QStringList vals   = sParm.mid ( 1 ).split ( '*' );
        int         iFirst = vals[0].toInt();
        int         iNum   = ( vals.size() > 1 ) ? vals[1].toInt() : 1;

        // Bounds checking
        if ( iFirst < 0 || iFirst >= 128 )
        {
            continue;
        }

        iNum = qMin ( iNum, MAX_NUM_CHANNELS );
        iNum = qMin ( iNum, 128 - iFirst );

        if ( iNum <= 0 )
        {
            continue;
        }

        // Assign to appropriate controller type
        if ( cType == 'f' )
        {
            iMidiFaderOffset  = iFirst;
            iMidiFaderCount   = iNum;
            bMidiFaderEnabled = true;
        }
        else if ( cType == 'p' )
        {
            iMidiPanOffset  = iFirst;
            iMidiPanCount   = iNum;
            bMidiPanEnabled = true;
        }
        else if ( cType == 's' )
        {
            iMidiSoloOffset  = iFirst;
            iMidiSoloCount   = iNum;
            bMidiSoloEnabled = true;
        }
        else if ( cType == 'm' )
        {
            iMidiMuteOffset  = iFirst;
            iMidiMuteCount   = iNum;
            bMidiMuteEnabled = true;
        }
        else if ( cType == 'o' )
        {
            iMidiMuteMyself        = iFirst;
            bMidiMuteMyselfEnabled = true;
        }
    }

    bUseMIDIController = true;
}

// Client settings -------------------------------------------------------------
void CClientSettings::LoadFaderSettings ( const QString& strCurFileName )
{
    // prepare file name for loading initialization data from XML file and read
    // data from file if possible
    QDomDocument IniXMLDocument;
    ReadFromFile ( strCurFileName, IniXMLDocument );

    // read the settings from the given XML file
    ReadFaderSettingsFromXML ( IniXMLDocument );
}

void CClientSettings::SaveFaderSettings ( const QString& strCurFileName )
{
    // create XML document for storing initialization parameters
    QDomDocument IniXMLDocument;

    // write the settings in the XML file
    WriteFaderSettingsToXML ( IniXMLDocument );

    // prepare file name for storing initialization data in XML file and store
    // XML data in file
    WriteToFile ( strCurFileName, IniXMLDocument );
}

void CClientSettings::ReadSettingsFromXML ( const QDomDocument& IniXMLDocument, const QList<QString>& CommandLineOptions )
{
    int  iIdx;
    int  iValue;
    bool bValue;

    // IP addresses
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        vstrIPAddress[iIdx] = GetIniSetting ( IniXMLDocument, "client", QString ( "ipaddress%1" ).arg ( iIdx ), "" );
    }

    // new client level
    if ( GetNumericIniSet ( IniXMLDocument, "client", "newclientlevel", 0, 100, iValue ) )
    {
        iNewClientFaderLevel = iValue;
    }

    // input boost
    if ( GetNumericIniSet ( IniXMLDocument, "client", "inputboost", 1, 10, iValue ) )
    {
        iInputBoost = iValue;
    }

    if ( GetFlagIniSet ( IniXMLDocument, "client", "enablefeedbackdetection", bValue ) )
    {
        bEnableFeedbackDetection = bValue;
    }

    // connect dialog show all musicians
    if ( GetFlagIniSet ( IniXMLDocument, "client", "connectdlgshowallmusicians", bValue ) )
    {
        bConnectDlgShowAllMusicians = bValue;
    }

    // language
    strLanguage =
        GetIniSetting ( IniXMLDocument, "client", "language", CLocale::FindSysLangTransFileName ( CLocale::GetAvailableTranslations() ).first );

    // fader channel sorting
    if ( GetNumericIniSet ( IniXMLDocument, "client", "channelsort", 0, 5 /* ST_BY_SERVER_CHANNEL */, iValue ) )
    {
        eChannelSortType = static_cast<EChSortType> ( iValue );
    }

    // own fader first sorting
    if ( GetFlagIniSet ( IniXMLDocument, "client", "ownfaderfirst", bValue ) )
    {
        bOwnFaderFirst = bValue;
    }

    // number of mixer panel rows
    if ( GetNumericIniSet ( IniXMLDocument, "client", "numrowsmixpan", 1, 8, iValue ) )
    {
        iNumMixerPanelRows = iValue;
    }

    // audio alerts
    if ( GetFlagIniSet ( IniXMLDocument, "client", "enableaudioalerts", bValue ) )
    {
        bEnableAudioAlerts = bValue;
    }

    // name
    pClient->ChannelInfo.strName = FromBase64ToString (
        GetIniSetting ( IniXMLDocument, "client", "name_base64", ToBase64 ( QCoreApplication::translate ( "CMusProfDlg", "No Name" ) ) ) );

    // instrument
    if ( GetNumericIniSet ( IniXMLDocument, "client", "instrument", 0, CInstPictures::GetNumAvailableInst() - 1, iValue ) )
    {
        pClient->ChannelInfo.iInstrument = iValue;
    }

    // country
    if ( GetNumericIniSet ( IniXMLDocument, "client", "country", 0, static_cast<int> ( QLocale::LastCountry ), iValue ) )
    {
        pClient->ChannelInfo.eCountry = CLocale::WireFormatCountryCodeToQtCountry ( iValue );
    }
    else
    {
        // if no country is given, use the one from the operating system
        pClient->ChannelInfo.eCountry = QLocale::system().country();
    }

    // city
    pClient->ChannelInfo.strCity = FromBase64ToString ( GetIniSetting ( IniXMLDocument, "client", "city_base64" ) );

    // skill level
    if ( GetNumericIniSet ( IniXMLDocument, "client", "skill", 0, 3 /* SL_PROFESSIONAL */, iValue ) )
    {
        pClient->ChannelInfo.eSkillLevel = static_cast<ESkillLevel> ( iValue );
    }

    // audio fader
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audfad", AUD_FADER_IN_MIN, AUD_FADER_IN_MAX, iValue ) )
    {
        pClient->SetAudioInFader ( iValue );
    }

    // reverberation level
    if ( GetNumericIniSet ( IniXMLDocument, "client", "revlev", 0, AUD_REVERB_MAX, iValue ) )
    {
        pClient->SetReverbLevel ( iValue );
    }

    // reverberation channel assignment
    if ( GetFlagIniSet ( IniXMLDocument, "client", "reverblchan", bValue ) )
    {
        pClient->SetReverbOnLeftChan ( bValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "revpredelay", 0, REVERB_PRE_DELAY_MAX_MS, iValue ) )
    {
        pClient->SetReverbPreDelayMs ( iValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "revroom", 0, REVERB_ROOM_SIZE_MAX, iValue ) )
    {
        pClient->SetReverbRoomSize ( iValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "revdamping", 0, REVERB_DAMPING_MAX, iValue ) )
    {
        pClient->SetReverbDamping ( iValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "revwet", 0, REVERB_WET_MIX_MAX, iValue ) )
    {
        pClient->SetReverbWetMix ( iValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "revearlylevel", 0, REVERB_EARLY_LEVEL_MAX, iValue ) )
    {
        pClient->SetReverbEarlyLevel ( iValue );
    }

    if ( GetFlagIniSet ( IniXMLDocument, "client", "revearlyenable", bValue ) )
    {
        pClient->SetReverbEarlyEnabled ( bValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "revwidth", 0, REVERB_WIDTH_MAX, iValue ) )
    {
        pClient->SetReverbWidth ( iValue );
    }

    if ( GetFlagIniSet ( IniXMLDocument, "client", "revfreeze", bValue ) )
    {
        pClient->SetReverbFreeze ( bValue );
    }

    if ( GetFlagIniSet ( IniXMLDocument, "client", "revbypass", bValue ) )
    {
        pClient->SetReverbBypass ( bValue );
    }

    if ( GetFlagIniSet ( IniXMLDocument, "client", "compbypass", bValue ) )
    {
        pClient->SetCompressorBypass ( bValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "compthreshold", -60, 0, iValue ) )
    {
        pClient->SetCompressorThresholdDb ( static_cast<float> ( iValue ) );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "compratio", 1, 20, iValue ) )
    {
        pClient->SetCompressorRatio ( static_cast<float> ( iValue ) / 1.0f );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "compattack", 1, 200, iValue ) )
    {
        pClient->SetCompressorAttackMs ( static_cast<float> ( iValue ) );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "comprelease", 10, 500, iValue ) )
    {
        pClient->SetCompressorReleaseMs ( static_cast<float> ( iValue ) );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "compmakeup", 0, 24, iValue ) )
    {
        pClient->SetCompressorMakeupDb ( static_cast<float> ( iValue ) );
    }

    if ( GetFlagIniSet ( IniXMLDocument, "client", "complimiter", bValue ) )
    {
        pClient->SetCompressorLimiterEnabled ( bValue );
    }

    // sound card selection
    const QString strError = pClient->SetSndCrdDev ( FromBase64ToString ( GetIniSetting ( IniXMLDocument, "client", "auddev_base64", "" ) ) );

    if ( !strError.isEmpty() )
    {
#    ifndef HEADLESS
        // special case: when settings are loaded no GUI is yet created, therefore
        // we have to create a warning message box here directly
        QMessageBox::warning ( nullptr, APP_NAME, strError );
#    endif
    }

    // sound card channel mapping settings: make sure these settings are
    // set AFTER the sound card device is set, otherwise the settings are
    // overwritten by the defaults
    //
    // sound card left input channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdinlch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdLeftInputChannel ( iValue );
    }

    // sound card right input channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdinrch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdRightInputChannel ( iValue );
    }

    // sound card left output channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutlch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdLeftOutputChannel ( iValue );
    }

    // sound card right output channel mapping
    if ( GetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutrch", 0, MAX_NUM_IN_OUT_CHANNELS - 1, iValue ) )
    {
        pClient->SetSndCrdRightOutputChannel ( iValue );
    }

    // sound card preferred buffer size index
    if ( GetNumericIniSet ( IniXMLDocument, "client", "prefsndcrdbufidx", FRAME_SIZE_FACTOR_PREFERRED, FRAME_SIZE_FACTOR_SAFE, iValue ) )
    {
        // additional check required since only a subset of factors are
        // defined
        if ( ( iValue == FRAME_SIZE_FACTOR_PREFERRED ) || ( iValue == FRAME_SIZE_FACTOR_DEFAULT ) || ( iValue == FRAME_SIZE_FACTOR_SAFE ) )
        {
            pClient->SetSndCrdPrefFrameSizeFactor ( iValue );
        }
    }

    // automatic network jitter buffer size setting
    if ( GetFlagIniSet ( IniXMLDocument, "client", "autojitbuf", bValue ) )
    {
        pClient->SetDoAutoSockBufSize ( bValue );
    }

    // network jitter buffer size
    if ( GetNumericIniSet ( IniXMLDocument, "client", "jitbuf", MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL, iValue ) )
    {
        pClient->SetSockBufNumFrames ( iValue );
    }

    // network jitter buffer size for server
    if ( GetNumericIniSet ( IniXMLDocument, "client", "jitbufserver", MIN_NET_BUF_SIZE_NUM_BL, MAX_NET_BUF_SIZE_NUM_BL, iValue ) )
    {
        pClient->SetServerSockBufNumFrames ( iValue );
    }

    // enable OPUS64 setting
    if ( GetFlagIniSet ( IniXMLDocument, "client", "enableopussmall", bValue ) )
    {
        pClient->SetEnableOPUS64 ( bValue );
    }

    ReadEQSettingsFromXML ( IniXMLDocument, pClient, false );
    ReadEQSettingsFromXML ( IniXMLDocument, pClient, true );
    ReadCompressorSettingsFromXML ( IniXMLDocument, pClient, false );
    ReadCompressorSettingsFromXML ( IniXMLDocument, pClient, true );
    ReadReverbSettingsFromXML ( IniXMLDocument, pClient, false );
    ReadReverbSettingsFromXML ( IniXMLDocument, pClient, true );

    // UI theme
    if ( GetNumericIniSet ( IniXMLDocument, "client", "uitheme", 0, 2 /* UIT_SYSTEM */, iValue ) )
    {
        eUITheme = ( iValue == 1 ) ? UIT_DARK : UIT_LIGHT;
    }

    // GUI design
    if ( GetNumericIniSet ( IniXMLDocument, "client", "guidesign", 0, 2 /* GD_SLIMFADER */, iValue ) )
    {
        pClient->SetGUIDesign ( ( iValue == GD_SLIMFADER ) ? GD_SLIMFADER : GD_STANDARD );
    }

    // MeterStyle
    if ( GetNumericIniSet ( IniXMLDocument, "client", "meterstyle", 0, 4 /* legacy max */, iValue ) )
    {
        // Preserve backward compatibility with legacy config values:
        // 0=narrow bar, 1=wide bar, 2=LED stripe (wide), 3=LED round small (narrow), 4=LED round big (wide).
        const EMeterStyle eNormalizedMeterStyle = ( iValue == 0 || iValue == 3 ) ? MT_BAR_NARROW : MT_BAR_WIDE;
        pClient->SetMeterStyle ( eNormalizedMeterStyle );
    }
    else
    {
        // if MeterStyle is not found in the ini, set it based on the GUI design
        if ( GetNumericIniSet ( IniXMLDocument, "client", "guidesign", 0, 2 /* GD_SLIMFADER */, iValue ) )
        {
            pClient->SetMeterStyle ( iValue == GD_SLIMFADER ? MT_BAR_NARROW : MT_BAR_WIDE );
        }
    }

    // audio channels
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audiochannels", 0, 2 /* CC_STEREO */, iValue ) )
    {
        pClient->SetAudioChannels ( static_cast<EAudChanConf> ( iValue ) );
    }

    // audio quality
    if ( GetNumericIniSet ( IniXMLDocument, "client", "audioquality", 0, 3 /* AQ_RAW */, iValue ) )
    {
        pClient->SetAudioQuality ( static_cast<EAudioQuality> ( iValue ) );
    }

    // MIDI settings: Always read from XML first to preserve values
    if ( GetNumericIniSet ( IniXMLDocument, "client", "midichannel", 0, 16, iValue ) )
        iMidiChannel = iValue;

    struct MidiSettingEntry
    {
        const char* key;
        int*        variable;
    };
    MidiSettingEntry midiSettings[] = { { "midifaderoffset", &iMidiFaderOffset },
                                        { "midifadercount", &iMidiFaderCount },
                                        { "midipanoffset", &iMidiPanOffset },
                                        { "midipancount", &iMidiPanCount },
                                        { "midisolooffset", &iMidiSoloOffset },
                                        { "midisolocount", &iMidiSoloCount },
                                        { "midimuteoffset", &iMidiMuteOffset },
                                        { "midimutecount", &iMidiMuteCount },
                                        { "midimutemyself", &iMidiMuteMyself } };
    for ( const auto& entry : midiSettings )
    {
        if ( GetNumericIniSet ( IniXMLDocument, "client", entry.key, 0, 127, iValue ) )
            *( entry.variable ) = iValue;
    }
    if ( GetFlagIniSet ( IniXMLDocument, "client", "usemidicontroller", bValue ) )
        bUseMIDIController = bValue;
    if ( GetFlagIniSet ( IniXMLDocument, "client", "midipickupmode", bValue ) )
        bMIDIPickupMode = bValue;

    // Read enable flags
    if ( GetFlagIniSet ( IniXMLDocument, "client", "midifaderenabled", bValue ) )
        bMidiFaderEnabled = bValue;
    if ( GetFlagIniSet ( IniXMLDocument, "client", "midipanenabled", bValue ) )
        bMidiPanEnabled = bValue;
    if ( GetFlagIniSet ( IniXMLDocument, "client", "midisoloenabled", bValue ) )
        bMidiSoloEnabled = bValue;
    if ( GetFlagIniSet ( IniXMLDocument, "client", "midimuteenabled", bValue ) )
        bMidiMuteEnabled = bValue;
    if ( GetFlagIniSet ( IniXMLDocument, "client", "midimutemyselfenabled", bValue ) )
        bMidiMuteMyselfEnabled = bValue;

    // Read MIDI device name from settings
    strMidiDevice = GetIniSetting ( IniXMLDocument, "client", "mididevice_base64", "" );
    if ( !strMidiDevice.isEmpty() )
    {
        strMidiDevice = FromBase64ToString ( strMidiDevice );
    }

    // Command line overrides: disable all controls, then re-enable only those specified
    for ( const QString& option : CommandLineOptions )
    {
        if ( option.startsWith ( "--ctrlmidich=" ) )
        {
            QString strMidiMap = option.section ( '=', 1 );

            // Check if channel is valid before disabling section flags
            bool        bValidChannel = false;
            QStringList parts         = strMidiMap.split ( ';' );
            if ( !parts.isEmpty() && !strMidiMap.isEmpty() )
            {
                bool bIsNumber = false;
                int  iChannel  = parts[0].trimmed().toInt ( &bIsNumber );
                if ( bIsNumber && iChannel >= 0 && iChannel <= 16 )
                {
                    bValidChannel = true;
                }
            }

            // Only disable section flags if channel is valid - this allows command line
            // to specify which sections to enable. If channel is invalid/empty, preserve
            // ini file section settings but disable MIDI.
            if ( bValidChannel )
            {
                bMidiFaderEnabled      = false;
                bMidiPanEnabled        = false;
                bMidiSoloEnabled       = false;
                bMidiMuteEnabled       = false;
                bMidiMuteMyselfEnabled = false;
                bMIDIPickupMode        = false;
            }

            // Parse command line - this will update channel, enable/disable MIDI,
            // and re-enable any specified sections
            CClientSettings::ParseCtrlMidiCh ( strMidiMap,
                                               iMidiChannel,
                                               iMidiFaderOffset,
                                               iMidiFaderCount,
                                               iMidiPanOffset,
                                               iMidiPanCount,
                                               iMidiSoloOffset,
                                               iMidiSoloCount,
                                               iMidiMuteOffset,
                                               iMidiMuteCount,
                                               iMidiMuteMyself,
                                               bMidiFaderEnabled,
                                               bMidiPanEnabled,
                                               bMidiSoloEnabled,
                                               bMidiMuteEnabled,
                                               bMidiMuteMyselfEnabled,
                                               bUseMIDIController,
                                               bMIDIPickupMode,
                                               &strMidiDevice );
            break;
        }
    }

    // custom directories

    //### TODO: BEGIN ###//
    // compatibility to old version (< 3.6.1)
    QString strDirectoryAddress = GetIniSetting ( IniXMLDocument, "client", "centralservaddr", "" );
    //### TODO: END ###//

    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        //### TODO: BEGIN ###//
        // compatibility to old version (< 3.8.2)
        strDirectoryAddress = GetIniSetting ( IniXMLDocument, "client", QString ( "centralservaddr%1" ).arg ( iIdx ), strDirectoryAddress );
        //### TODO: END ###//

        vstrDirectoryAddress[iIdx] = GetIniSetting ( IniXMLDocument, "client", QString ( "directoryaddress%1" ).arg ( iIdx ), strDirectoryAddress );
        strDirectoryAddress        = "";
    }

    ReadEffectsPresetsFromXML ( IniXMLDocument, false );
    ReadEffectsPresetsFromXML ( IniXMLDocument, true );

    if ( GetNumericIniSet ( IniXMLDocument, "client", "selectedeffectspreset", -1, MAX_NUM_EFFECT_PRESETS, iValue ) )
    {
        iSelectedEffectsPreset[0] = iValue;
    }
    else
    {
        iSelectedEffectsPreset[0] = INVALID_INDEX;
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", "selectedouteffectspreset", -1, MAX_NUM_EFFECT_PRESETS, iValue ) )
    {
        iSelectedEffectsPreset[1] = iValue;
    }
    else
    {
        iSelectedEffectsPreset[1] = INVALID_INDEX;
    }

    // directory type

    //### TODO: BEGIN ###//
    // compatibility to old version (<3.4.7)
    // only the case that "centralservaddr" was set in old ini must be considered
    if ( !vstrDirectoryAddress[0].isEmpty() && GetFlagIniSet ( IniXMLDocument, "client", "defcentservaddr", bValue ) && !bValue )
    {
        eDirectoryType = AT_CUSTOM;
    }
    // compatibility to old version (< 3.8.2)
    else if ( GetNumericIniSet ( IniXMLDocument, "client", "centservaddrtype", 0, static_cast<int> ( AT_CUSTOM ), iValue ) )
    {
        eDirectoryType = static_cast<EDirectoryType> ( iValue );
    }
    //### TODO: END ###//

    else if ( GetNumericIniSet ( IniXMLDocument, "client", "directorytype", 0, static_cast<int> ( AT_CUSTOM ), iValue ) )
    {
        eDirectoryType = static_cast<EDirectoryType> ( iValue );
    }
    else
    {
        // if no address type is given, choose one from the operating system locale
        eDirectoryType = AT_DEFAULT;
    }

    // custom directory index
    if ( ( eDirectoryType == AT_CUSTOM ) &&
         GetNumericIniSet ( IniXMLDocument, "client", "customdirectoryindex", 0, MAX_NUM_SERVER_ADDR_ITEMS - 1, iValue ) )
    {
        iCustomDirectoryIndex = iValue;
    }
    else
    {
        // if directory is not set to custom, or if no custom directory index is found in the settings .ini file, then initialize to zero
        iCustomDirectoryIndex = 0;
    }

    // window position of the main window
    vecWindowPosMain = FromBase64ToByteArray ( GetIniSetting ( IniXMLDocument, "client", "winposmain_base64" ) );

    // window position of the settings window
    vecWindowPosSettings = FromBase64ToByteArray ( GetIniSetting ( IniXMLDocument, "client", "winposset_base64" ) );

    // window position of the chat window
    vecWindowPosChat = FromBase64ToByteArray ( GetIniSetting ( IniXMLDocument, "client", "winposchat_base64" ) );

    // window position of the effects window
    vecWindowPosEffects = FromBase64ToByteArray ( GetIniSetting ( IniXMLDocument, "client", "winposeff_base64" ) );

    // window position of the connect window
    vecWindowPosConnect = FromBase64ToByteArray ( GetIniSetting ( IniXMLDocument, "client", "winposcon_base64" ) );

    // visibility state of the settings window
    if ( GetFlagIniSet ( IniXMLDocument, "client", "winvisset", bValue ) )
    {
        bWindowWasShownSettings = bValue;
    }

    // visibility state of the chat window
    if ( GetFlagIniSet ( IniXMLDocument, "client", "winvischat", bValue ) )
    {
        bWindowWasShownChat = bValue;
    }

    // visibility state of the effects window
    if ( GetFlagIniSet ( IniXMLDocument, "client", "winviseff", bValue ) )
    {
        bWindowWasShownEffects = bValue;
    }

    // visibility state of the connect window
    if ( GetFlagIniSet ( IniXMLDocument, "client", "winviscon", bValue ) )
    {
        bWindowWasShownConnect = bValue;
    }

    // selected Settings Tab
    if ( GetNumericIniSet ( IniXMLDocument, "client", "settingstab", 0, 3, iValue ) )
    {
        iSettingsTab = iValue;
    }

    // selected Effects Tab
    if ( GetNumericIniSet ( IniXMLDocument, "client", "effectstab", 0, 3, iValue ) )
    {
        iEffectsTab = iValue;
    }

    // fader settings
    ReadFaderSettingsFromXML ( IniXMLDocument );
}
void CClientSettings::ReadFaderSettingsFromXML ( const QDomDocument& IniXMLDocument )
{
    int  iIdx;
    int  iValue;
    bool bValue;

    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        // stored fader tags
        QString strFaderTag =
            FromBase64ToString ( GetIniSetting ( IniXMLDocument, "client", QString ( "storedfadertag%1_base64" ).arg ( iIdx ), "" ) );

        if ( strFaderTag.isEmpty() )
        {
            // duplicate from clean up code
            continue;
        }

        vecStoredFaderTags[iIdx] = strFaderTag;

        // stored fader levels
        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "storedfaderlevel%1" ).arg ( iIdx ), 0, AUD_MIX_FADER_MAX, iValue ) )
        {
            vecStoredFaderLevels[iIdx] = iValue;
        }

        // stored pan values
        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "storedpanvalue%1" ).arg ( iIdx ), 0, AUD_MIX_PAN_MAX, iValue ) )
        {
            vecStoredPanValues[iIdx] = iValue;
        }

        // stored fader solo state
        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "storedfaderissolo%1" ).arg ( iIdx ), bValue ) )
        {
            vecStoredFaderIsSolo[iIdx] = bValue;
        }

        // stored fader muted state
        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "storedfaderismute%1" ).arg ( iIdx ), bValue ) )
        {
            vecStoredFaderIsMute[iIdx] = bValue;
        }

        // stored fader group ID
        if ( GetNumericIniSet ( IniXMLDocument,
                                "client",
                                QString ( "storedgroupid%1" ).arg ( iIdx ),
                                INVALID_INDEX,
                                MAX_NUM_FADER_GROUPS - 1,
                                iValue ) )
        {
            vecStoredFaderGroupID[iIdx] = iValue;
        }
    }
}

void CClientSettings::WriteSettingsToXML ( QDomDocument& IniXMLDocument, bool isAboutToQuit )
{
    Q_UNUSED ( isAboutToQuit )
    int iIdx;

    // IP addresses
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        PutIniSetting ( IniXMLDocument, "client", QString ( "ipaddress%1" ).arg ( iIdx ), vstrIPAddress[iIdx] );
    }

    // new client level
    SetNumericIniSet ( IniXMLDocument, "client", "newclientlevel", iNewClientFaderLevel );

    // input boost
    SetNumericIniSet ( IniXMLDocument, "client", "inputboost", iInputBoost );

    // feedback detection
    SetFlagIniSet ( IniXMLDocument, "client", "enablefeedbackdetection", bEnableFeedbackDetection );

    // connect dialog show all musicians
    SetFlagIniSet ( IniXMLDocument, "client", "connectdlgshowallmusicians", bConnectDlgShowAllMusicians );

    // language
    PutIniSetting ( IniXMLDocument, "client", "language", strLanguage );

    // fader channel sorting
    SetNumericIniSet ( IniXMLDocument, "client", "channelsort", static_cast<int> ( eChannelSortType ) );

    // own fader first sorting
    SetFlagIniSet ( IniXMLDocument, "client", "ownfaderfirst", bOwnFaderFirst );

    // number of mixer panel rows
    SetNumericIniSet ( IniXMLDocument, "client", "numrowsmixpan", iNumMixerPanelRows );

    // audio alerts
    SetFlagIniSet ( IniXMLDocument, "client", "enableaudioalerts", bEnableAudioAlerts );

    // name
    PutIniSetting ( IniXMLDocument, "client", "name_base64", ToBase64 ( pClient->ChannelInfo.strName ) );

    // instrument
    SetNumericIniSet ( IniXMLDocument, "client", "instrument", pClient->ChannelInfo.iInstrument );

    // country
    SetNumericIniSet ( IniXMLDocument, "client", "country", CLocale::QtCountryToWireFormatCountryCode ( pClient->ChannelInfo.eCountry ) );

    // city
    PutIniSetting ( IniXMLDocument, "client", "city_base64", ToBase64 ( pClient->ChannelInfo.strCity ) );

    // skill level
    SetNumericIniSet ( IniXMLDocument, "client", "skill", static_cast<int> ( pClient->ChannelInfo.eSkillLevel ) );

    // audio fader
    SetNumericIniSet ( IniXMLDocument, "client", "audfad", pClient->GetAudioInFader() );

    WriteReverbSettingsToXML ( IniXMLDocument, pClient, false );
    WriteReverbSettingsToXML ( IniXMLDocument, pClient, true );
    WriteCompressorSettingsToXML ( IniXMLDocument, pClient, false );
    WriteCompressorSettingsToXML ( IniXMLDocument, pClient, true );

    // sound card selection
    PutIniSetting ( IniXMLDocument, "client", "auddev_base64", ToBase64 ( pClient->GetSndCrdDev() ) );

    // sound card left input channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdinlch", pClient->GetSndCrdLeftInputChannel() );

    // sound card right input channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdinrch", pClient->GetSndCrdRightInputChannel() );

    // sound card left output channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutlch", pClient->GetSndCrdLeftOutputChannel() );

    // sound card right output channel mapping
    SetNumericIniSet ( IniXMLDocument, "client", "sndcrdoutrch", pClient->GetSndCrdRightOutputChannel() );

    // sound card preferred buffer size index
    SetNumericIniSet ( IniXMLDocument, "client", "prefsndcrdbufidx", pClient->GetSndCrdPrefFrameSizeFactor() );

    // automatic network jitter buffer size setting
    SetFlagIniSet ( IniXMLDocument, "client", "autojitbuf", pClient->GetDoAutoSockBufSize() );

    // network jitter buffer size
    SetNumericIniSet ( IniXMLDocument, "client", "jitbuf", pClient->GetSockBufNumFrames() );

    // network jitter buffer size for server
    SetNumericIniSet ( IniXMLDocument, "client", "jitbufserver", pClient->GetServerSockBufNumFrames() );

    // enable OPUS64 setting
    SetFlagIniSet ( IniXMLDocument, "client", "enableopussmall", pClient->GetEnableOPUS64() );

    WriteEQSettingsToXML ( IniXMLDocument, pClient, false );
    WriteEQSettingsToXML ( IniXMLDocument, pClient, true );

    WriteEffectsPresetsToXML ( IniXMLDocument, true );

    // GUI design
    SetNumericIniSet ( IniXMLDocument, "client", "guidesign", static_cast<int> ( pClient->GetGUIDesign() ) );

    // UI theme
    SetNumericIniSet ( IniXMLDocument, "client", "uitheme", static_cast<int> ( eUITheme ) );

    // MeterStyle
    SetNumericIniSet ( IniXMLDocument, "client", "meterstyle", static_cast<int> ( pClient->GetMeterStyle() ) );

    // audio channels
    SetNumericIniSet ( IniXMLDocument, "client", "audiochannels", static_cast<int> ( pClient->GetAudioChannels() ) );

    // audio quality
    SetNumericIniSet ( IniXMLDocument, "client", "audioquality", static_cast<int> ( pClient->GetAudioQuality() ) );

    // custom directories
    for ( iIdx = 0; iIdx < MAX_NUM_SERVER_ADDR_ITEMS; iIdx++ )
    {
        PutIniSetting ( IniXMLDocument, "client", QString ( "directoryaddress%1" ).arg ( iIdx ), vstrDirectoryAddress[iIdx] );
    }

    WriteEffectsPresetsToXML ( IniXMLDocument, false );

    // selected preset indices
    SetNumericIniSet ( IniXMLDocument, "client", "selectedeffectspreset", iSelectedEffectsPreset[0] );
    SetNumericIniSet ( IniXMLDocument, "client", "selectedouteffectspreset", iSelectedEffectsPreset[1] );

    // directory type
    SetNumericIniSet ( IniXMLDocument, "client", "directorytype", static_cast<int> ( eDirectoryType ) );

    // custom directory index
    SetNumericIniSet ( IniXMLDocument, "client", "customdirectoryindex", iCustomDirectoryIndex );

    // window position of the main window
    PutIniSetting ( IniXMLDocument, "client", "winposmain_base64", ToBase64 ( vecWindowPosMain ) );

    // window position of the settings window
    PutIniSetting ( IniXMLDocument, "client", "winposset_base64", ToBase64 ( vecWindowPosSettings ) );

    // window position of the chat window
    PutIniSetting ( IniXMLDocument, "client", "winposchat_base64", ToBase64 ( vecWindowPosChat ) );

    // window position of the effects window
    PutIniSetting ( IniXMLDocument, "client", "winposeff_base64", ToBase64 ( vecWindowPosEffects ) );

    // window position of the connect window
    PutIniSetting ( IniXMLDocument, "client", "winposcon_base64", ToBase64 ( vecWindowPosConnect ) );

    // visibility state of the settings window
    SetFlagIniSet ( IniXMLDocument, "client", "winvisset", bWindowWasShownSettings );

    // visibility state of the chat window
    SetFlagIniSet ( IniXMLDocument, "client", "winvischat", bWindowWasShownChat );

    // visibility state of the effects window
    SetFlagIniSet ( IniXMLDocument, "client", "winviseff", bWindowWasShownEffects );

    // visibility state of the connect window
    SetFlagIniSet ( IniXMLDocument, "client", "winviscon", bWindowWasShownConnect );

    // Settings Tab
    SetNumericIniSet ( IniXMLDocument, "client", "settingstab", iSettingsTab );

    // Effects Tab
    SetNumericIniSet ( IniXMLDocument, "client", "effectstab", iEffectsTab );

    // MIDI settings
    SetNumericIniSet ( IniXMLDocument, "client", "midichannel", iMidiChannel );
    SetNumericIniSet ( IniXMLDocument, "client", "midifaderoffset", iMidiFaderOffset );
    SetNumericIniSet ( IniXMLDocument, "client", "midifadercount", iMidiFaderCount );
    SetNumericIniSet ( IniXMLDocument, "client", "midipanoffset", iMidiPanOffset );
    SetNumericIniSet ( IniXMLDocument, "client", "midipancount", iMidiPanCount );
    SetNumericIniSet ( IniXMLDocument, "client", "midisolooffset", iMidiSoloOffset );
    SetNumericIniSet ( IniXMLDocument, "client", "midisolocount", iMidiSoloCount );
    SetNumericIniSet ( IniXMLDocument, "client", "midimuteoffset", iMidiMuteOffset );
    SetNumericIniSet ( IniXMLDocument, "client", "midimutecount", iMidiMuteCount );
    SetNumericIniSet ( IniXMLDocument, "client", "midimutemyself", iMidiMuteMyself );
    SetFlagIniSet ( IniXMLDocument, "client", "usemidicontroller", bUseMIDIController );
    SetFlagIniSet ( IniXMLDocument, "client", "midipickupmode", bMIDIPickupMode );
    SetFlagIniSet ( IniXMLDocument, "client", "midifaderenabled", bMidiFaderEnabled );
    SetFlagIniSet ( IniXMLDocument, "client", "midipanenabled", bMidiPanEnabled );
    SetFlagIniSet ( IniXMLDocument, "client", "midisoloenabled", bMidiSoloEnabled );
    SetFlagIniSet ( IniXMLDocument, "client", "midimuteenabled", bMidiMuteEnabled );
    SetFlagIniSet ( IniXMLDocument, "client", "midimutemyselfenabled", bMidiMuteMyselfEnabled );

    // Save MIDI device name
    if ( !strMidiDevice.isEmpty() )
    {
        PutIniSetting ( IniXMLDocument, "client", "mididevice_base64", ToBase64 ( strMidiDevice ) );
    }

    // fader settings
    WriteFaderSettingsToXML ( IniXMLDocument );
}

void CClientSettings::WriteFaderSettingsToXML ( QDomDocument& IniXMLDocument )
{
    int iIdx;

    for ( iIdx = 0; iIdx < MAX_NUM_STORED_FADER_SETTINGS; iIdx++ )
    {
        // stored fader tags
        PutIniSetting ( IniXMLDocument, "client", QString ( "storedfadertag%1_base64" ).arg ( iIdx ), ToBase64 ( vecStoredFaderTags[iIdx] ) );

        // stored fader levels
        SetNumericIniSet ( IniXMLDocument, "client", QString ( "storedfaderlevel%1" ).arg ( iIdx ), vecStoredFaderLevels[iIdx] );

        // stored pan values
        SetNumericIniSet ( IniXMLDocument, "client", QString ( "storedpanvalue%1" ).arg ( iIdx ), vecStoredPanValues[iIdx] );

        // stored fader solo states
        SetFlagIniSet ( IniXMLDocument, "client", QString ( "storedfaderissolo%1" ).arg ( iIdx ), vecStoredFaderIsSolo[iIdx] != 0 );

        // stored fader muted states
        SetFlagIniSet ( IniXMLDocument, "client", QString ( "storedfaderismute%1" ).arg ( iIdx ), vecStoredFaderIsMute[iIdx] != 0 );

        // stored fader group ID
        SetNumericIniSet ( IniXMLDocument, "client", QString ( "storedgroupid%1" ).arg ( iIdx ), vecStoredFaderGroupID[iIdx] );
    }
}
#endif

// Server settings -------------------------------------------------------------
// that this gets called means we are not headless
void CServerSettings::ReadSettingsFromXML ( const QDomDocument& IniXMLDocument, const QList<QString>& CommandLineOptions )
{
    int  iValue;
    bool bValue;

    // window position of the main window
    vecWindowPosMain = FromBase64ToByteArray ( GetIniSetting ( IniXMLDocument, "server", "winposmain_base64" ) );

    // name/city/country
    if ( !CommandLineOptions.contains ( "--serverinfo" ) )
    {
        // name
        pServer->SetServerName ( GetIniSetting ( IniXMLDocument, "server", "name" ) );

        // city
        pServer->SetServerCity ( GetIniSetting ( IniXMLDocument, "server", "city" ) );

        // country
        if ( GetNumericIniSet ( IniXMLDocument, "server", "country", 0, static_cast<int> ( QLocale::LastCountry ), iValue ) )
        {
            pServer->SetServerCountry ( CLocale::WireFormatCountryCodeToQtCountry ( iValue ) );
        }
    }

    // norecord flag
    if ( !CommandLineOptions.contains ( "--norecord" ) )
    {
        if ( GetFlagIniSet ( IniXMLDocument, "server", "norecord", bValue ) )
        {
            pServer->SetEnableRecording ( !bValue );
        }
    }

    // welcome message
    if ( !CommandLineOptions.contains ( "--welcomemessage" ) )
    {
        pServer->SetWelcomeMessage ( FromBase64ToString ( GetIniSetting ( IniXMLDocument, "server", "welcome" ) ) );
    }

    // language
    strLanguage =
        GetIniSetting ( IniXMLDocument, "server", "language", CLocale::FindSysLangTransFileName ( CLocale::GetAvailableTranslations() ).first );

    // UI theme
    if ( GetNumericIniSet ( IniXMLDocument, "server", "uitheme", 0, 2 /* UIT_DARK */, iValue ) )
    {
        eUITheme = ( iValue == 1 ) ? UIT_DARK : UIT_LIGHT;
    }

    // base recording directory
    if ( !CommandLineOptions.contains ( "--recording" ) )
    {
        pServer->SetRecordingDir ( FromBase64ToString ( GetIniSetting ( IniXMLDocument, "server", "recordingdir_base64" ) ) );
    }

    // to avoid multiple registrations, must do this after collecting serverinfo
    if ( !CommandLineOptions.contains ( "--centralserver" ) &&   // for backwards compatibility
         !CommandLineOptions.contains ( "--directoryserver" ) && // also for backwards compatibility
         !CommandLineOptions.contains ( "--directoryaddress" ) )
    {
        // custom directory
        // CServerListManager defaults to command line argument (or "" if not passed)
        // Server GUI defaults to ""
        QString directoryAddress = "";

        //### TODO: BEGIN ###//
        // compatibility to old version < 3.8.2
        directoryAddress = GetIniSetting ( IniXMLDocument, "server", "centralservaddr", directoryAddress );
        //### TODO: END ###//

        directoryAddress = GetIniSetting ( IniXMLDocument, "server", "directoryaddress", directoryAddress );

        pServer->SetDirectoryAddress ( directoryAddress );
    }

    // directory type
    // CServerListManager defaults to AT_NONE
    // Because type could be AT_CUSTOM, it has to be set after the address to avoid multiple registrations
    EDirectoryType directoryType = AT_NONE;

    // if a command line Directory address is set, set the Directory Type (genre) to AT_CUSTOM so it's used
    if ( CommandLineOptions.contains ( "--centralserver" ) || CommandLineOptions.contains ( "--directoryserver" ) ||
         CommandLineOptions.contains ( "--directoryaddress" ) )
    {
        directoryType = AT_CUSTOM;
    }
    else
    {
        //### TODO: BEGIN ###//
        // compatibility to old version < 3.4.7
        if ( GetFlagIniSet ( IniXMLDocument, "server", "defcentservaddr", bValue ) )
        {
            directoryType = bValue ? AT_DEFAULT : AT_CUSTOM;
        }
        else
        {
            //### TODO: END ###//

            // if "directorytype" itself is set, use it (note "AT_NONE", "AT_DEFAULT" and "AT_CUSTOM" are min/max directory type here)

            //### TODO: BEGIN ###//
            // compatibility to old version < 3.8.2
            if ( GetNumericIniSet ( IniXMLDocument,
                                    "server",
                                    "centservaddrtype",
                                    static_cast<int> ( AT_DEFAULT ),
                                    static_cast<int> ( AT_CUSTOM ),
                                    iValue ) )
            {
                directoryType = static_cast<EDirectoryType> ( iValue );
            }
            //### TODO: END ###//

            else
            {
                if ( GetNumericIniSet ( IniXMLDocument,
                                        "server",
                                        "directorytype",
                                        static_cast<int> ( AT_NONE ),
                                        static_cast<int> ( AT_CUSTOM ),
                                        iValue ) )
                {
                    directoryType = static_cast<EDirectoryType> ( iValue );
                }
            }
        }

        //### TODO: BEGIN ###//
        // compatibility to old version < 3.9.0
        // override type to AT_NONE if servlistenabled exists and is false
        if ( GetFlagIniSet ( IniXMLDocument, "server", "servlistenabled", bValue ) && !bValue )
        {
            directoryType = AT_NONE;
        }
        //### TODO: END ###//
    }

    pServer->SetDirectoryType ( directoryType );

    // server list persistence file name
    if ( !CommandLineOptions.contains ( "--directoryfile" ) )
    {
        pServer->SetServerListFileName ( FromBase64ToString ( GetIniSetting ( IniXMLDocument, "server", "directoryfile_base64" ) ) );
    }

    // start minimized on OS start
    if ( !CommandLineOptions.contains ( "--startminimized" ) )
    {
        if ( GetFlagIniSet ( IniXMLDocument, "server", "autostartmin", bValue ) )
        {
            pServer->SetAutoRunMinimized ( bValue );
        }
    }

    // delay panning
    if ( !CommandLineOptions.contains ( "--delaypan" ) )
    {
        if ( GetFlagIniSet ( IniXMLDocument, "server", "delaypan", bValue ) )
        {
            pServer->SetEnableDelayPanning ( bValue );
        }
    }
}

void CServerSettings::WriteSettingsToXML ( QDomDocument& IniXMLDocument, bool isAboutToQuit )
{
    // window position of the main window
    PutIniSetting ( IniXMLDocument, "server", "winposmain_base64", ToBase64 ( vecWindowPosMain ) );

    // directory type
    SetNumericIniSet ( IniXMLDocument, "server", "directorytype", static_cast<int> ( pServer->GetDirectoryType() ) );

    // name
    PutIniSetting ( IniXMLDocument, "server", "name", pServer->GetServerName() );

    // city
    PutIniSetting ( IniXMLDocument, "server", "city", pServer->GetServerCity() );

    // country
    SetNumericIniSet ( IniXMLDocument, "server", "country", CLocale::QtCountryToWireFormatCountryCode ( pServer->GetServerCountry() ) );

    // norecord flag
    SetFlagIniSet ( IniXMLDocument, "server", "norecord", pServer->GetDisableRecording() );

    // welcome message
    PutIniSetting ( IniXMLDocument, "server", "welcome", ToBase64 ( pServer->GetWelcomeMessage() ) );

    // language
    PutIniSetting ( IniXMLDocument, "server", "language", strLanguage );

    // UI theme
    SetNumericIniSet ( IniXMLDocument, "server", "uitheme", static_cast<int> ( eUITheme ) );

    // base recording directory
    PutIniSetting ( IniXMLDocument, "server", "recordingdir_base64", ToBase64 ( pServer->GetRecordingDir() ) );

    // custom directory
    PutIniSetting ( IniXMLDocument, "server", "directoryaddress", pServer->GetDirectoryAddress() );

    // server list persistence file name
    PutIniSetting ( IniXMLDocument, "server", "directoryfile_base64", ToBase64 ( pServer->GetServerListFileName() ) );

    // start minimized on OS start
    SetFlagIniSet ( IniXMLDocument, "server", "autostartmin", pServer->GetAutoRunMinimized() );

    // delay panning
    SetFlagIniSet ( IniXMLDocument, "server", "delaypan", pServer->IsDelayPanningEnabled() );

    // we MUST do this after saving the value and Save() is called OnAboutToQuit()
    if ( isAboutToQuit )
    {
        pServer->SetDirectoryType ( AT_NONE );
    }
}

void CClientSettings::SaveEffectsPresetFromClient ( int iPresetSlot, bool bIsOutput )
{
    if ( iPresetSlot >= 0 && iPresetSlot < MAX_NUM_EFFECT_PRESETS )
    {
        const int       ctx    = bIsOutput ? 1 : 0;
        SEffectsPreset& preset = EffectsPresets[ctx][iPresetSlot];

        preset.iReverbLevel        = pClient->GetReverbLevel ( bIsOutput );
        preset.bReverbOnLeftChan   = bIsOutput ? false : static_cast<bool> ( pClient->GetReverbOnLeftChan ( false ) );
        preset.iReverbPreDelayMs   = pClient->GetReverbPreDelayMs ( bIsOutput );
        preset.iReverbRoomSize     = pClient->GetReverbRoomSize ( bIsOutput );
        preset.iReverbDamping      = pClient->GetReverbDamping ( bIsOutput );
        preset.iReverbWetMix       = pClient->GetReverbWetMix ( bIsOutput );
        preset.iReverbEarlyLevel   = pClient->GetReverbEarlyLevel ( bIsOutput );
        preset.iReverbWidth        = pClient->GetReverbWidth ( bIsOutput );
        preset.bReverbEarlyEnabled = pClient->GetReverbEarlyEnabled ( bIsOutput );
        preset.bReverbFreeze       = pClient->GetReverbFreeze ( bIsOutput );
        preset.bReverbBypass       = pClient->GetReverbBypass ( bIsOutput );

        const CAudioCompressor& comp     = pClient->GetCompressor ( bIsOutput );
        preset.bCompressorBypass         = comp.GetBypass();
        preset.iCompressorThresholdDb    = static_cast<int> ( comp.GetThresholdDb() );
        preset.iCompressorRatio          = static_cast<int> ( comp.GetRatio() );
        preset.iCompressorAttackMs       = static_cast<int> ( comp.GetAttackMs() );
        preset.iCompressorReleaseMs      = static_cast<int> ( comp.GetReleaseMs() );
        preset.iCompressorMakeupDb       = static_cast<int> ( comp.GetMakeupDb() );
        preset.bCompressorLimiterEnabled = comp.GetLimiterEnabled();

        const CAudioEqualizer& eq = pClient->GetEQ ( bIsOutput );
        preset.bEQBypass          = eq.GetBypass();
        for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
        {
            preset.afEQBandGainDb[iBand]         = eq.GetBandGainDb ( iBand );
            preset.aiEQBandFrequency[iBand]      = static_cast<int> ( eq.GetBandFrequency ( iBand ) );
            preset.abEQBandDynEnabled[iBand]     = eq.GetBandDynEnabled ( iBand );
            preset.aiEQBandDynThresholdDb[iBand] = static_cast<int> ( eq.GetBandDynThresholdDb ( iBand ) );
            preset.aiEQBandDynRatio[iBand]       = static_cast<int> ( eq.GetBandDynRatio ( iBand ) );
            preset.aiEQBandDynAttackMs[iBand]    = static_cast<int> ( eq.GetBandDynAttackMs ( iBand ) );
            preset.aiEQBandDynReleaseMs[iBand]   = static_cast<int> ( eq.GetBandDynReleaseMs ( iBand ) );
            preset.aiEQBandQ[iBand]              = static_cast<int> ( std::round ( eq.GetBandQ ( iBand ) * 10.0f ) );
        }
    }
}

void CClientSettings::ReadEQSettingsFromXML ( const QDomDocument& IniXMLDocument, CClient* pClient, bool bIsOutput )
{
    const QString prefix  = bIsOutput ? "out_" : "";
    bool          bBypass = true;
    if ( GetFlagIniSet ( IniXMLDocument, "client", prefix + "eqbypass", bBypass ) )
    {
        pClient->GetEQ ( bIsOutput ).SetBypass ( bBypass );
    }

    for ( int iIdx = 0; iIdx < CAudioEqualizer::NUM_BANDS; ++iIdx )
    {
        QString strVal = GetIniSetting ( IniXMLDocument, "client", QString ( "%1eqbandgain%2" ).arg ( prefix ).arg ( iIdx ) );
        if ( !strVal.isEmpty() )
        {
            pClient->GetEQ ( bIsOutput ).SetBandGainDb ( iIdx, strVal.toFloat() );
        }

        bool bValue;
        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "%1eqbanddynenabled%2" ).arg ( prefix ).arg ( iIdx ), bValue ) )
        {
            pClient->GetEQ ( bIsOutput ).SetBandDynEnabled ( iIdx, bValue );
        }

        int iValue;
        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1eqbanddynthreshold%2" ).arg ( prefix ).arg ( iIdx ), -60, 0, iValue ) )
        {
            pClient->GetEQ ( bIsOutput ).SetBandDynThresholdDb ( iIdx, iValue );
        }

        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1eqbanddynratio%2" ).arg ( prefix ).arg ( iIdx ), 1, 20, iValue ) )
        {
            pClient->GetEQ ( bIsOutput ).SetBandDynRatio ( iIdx, iValue );
        }

        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1eqbanddynattack%2" ).arg ( prefix ).arg ( iIdx ), 1, 200, iValue ) )
        {
            pClient->GetEQ ( bIsOutput ).SetBandDynAttackMs ( iIdx, iValue );
        }

        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1eqbanddynrelease%2" ).arg ( prefix ).arg ( iIdx ), 10, 500, iValue ) )
        {
            pClient->GetEQ ( bIsOutput ).SetBandDynReleaseMs ( iIdx, iValue );
        }

        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1eqbandfrequency%2" ).arg ( prefix ).arg ( iIdx ), 20, 20000, iValue ) )
        {
            pClient->GetEQ ( bIsOutput ).SetBandFrequency ( iIdx, iValue );
        }

        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1eqbandq%2" ).arg ( prefix ).arg ( iIdx ), 3, 100, iValue ) )
        {
            pClient->GetEQ ( bIsOutput ).SetBandQ ( iIdx, static_cast<float> ( iValue ) / 10.0f );
        }
    }
}

void CClientSettings::WriteEQSettingsToXML ( QDomDocument& IniXMLDocument, const CClient* pClient, bool bIsOutput )
{
    const QString          prefix = bIsOutput ? "out_" : "";
    const CAudioEqualizer& eq     = const_cast<CClient*> ( pClient )->GetEQ ( bIsOutput );

    SetFlagIniSet ( IniXMLDocument, "client", prefix + "eqbypass", eq.GetBypass() );

    for ( int iIdx = 0; iIdx < CAudioEqualizer::NUM_BANDS; ++iIdx )
    {
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1eqbandgain%2" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( eq.GetBandGainDb ( iIdx ), 'f', 1 ) );
        SetFlagIniSet ( IniXMLDocument, "client", QString ( "%1eqbanddynenabled%2" ).arg ( prefix ).arg ( iIdx ), eq.GetBandDynEnabled ( iIdx ) );
        SetNumericIniSet ( IniXMLDocument,
                           "client",
                           QString ( "%1eqbanddynthreshold%2" ).arg ( prefix ).arg ( iIdx ),
                           static_cast<int> ( eq.GetBandDynThresholdDb ( iIdx ) ) );
        SetNumericIniSet ( IniXMLDocument,
                           "client",
                           QString ( "%1eqbanddynratio%2" ).arg ( prefix ).arg ( iIdx ),
                           static_cast<int> ( eq.GetBandDynRatio ( iIdx ) ) );
        SetNumericIniSet ( IniXMLDocument,
                           "client",
                           QString ( "%1eqbanddynattack%2" ).arg ( prefix ).arg ( iIdx ),
                           static_cast<int> ( eq.GetBandDynAttackMs ( iIdx ) ) );
        SetNumericIniSet ( IniXMLDocument,
                           "client",
                           QString ( "%1eqbanddynrelease%2" ).arg ( prefix ).arg ( iIdx ),
                           static_cast<int> ( eq.GetBandDynReleaseMs ( iIdx ) ) );
        SetNumericIniSet ( IniXMLDocument,
                           "client",
                           QString ( "%1eqbandfrequency%2" ).arg ( prefix ).arg ( iIdx ),
                           static_cast<int> ( eq.GetBandFrequency ( iIdx ) ) );
        SetNumericIniSet ( IniXMLDocument,
                           "client",
                           QString ( "%1eqbandq%2" ).arg ( prefix ).arg ( iIdx ),
                           static_cast<int> ( std::round ( eq.GetBandQ ( iIdx ) * 10.0f ) ) );
    }
}

void CClientSettings::ReadCompressorSettingsFromXML ( const QDomDocument& IniXMLDocument, CClient* pClient, bool bIsOutput )
{
    const QString prefix = bIsOutput ? "out_" : "";
    bool          bValue;
    int           iValue;

    if ( GetFlagIniSet ( IniXMLDocument, "client", prefix + "compbypass", bValue ) )
    {
        pClient->GetCompressor ( bIsOutput ).SetBypass ( bValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "compthreshold", -60, 0, iValue ) )
    {
        pClient->GetCompressor ( bIsOutput ).SetThresholdDb ( iValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "compratio", 1, 20, iValue ) )
    {
        pClient->GetCompressor ( bIsOutput ).SetRatio ( iValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "compattack", 1, 200, iValue ) )
    {
        pClient->GetCompressor ( bIsOutput ).SetAttackMs ( iValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "comprelease", 10, 500, iValue ) )
    {
        pClient->GetCompressor ( bIsOutput ).SetReleaseMs ( iValue );
    }

    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "compmakeup", 0, 24, iValue ) )
    {
        pClient->GetCompressor ( bIsOutput ).SetMakeupDb ( iValue );
    }

    if ( GetFlagIniSet ( IniXMLDocument, "client", prefix + "complimiter", bValue ) )
    {
        pClient->GetCompressor ( bIsOutput ).SetLimiterEnabled ( bValue );
    }
}

void CClientSettings::WriteCompressorSettingsToXML ( QDomDocument& IniXMLDocument, const CClient* pClient, bool bIsOutput )
{
    const QString           prefix = bIsOutput ? "out_" : "";
    const CAudioCompressor& comp   = const_cast<CClient*> ( pClient )->GetCompressor ( bIsOutput );

    SetFlagIniSet ( IniXMLDocument, "client", prefix + "compbypass", comp.GetBypass() );
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "compthreshold", static_cast<int> ( std::round ( comp.GetThresholdDb() ) ) );
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "compratio", static_cast<int> ( std::round ( comp.GetRatio() ) ) );
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "compattack", static_cast<int> ( std::round ( comp.GetAttackMs() ) ) );
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "comprelease", static_cast<int> ( std::round ( comp.GetReleaseMs() ) ) );
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "compmakeup", static_cast<int> ( std::round ( comp.GetMakeupDb() ) ) );
    SetFlagIniSet ( IniXMLDocument, "client", prefix + "complimiter", comp.GetLimiterEnabled() );
}

void CClientSettings::ReadReverbSettingsFromXML ( const QDomDocument& IniXMLDocument, CClient* pClient, bool bIsOutput )
{
    const QString prefix = bIsOutput ? "out_" : "";
    bool          bValue;
    int           iValue;

    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "revlev", 0, AUD_REVERB_MAX, iValue ) )
    {
        pClient->GetReverbLevel ( bIsOutput ) = iValue;
    }
    if ( !bIsOutput )
    {
        if ( GetFlagIniSet ( IniXMLDocument, "client", "reverblchan", bValue ) )
        {
            pClient->GetReverbOnLeftChan ( false ) = bValue;
        }
    }
    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "revpredelay", 0, REVERB_PRE_DELAY_MAX_MS, iValue ) )
    {
        pClient->GetReverbPreDelayMs ( bIsOutput ) = iValue;
    }
    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "revroom", 0, REVERB_ROOM_SIZE_MAX, iValue ) )
    {
        pClient->GetReverbRoomSize ( bIsOutput ) = iValue;
    }
    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "revdamping", 0, REVERB_DAMPING_MAX, iValue ) )
    {
        pClient->GetReverbDamping ( bIsOutput ) = iValue;
    }
    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "revwet", 0, REVERB_WET_MIX_MAX, iValue ) )
    {
        pClient->GetReverbWetMix ( bIsOutput ) = iValue;
    }
    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "revearlylevel", 0, REVERB_EARLY_LEVEL_MAX, iValue ) )
    {
        pClient->GetReverbEarlyLevel ( bIsOutput ) = iValue;
    }
    if ( GetFlagIniSet ( IniXMLDocument, "client", prefix + "revearlyenable", bValue ) )
    {
        pClient->GetReverbEarlyEnabled ( bIsOutput ) = bValue;
    }
    if ( GetNumericIniSet ( IniXMLDocument, "client", prefix + "revwidth", 0, REVERB_WIDTH_MAX, iValue ) )
    {
        pClient->GetReverbWidth ( bIsOutput ) = iValue;
    }
    if ( GetFlagIniSet ( IniXMLDocument, "client", prefix + "revfreeze", bValue ) )
    {
        pClient->GetReverbFreeze ( bIsOutput ) = bValue;
    }
    if ( GetFlagIniSet ( IniXMLDocument, "client", prefix + "revbypass", bValue ) )
    {
        pClient->GetReverbBypass ( bIsOutput ) = bValue;
    }
}

void CClientSettings::WriteReverbSettingsToXML ( QDomDocument& IniXMLDocument, const CClient* pClient, bool bIsOutput )
{
    const QString prefix          = bIsOutput ? "out_" : "";
    CClient*      pNonConstClient = const_cast<CClient*> ( pClient );

    SetNumericIniSet ( IniXMLDocument, "client", prefix + "revlev", pNonConstClient->GetReverbLevel ( bIsOutput ) );
    if ( !bIsOutput )
    {
        SetFlagIniSet ( IniXMLDocument, "client", "reverblchan", pNonConstClient->GetReverbOnLeftChan ( false ) );
    }
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "revpredelay", pNonConstClient->GetReverbPreDelayMs ( bIsOutput ) );
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "revroom", pNonConstClient->GetReverbRoomSize ( bIsOutput ) );
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "revdamping", pNonConstClient->GetReverbDamping ( bIsOutput ) );
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "revwet", pNonConstClient->GetReverbWetMix ( bIsOutput ) );
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "revearlylevel", pNonConstClient->GetReverbEarlyLevel ( bIsOutput ) );
    SetFlagIniSet ( IniXMLDocument, "client", prefix + "revearlyenable", pNonConstClient->GetReverbEarlyEnabled ( bIsOutput ) );
    SetNumericIniSet ( IniXMLDocument, "client", prefix + "revwidth", pNonConstClient->GetReverbWidth ( bIsOutput ) );
    SetFlagIniSet ( IniXMLDocument, "client", prefix + "revfreeze", pNonConstClient->GetReverbFreeze ( bIsOutput ) );
    SetFlagIniSet ( IniXMLDocument, "client", prefix + "revbypass", pNonConstClient->GetReverbBypass ( bIsOutput ) );
}

void CClientSettings::ReadEffectsPresetsFromXML ( const QDomDocument& IniXMLDocument, bool bIsOutput )
{
    const int     ctx    = bIsOutput ? 1 : 0;
    const QString prefix = bIsOutput ? "out_" : "";
    bool          bValue;
    int           iValue;

    for ( int iIdx = 0; iIdx < MAX_NUM_EFFECT_PRESETS; ++iIdx )
    {
        vstrEffectsPresetNames[ctx][iIdx] = FromBase64ToString (
            GetIniSetting ( IniXMLDocument, "client", QString ( "%1effectpresetname%2_base64" ).arg ( prefix ).arg ( iIdx ), "" ) );

        SEffectsPreset& p = EffectsPresets[ctx][iIdx];

        p.bEQBypass = true;
        for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
        {
            p.afEQBandGainDb[iBand] = 0.0f;
            QString strVal =
                GetIniSetting ( IniXMLDocument, "client", QString ( "%1effectpreset%2_eqband%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ) );
            if ( !strVal.isEmpty() )
            {
                float fVal = strVal.toFloat();
                if ( fVal >= -12.0f && fVal <= 12.0f )
                {
                    p.afEQBandGainDb[iBand] = fVal;
                }
            }

            p.abEQBandDynEnabled[iBand] = false;
            if ( GetFlagIniSet ( IniXMLDocument,
                                 "client",
                                 QString ( "%1effectpreset%2_eqbanddynenabled%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                                 bValue ) )
            {
                p.abEQBandDynEnabled[iBand] = bValue;
            }

            p.aiEQBandDynThresholdDb[iBand] = -20;
            if ( GetNumericIniSet ( IniXMLDocument,
                                    "client",
                                    QString ( "%1effectpreset%2_eqbanddynthreshold%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                                    -60,
                                    0,
                                    iValue ) )
            {
                p.aiEQBandDynThresholdDb[iBand] = iValue;
            }

            p.aiEQBandDynRatio[iBand] = 4;
            if ( GetNumericIniSet ( IniXMLDocument,
                                    "client",
                                    QString ( "%1effectpreset%2_eqbanddynratio%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                                    1,
                                    20,
                                    iValue ) )
            {
                p.aiEQBandDynRatio[iBand] = iValue;
            }

            p.aiEQBandDynAttackMs[iBand] = 5;
            if ( GetNumericIniSet ( IniXMLDocument,
                                    "client",
                                    QString ( "%1effectpreset%2_eqbanddynattack%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                                    1,
                                    200,
                                    iValue ) )
            {
                p.aiEQBandDynAttackMs[iBand] = iValue;
            }

            p.aiEQBandDynReleaseMs[iBand] = 80;
            if ( GetNumericIniSet ( IniXMLDocument,
                                    "client",
                                    QString ( "%1effectpreset%2_eqbanddynrelease%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                                    10,
                                    500,
                                    iValue ) )
            {
                p.aiEQBandDynReleaseMs[iBand] = iValue;
            }

            p.aiEQBandFrequency[iBand] = static_cast<int> ( CAudioEqualizer::GetDefaultBandFrequency ( iBand ) );
            if ( GetNumericIniSet ( IniXMLDocument,
                                    "client",
                                    QString ( "%1effectpreset%2_eqbandfrequency%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                                    20,
                                    20000,
                                    iValue ) )
            {
                p.aiEQBandFrequency[iBand] = iValue;
            }

            p.aiEQBandQ[iBand] = 10;
            if ( GetNumericIniSet ( IniXMLDocument,
                                    "client",
                                    QString ( "%1effectpreset%2_eqbandq%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                                    3,
                                    100,
                                    iValue ) )
            {
                p.aiEQBandQ[iBand] = iValue;
            }
        }

        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_eqbypass" ).arg ( prefix ).arg ( iIdx ), bValue ) )
        {
            p.bEQBypass = bValue;
        }

        p.iReverbLevel        = 0;
        p.iReverbPreDelayMs   = 0;
        p.iReverbRoomSize     = 60;
        p.iReverbDamping      = 30;
        p.iReverbWetMix       = 25;
        p.iReverbEarlyLevel   = 30;
        p.iReverbWidth        = 100;
        p.bReverbEarlyEnabled = true;
        p.bReverbFreeze       = false;
        p.bReverbBypass       = true;
        p.bReverbOnLeftChan   = false;

        if ( GetNumericIniSet ( IniXMLDocument,
                                "client",
                                QString ( "%1effectpreset%2_revlev" ).arg ( prefix ).arg ( iIdx ),
                                0,
                                AUD_REVERB_MAX,
                                iValue ) )
        {
            p.iReverbLevel = iValue;
        }
        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_reverblchan" ).arg ( prefix ).arg ( iIdx ), bValue ) )
        {
            p.bReverbOnLeftChan = bValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument,
                                "client",
                                QString ( "%1effectpreset%2_revpredelay" ).arg ( prefix ).arg ( iIdx ),
                                0,
                                REVERB_PRE_DELAY_MAX_MS,
                                iValue ) )
        {
            p.iReverbPreDelayMs = iValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument,
                                "client",
                                QString ( "%1effectpreset%2_revroom" ).arg ( prefix ).arg ( iIdx ),
                                0,
                                REVERB_ROOM_SIZE_MAX,
                                iValue ) )
        {
            p.iReverbRoomSize = iValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument,
                                "client",
                                QString ( "%1effectpreset%2_revdamping" ).arg ( prefix ).arg ( iIdx ),
                                0,
                                REVERB_DAMPING_MAX,
                                iValue ) )
        {
            p.iReverbDamping = iValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument,
                                "client",
                                QString ( "%1effectpreset%2_revwet" ).arg ( prefix ).arg ( iIdx ),
                                0,
                                REVERB_WET_MIX_MAX,
                                iValue ) )
        {
            p.iReverbWetMix = iValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument,
                                "client",
                                QString ( "%1effectpreset%2_revearlylevel" ).arg ( prefix ).arg ( iIdx ),
                                0,
                                REVERB_EARLY_LEVEL_MAX,
                                iValue ) )
        {
            p.iReverbEarlyLevel = iValue;
        }
        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_revearlyenable" ).arg ( prefix ).arg ( iIdx ), bValue ) )
        {
            p.bReverbEarlyEnabled = bValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument,
                                "client",
                                QString ( "%1effectpreset%2_revwidth" ).arg ( prefix ).arg ( iIdx ),
                                0,
                                REVERB_WIDTH_MAX,
                                iValue ) )
        {
            p.iReverbWidth = iValue;
        }
        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_revfreeze" ).arg ( prefix ).arg ( iIdx ), bValue ) )
        {
            p.bReverbFreeze = bValue;
        }
        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_revbypass" ).arg ( prefix ).arg ( iIdx ), bValue ) )
        {
            p.bReverbBypass = bValue;
        }

        p.bCompressorBypass         = true;
        p.iCompressorThresholdDb    = -12;
        p.iCompressorRatio          = 3;
        p.iCompressorAttackMs       = 5;
        p.iCompressorReleaseMs      = 120;
        p.iCompressorMakeupDb       = 3;
        p.bCompressorLimiterEnabled = true;

        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_compbypass" ).arg ( prefix ).arg ( iIdx ), bValue ) )
        {
            p.bCompressorBypass = bValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_compthreshold" ).arg ( prefix ).arg ( iIdx ), -60, 0, iValue ) )
        {
            p.iCompressorThresholdDb = iValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_compratio" ).arg ( prefix ).arg ( iIdx ), 1, 20, iValue ) )
        {
            p.iCompressorRatio = iValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_compattack" ).arg ( prefix ).arg ( iIdx ), 1, 200, iValue ) )
        {
            p.iCompressorAttackMs = iValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_comprelease" ).arg ( prefix ).arg ( iIdx ), 10, 500, iValue ) )
        {
            p.iCompressorReleaseMs = iValue;
        }
        if ( GetNumericIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_compmakeup" ).arg ( prefix ).arg ( iIdx ), 0, 24, iValue ) )
        {
            p.iCompressorMakeupDb = iValue;
        }
        if ( GetFlagIniSet ( IniXMLDocument, "client", QString ( "%1effectpreset%2_complimiter" ).arg ( prefix ).arg ( iIdx ), bValue ) )
        {
            p.bCompressorLimiterEnabled = bValue;
        }
    }
}

void CClientSettings::WriteEffectsPresetsToXML ( QDomDocument& IniXMLDocument, bool bIsOutput )
{
    const int     ctx    = bIsOutput ? 1 : 0;
    const QString prefix = bIsOutput ? "out_" : "";

    for ( int iIdx = 0; iIdx < MAX_NUM_EFFECT_PRESETS; ++iIdx )
    {
        if ( vstrEffectsPresetNames[ctx][iIdx].isEmpty() )
        {
            continue;
        }

        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpresetname%2_base64" ).arg ( prefix ).arg ( iIdx ),
                        ToBase64 ( vstrEffectsPresetNames[ctx][iIdx] ) );

        const SEffectsPreset& p = EffectsPresets[ctx][iIdx];

        PutIniSetting ( IniXMLDocument, "client", QString ( "%1effectpreset%2_eqbypass" ).arg ( prefix ).arg ( iIdx ), p.bEQBypass ? "1" : "0" );

        for ( int iBand = 0; iBand < CAudioEqualizer::NUM_BANDS; ++iBand )
        {
            PutIniSetting ( IniXMLDocument,
                            "client",
                            QString ( "%1effectpreset%2_eqband%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                            QString::number ( p.afEQBandGainDb[iBand], 'f', 1 ) );
            PutIniSetting ( IniXMLDocument,
                            "client",
                            QString ( "%1effectpreset%2_eqbanddynenabled%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                            p.abEQBandDynEnabled[iBand] ? "1" : "0" );
            PutIniSetting ( IniXMLDocument,
                            "client",
                            QString ( "%1effectpreset%2_eqbanddynthreshold%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                            QString::number ( p.aiEQBandDynThresholdDb[iBand] ) );
            PutIniSetting ( IniXMLDocument,
                            "client",
                            QString ( "%1effectpreset%2_eqbanddynratio%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                            QString::number ( p.aiEQBandDynRatio[iBand] ) );
            PutIniSetting ( IniXMLDocument,
                            "client",
                            QString ( "%1effectpreset%2_eqbanddynattack%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                            QString::number ( p.aiEQBandDynAttackMs[iBand] ) );
            PutIniSetting ( IniXMLDocument,
                            "client",
                            QString ( "%1effectpreset%2_eqbanddynrelease%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                            QString::number ( p.aiEQBandDynReleaseMs[iBand] ) );
            PutIniSetting ( IniXMLDocument,
                            "client",
                            QString ( "%1effectpreset%2_eqbandfrequency%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                            QString::number ( p.aiEQBandFrequency[iBand] ) );
            PutIniSetting ( IniXMLDocument,
                            "client",
                            QString ( "%1effectpreset%2_eqbandq%3" ).arg ( prefix ).arg ( iIdx ).arg ( iBand ),
                            QString::number ( p.aiEQBandQ[iBand] ) );
        }

        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_revlev" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iReverbLevel ) );
        if ( !bIsOutput )
        {
            PutIniSetting ( IniXMLDocument,
                            "client",
                            QString ( "%1effectpreset%2_reverblchan" ).arg ( prefix ).arg ( iIdx ),
                            p.bReverbOnLeftChan ? "1" : "0" );
        }
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_revpredelay" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iReverbPreDelayMs ) );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_revroom" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iReverbRoomSize ) );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_revdamping" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iReverbDamping ) );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_revwet" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iReverbWetMix ) );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_revearlylevel" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iReverbEarlyLevel ) );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_revearlyenable" ).arg ( prefix ).arg ( iIdx ),
                        p.bReverbEarlyEnabled ? "1" : "0" );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_revwidth" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iReverbWidth ) );
        PutIniSetting ( IniXMLDocument, "client", QString ( "%1effectpreset%2_revfreeze" ).arg ( prefix ).arg ( iIdx ), p.bReverbFreeze ? "1" : "0" );
        PutIniSetting ( IniXMLDocument, "client", QString ( "%1effectpreset%2_revbypass" ).arg ( prefix ).arg ( iIdx ), p.bReverbBypass ? "1" : "0" );

        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_compbypass" ).arg ( prefix ).arg ( iIdx ),
                        p.bCompressorBypass ? "1" : "0" );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_compthreshold" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iCompressorThresholdDb ) );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_compratio" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iCompressorRatio ) );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_compattack" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iCompressorAttackMs ) );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_comprelease" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iCompressorReleaseMs ) );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_compmakeup" ).arg ( prefix ).arg ( iIdx ),
                        QString::number ( p.iCompressorMakeupDb ) );
        PutIniSetting ( IniXMLDocument,
                        "client",
                        QString ( "%1effectpreset%2_complimiter" ).arg ( prefix ).arg ( iIdx ),
                        p.bCompressorLimiterEnabled ? "1" : "0" );
    }
}
