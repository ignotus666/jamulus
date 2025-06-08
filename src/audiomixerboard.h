/******************************************************************************\
 * Copyright (c) 2004-2025
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

#include <QFrame>
#include <QScrollArea>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QLayout>
#include <QString>
#include <QSlider>
#include <QDial>
#include <QSizePolicy>
#include <QHostAddress>
#include <QListWidget>
#include <QMenu>
#include <QMutex>
#include <QTextBoundaryFinder>
#include "global.h"
#include "util.h"
#include "levelmeter.h"
#include "settings.h"

/* Classes ********************************************************************/
class CChannelFader : public QObject
{
    Q_OBJECT

public:
    CChannelFader ( QWidget* pNW );

    QString GetReceivedName() { return cReceivedChanInfo.strName; }
    int     GetReceivedInstrument() { return cReceivedChanInfo.iInstrument; }
    QString GetReceivedCity() { return cReceivedChanInfo.strCity; }
    int     GetReceivedChID() { return cReceivedChanInfo.iChanID; }
    void    SetChannelInfos ( const CChannelInfo& cChanInfo );
    void    Show() { pFrame->show(); }
    void    Hide() { pFrame->hide(); }
    bool    IsVisible() { return !pFrame->isHidden(); }
    bool    IsSolo() { return pcbSolo->isChecked(); }
    bool    IsMute() { return pcbMute->isChecked(); }
    int     GetGroupID() { return iGroupID; }
    void    SetGUIDesign ( const EGUIDesign eNewDesign );
    void    SetMeterStyle ( const EMeterStyle eNewMeterStyle );
    void    SetDisplayChannelLevel ( const bool eNDCL );
    bool    GetDisplayChannelLevel();
    void    SetDisplayPans ( const bool eNDP );
    QFrame* GetMainWidget() { return pFrame; }

    void SetPanValue ( const int iPan, const bool bIsMIDIUpdate = false );
    void SetFaderIsSolo ( const bool bIsSolo );
    void SetFaderIsMute ( const bool bIsMute );
    void SetGroupID ( const int iNGroupID );
    void SetRemoteFaderIsMute ( const bool bIsMute );
    void SetFaderLevel ( const double dLevel, const bool bIsGroupUpdate = false, const bool bIsMIDIUpdate = false );

    int    GetFaderLevel() { return pFader->value(); }
    double GetPreviousFaderLevel() { return dPreviousFaderLevel; }
    int    GetPanValue() { return pPan->value(); }
    void   Reset();
    void   SetRunningNewClientCnt ( const int iNRunningNewClientCnt ) { iRunningNewClientCnt = iNRunningNewClientCnt; }
    int    GetRunningNewClientCnt() { return iRunningNewClientCnt; }
    void   SetChannelLevel ( const uint16_t iLevel );
    void   SetIsMyOwnFader() { bIsMyOwnFader = true; }
    bool   GetIsMyOwnFader() { return bIsMyOwnFader; }
    void   UpdateSoloState ( const bool bNewOtherSoloState );

    void SetMIDICtrlUsed ( const bool isMIDICtrlUsed ) { 
        bool wasPreviouslyUsed = bMIDICtrlUsed;
        bMIDICtrlUsed = isMIDICtrlUsed; 
        
        // Reset pickup state whenever MIDI controller usage changes
        // This ensures controller must pick up value after enabling MIDI
        bFaderPickedUp = false;
        bPanPickedUp = false;
        
        // Reset the MIDI values to force re-pickup
        iLastMIDIFaderVal = -1;
        iLastMIDIPanVal = -1;
        
        qDebug() << "MIDI ctrl used changed from" << wasPreviouslyUsed << "to" << isMIDICtrlUsed 
                << ", reset pickup state - Fader:" << bFaderPickedUp 
                << "Pan:" << bPanPickedUp
                << "Values reset to require pickup";
    }
    bool GetMIDICtrlUsed() const { return bMIDICtrlUsed; }
    
    // For MIDI pickup mode
    void SetLastMIDIFaderVal ( const int iValue ) { iLastMIDIFaderVal = iValue; }
    int GetLastMIDIFaderVal() { return iLastMIDIFaderVal; }
    void SetLastMIDIPanVal ( const int iValue ) { iLastMIDIPanVal = iValue; }
    int GetLastMIDIPanVal() { return iLastMIDIPanVal; }
    void SetFaderPickedUp ( const bool bValue ) { bFaderPickedUp = bValue; }
    bool GetFaderPickedUp() { return bFaderPickedUp; }
    void SetPanPickedUp ( const bool bValue ) { bPanPickedUp = bValue; }
    bool GetPanPickedUp() { return bPanPickedUp; }

protected:
    void UpdateGroupIDDependencies();
    void SetMute ( const bool bState );
    void SetupFaderTag ( const ESkillLevel eSkillLevel );
    void SendPanValueToServer ( const int iPan );
    void SendFaderLevelToServer ( const double dLevel, const bool bIsGroupUpdate );

    QFrame* pFrame;

    QWidget*     pLevelsBox;
    QWidget*     pMuteSoloBox;
    CLevelMeter* plbrChannelLevel;
    QSlider*     pFader;
    QDial*       pPan;
    QLabel*      pPanLabel;
    QLabel*      pInfoLabel;
    QHBoxLayout* pLabelGrid;
    QVBoxLayout* pLabelPictGrid;

    QCheckBox* pcbMute;
    QCheckBox* pcbSolo;
    QCheckBox* pcbGroup;
    QMenu*     pGroupPopupMenu;

    QGroupBox* pLabelInstBox;
    QLabel*    plblLabel;
    QLabel*    plblInstrument;
    QLabel*    plblCountryFlag;

    CChannelInfo cReceivedChanInfo;

    bool        bOtherChannelIsSolo;
    bool        bIsMyOwnFader;
    bool        bIsMutedAtServer;
    double      dPreviousFaderLevel;
    int         iGroupID;
    QString     strGroupBaseText;
    int         iRunningNewClientCnt;
    int         iInstrPicMaxWidth;
    EGUIDesign  eDesign;
    EMeterStyle eMeterStyle;
    QPixmap     BitmapMutedIcon;
    bool        bMIDICtrlUsed;
    int         iLastMIDIFaderVal; // for MIDI pickup mode
    int         iLastMIDIPanVal;   // for MIDI pickup mode
    bool        bFaderPickedUp;    // for MIDI pickup mode sticky window
    bool        bPanPickedUp;      // for MIDI pickup mode sticky window

public slots:
    void OnLevelValueChanged ( int value )
    {
        SendFaderLevelToServer ( value,
                                 QGuiApplication::keyboardModifiers() ==
                                     Qt::ShiftModifier ); /* isolate a channel from the group temporarily with shift-click-drag (#695) */
    }

    void OnPanValueChanged ( int value );
    void OnMuteStateChanged ( int value );
    void OnGroupStateChanged ( int );

    void OnGroupMenuGrp ( int iGrp ) { SetGroupID ( iGrp ); }

signals:
    void gainValueChanged ( float value, bool bIsMyOwnFader, bool bIsGroupUpdate, bool bSuppressServerUpdate, double dLevelRatio );

    void panValueChanged ( float value );
    void soloStateChanged ( int value );
};

template<unsigned int slotId>
class CAudioMixerBoardSlots : public CAudioMixerBoardSlots<slotId - 1>
{
public:
    void OnChGainValueChanged ( float fValue, bool bIsMyOwnFader, bool bIsGroupUpdate, bool bSuppressServerUpdate, double dLevelRatio )
    {
        UpdateGainValue ( slotId - 1, fValue, bIsMyOwnFader, bIsGroupUpdate, bSuppressServerUpdate, dLevelRatio );
    }

    void OnChPanValueChanged ( float fValue ) { UpdatePanValue ( slotId - 1, fValue ); }

protected:
    virtual void UpdateGainValue ( const int    iChannelIdx,
                                   const float  fValue,
                                   const bool   bIsMyOwnFader,
                                   const bool   bIsGroupUpdate,
                                   const bool   bSuppressServerUpdate,
                                   const double dLevelRatio ) = 0;

    virtual void UpdatePanValue ( const int iChannelIdx, const float fValue ) = 0;
};

template<>
class CAudioMixerBoardSlots<0>
{};

class CAudioMixerBoard : public QGroupBox, public CAudioMixerBoardSlots<MAX_NUM_CHANNELS>
{
    Q_OBJECT

public:
    CAudioMixerBoard ( QWidget* parent = nullptr );

    virtual ~CAudioMixerBoard();

    void    SetSettingsPointer ( CClientSettings* pNSet ) { pSettings = pNSet; }
    void    HideAll();
    void    ApplyNewConClientList ( CVector<CChannelInfo>& vecChanInfo );
    void    SetServerName ( const QString& strNewServerName );
    QString GetServerName() { return strServerName; }
    void    SetGUIDesign ( const EGUIDesign eNewDesign );
    void    SetMeterStyle ( const EMeterStyle eNewMeterStyle );
    void    SetDisplayPans ( const bool eNDP );
    void    SetPanIsSupported();
    void    SetRemoteFaderIsMute ( const int iChannelIdx, const bool bIsMute );
    void    SetMyChannelID ( const int iChannelIdx ) { iMyChannelID = iChannelIdx; }
    int     GetMyChannelID() const { return iMyChannelID; }

    void SetFaderLevel ( const int iChannelIdx, const int iValue );

    void SetPanValue ( const int iChannelIdx, const int iValue );

    void SetFaderIsSolo ( const int iChannelIdx, const bool bIsSolo );

    void SetFaderIsMute ( const int iChannelIdx, const bool bIsMute );

    void SetNumMixerPanelRows ( const int iNNumMixerPanelRows );
    int  GetNumMixerPanelRows() { return iNumMixerPanelRows; }

    void        SetFaderSorting ( const EChSortType eNChSortType );
    EChSortType GetFaderSorting() { return eChSortType; }

    void SetChannelLevels ( const CVector<uint16_t>& vecChannelLevel );

    void           SetRecorderState ( const ERecorderState newRecorderState );
    ERecorderState GetRecorderState() { return eRecorderState; };

    void SetAllFaderLevelsToNewClientLevel();
    void StoreAllFaderSettings();
    void LoadAllFaderSettings();
    void AutoAdjustAllFaderLevels();

    void MuteMyChannel();

    void SetMIDICtrlUsed ( const bool bMIDICtrlUsed );
    
    // For MIDI pickup mode
    void SetMIDIPickupMode ( const bool bIsMIDIPickupMode ) { 
        bool wasPreviouslyEnabled = bMIDIPickupModeEnabled;
        bMIDIPickupModeEnabled = bIsMIDIPickupMode; 
        qDebug() << "MIDI Pickup Mode changed from" << (wasPreviouslyEnabled ? "ENABLED" : "DISABLED")
                 << "to" << (bMIDIPickupModeEnabled ? "ENABLED" : "DISABLED");
        
        // Always force complete reset of pickup states when changing mode
        QMutexLocker locker ( &Mutex );
        for ( size_t i = 0; i < MAX_NUM_CHANNELS; i++ )
        {
            if (vecpChanFader[i]->GetMIDICtrlUsed())
            {
                // Reset pickup states for all channels
                vecpChanFader[i]->SetFaderPickedUp(false);
                vecpChanFader[i]->SetPanPickedUp(false);
                
                // Reset the stored MIDI values to force re-pickup
                vecpChanFader[i]->SetLastMIDIFaderVal(-1);
                vecpChanFader[i]->SetLastMIDIPanVal(-1);
                
                qDebug() << "Reset pickup state for channel" << i 
                        << "- Required pickup on next controller movement";
            }
        }
    }
    
    bool GetMIDICtrlUsed(const int iChannelIdx) 
    {
        if ((iChannelIdx >= 0) && (iChannelIdx < MAX_NUM_CHANNELS))
        {
            return vecpChanFader[static_cast<size_t>(iChannelIdx)]->GetMIDICtrlUsed();
        }
        return false;
    }

protected:
    class CMixerBoardScrollArea : public QScrollArea
    {
    public:
        CMixerBoardScrollArea ( QWidget* parent = nullptr ) : QScrollArea ( parent ) {}

    protected:
        virtual void resizeEvent ( QResizeEvent* event )
        {
            // if after a resize of the main window a vertical scroll bar is required, make
            // sure that the fader label is visible (scroll down completely)
            ensureVisible ( 0, 2000 ); // use a large value here
            QScrollArea::resizeEvent ( event );
        }
    };

    void ChangeFaderOrder ( const EChSortType eChSortType );

    bool GetStoredFaderSettings ( const QString& strName,
                                  int&           iStoredFaderLevel,
                                  int&           iStoredPanValue,
                                  bool&          bStoredFaderIsSolo,
                                  bool&          bStoredFaderIsMute,
                                  int&           iGroupID );

    void StoreFaderSettings ( CChannelFader* pChanFader );
    void UpdateSoloStates();
    void UpdateTitle();

    CClientSettings*        pSettings;
    CVector<CChannelFader*> vecpChanFader;
    CMixerBoardScrollArea*  pScrollArea;
    QGridLayout*            pMainLayout;
    bool                    bDisplayPans;
    bool                    bIsPanSupported;
    bool                    bNoFaderVisible;
    int                     iMyChannelID;         // must use int (not size_t) so INVALID_INDEX can be stored
    int                     iRunningNewClientCnt; // integer type is sufficient, will never overrun for its purpose
    int                     iNumMixerPanelRows;
    QString                 strServerName;
    ERecorderState          eRecorderState;
    QMutex                  Mutex;
    EChSortType             eChSortType;
    CVector<float>          vecAvgLevels;
    bool                    bMIDIPickupModeEnabled;

    virtual void UpdateGainValue ( const int    iChannelIdx,
                                   const float  fValue,
                                   const bool   bIsMyOwnFader,
                                   const bool   bIsGroupUpdate,
                                   const bool   bSuppressServerUpdate,
                                   const double dLevelRatio );

    virtual void UpdatePanValue ( const int iChannelIdx, const float fValue );

    template<unsigned int slotId>
    inline void connectFaderSignalsToMixerBoardSlots();

signals:
    void ChangeChanGain ( int iId, float fGain, bool bIsMyOwnFader );
    void ChangeChanPan ( int iId, float fPan );
    void NumClientsChanged ( int iNewNumClients );
};
