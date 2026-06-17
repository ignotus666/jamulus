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

#pragma once

#include <QHostAddress>
#include <QHostInfo>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <atomic>
#ifdef USE_OPUS_SHARED_LIB
#    include "opus/opus_custom.h"
#else
#    include "opus_custom.h"
#endif
#include "global.h"
#include "socket.h"
#include "channel.h"
#include "util.h"
#include "plugins/audioreverb.h"
#include "plugins/audioequalizer.h"
#include "plugins/audiocompressor.h"
#include "buffer.h"
#include "signalhandler.h"

#if defined( _WIN32 ) && !defined( JACK_ON_WINDOWS )
#    include "sound/asio/sound.h"
#else
#    if ( defined( Q_OS_MACOS ) ) && !defined( JACK_REPLACES_COREAUDIO )
#        include "sound/coreaudio-mac/sound.h"
#    else
#        if defined( Q_OS_IOS )
#            include "sound/coreaudio-ios/sound.h"
#        else
#            ifdef ANDROID
#                include "sound/oboe/sound.h"
#            else
#                include "sound/jack/sound.h"
#                ifndef JACK_ON_WINDOWS // these headers are not available in Windows OS
#                    include <sched.h>
#                    include <netdb.h>
#                endif
#                include <socket.h>
#            endif
#        endif
#    endif
#endif

/* Definitions ****************************************************************/
// audio in fader range
#define AUD_FADER_IN_MIN    0
#define AUD_FADER_IN_MAX    100
#define AUD_FADER_IN_MIDDLE ( AUD_FADER_IN_MAX / 2 )

// audio reverberation range
#define AUD_REVERB_MAX          100
#define REVERB_PRE_DELAY_MAX_MS 120
#define REVERB_ROOM_SIZE_MAX    100
#define REVERB_DAMPING_MAX      100
#define REVERB_WET_MIX_MAX      100
#define REVERB_EARLY_LEVEL_MAX  100
#define REVERB_WIDTH_MAX        100

// default delay period between successive gain updates (ms)
// this will be increased to double the ping time if connected to a distant server
#define DEFAULT_GAIN_DELAY_PERIOD_MS 50

// OPUS number of coded bytes per audio packet
// TODO we have to use new numbers for OPUS to avoid that old CELT packets
// are used in the OPUS decoder (which gives a bad noise output signal).
// Later on when the CELT is completely removed we could set the OPUS
// numbers back to the original CELT values (to reduce network load)

// calculation to get from the number of bytes to the code rate in bps:
// rate [pbs] = Fs / L * N * 8, where
// Fs: sampling rate (SYSTEM_SAMPLE_RATE_HZ)
// L:  number of samples per packet (SYSTEM_FRAME_SIZE_SAMPLES)
// N:  number of bytes per packet (values below)
#define OPUS_NUM_BYTES_MONO_LOW_QUALITY                   12
#define OPUS_NUM_BYTES_MONO_NORMAL_QUALITY                22
#define OPUS_NUM_BYTES_MONO_HIGH_QUALITY                  36
#define OPUS_NUM_BYTES_MONO_LOW_QUALITY_DBLE_FRAMESIZE    25
#define OPUS_NUM_BYTES_MONO_NORMAL_QUALITY_DBLE_FRAMESIZE 45
#define OPUS_NUM_BYTES_MONO_HIGH_QUALITY_DBLE_FRAMESIZE   82

#define OPUS_NUM_BYTES_STEREO_LOW_QUALITY                   24
#define OPUS_NUM_BYTES_STEREO_NORMAL_QUALITY                35
#define OPUS_NUM_BYTES_STEREO_HIGH_QUALITY                  73
#define OPUS_NUM_BYTES_STEREO_LOW_QUALITY_DBLE_FRAMESIZE    47
#define OPUS_NUM_BYTES_STEREO_NORMAL_QUALITY_DBLE_FRAMESIZE 71
#define OPUS_NUM_BYTES_STEREO_HIGH_QUALITY_DBLE_FRAMESIZE   165

/* Classes ********************************************************************/

class CClientChannel
{
public:
    int iServerChannelID; // unused channels will contain INVALID_INDEX
    int iJoinSequence;    // order of joining of session participants

    float oldGain, newGain; // for rate-limiting sending of gain messages
    float oldPan, newPan;   // for rate-limiting sending of pan messages

    uint16_t level; // last value of level meter received for channel

    // can store here other information about an active channel
};

class CClientSettings;

class CClient : public QObject
{
    Q_OBJECT

public:
    CClient ( const quint16  iPortNumber,
              const quint16  iQosNumber,
              const QString& strConnOnStartupAddress,
              const bool     bNoAutoJackConnect,
              const QString& strNClientName,
              const bool     bNDisableIPv6,
              const bool     bNMuteMeInPersonalMix );

    virtual ~CClient();

    void Start();
    void Stop();
    bool IsRunning() { return Sound.IsRunning(); }
    bool IsCallbackEntered() const { return Sound.IsCallbackEntered(); }
    bool SetServerAddr ( QString strNAddr );

    // IPv6 Available
    bool IsIPv6Available() { return bIPv6Available; }

    double GetLevelForMeterdBLeft() { return SignalLevelMeter.GetLevelForMeterdBLeftOrMono(); }
    double GetLevelForMeterdBRight() { return SignalLevelMeter.GetLevelForMeterdBRight(); }
    void   GetOutputBandLevels ( CVector<float>& vecOutLevels );

    bool GetAndResetbJitterBufferOKFlag();

    bool IsConnected() { return Channel.IsConnected(); }

    EGUIDesign GetGUIDesign() const { return eGUIDesign; }
    void       SetGUIDesign ( const EGUIDesign eNGD ) { eGUIDesign = eNGD; }

    EMeterStyle GetMeterStyle() const { return eMeterStyle; }
    void        SetMeterStyle ( const EMeterStyle eNMT ) { eMeterStyle = eNMT; }

    EAudioQuality GetAudioQuality() const { return eAudioQuality; }
    void          SetAudioQuality ( const EAudioQuality eNAudioQuality );

    EAudChanConf GetAudioChannels() const { return eAudioChannelConf; }
    void         SetAudioChannels ( const EAudChanConf eNAudChanConf );

    int  GetAudioInFader() const { return iAudioInFader; }
    void SetAudioInFader ( const int iNV ) { iAudioInFader = iNV; }

    int  GetReverbLevel() const { return iReverbLevel; }
    void SetReverbLevel ( const int iNL ) { iReverbLevel = iNL; }

    bool GetReverbBypass() const { return bReverbBypass; }
    void SetReverbBypass ( const bool bBypass ) { bReverbBypass = bBypass; }

    int  GetReverbPreDelayMs() const { return iReverbPreDelayMs; }
    void SetReverbPreDelayMs ( const int iMs ) { iReverbPreDelayMs = iMs; }

    int  GetReverbRoomSize() const { return iReverbRoomSize; }
    void SetReverbRoomSize ( const int iValue ) { iReverbRoomSize = iValue; }

    int  GetReverbDamping() const { return iReverbDamping; }
    void SetReverbDamping ( const int iValue ) { iReverbDamping = iValue; }

    int  GetReverbWetMix() const { return iReverbWetMix; }
    void SetReverbWetMix ( const int iValue ) { iReverbWetMix = iValue; }

    int  GetReverbEarlyLevel() const { return iReverbEarlyLevel; }
    void SetReverbEarlyLevel ( const int iValue ) { iReverbEarlyLevel = iValue; }

    bool GetReverbEarlyEnabled() const { return bReverbEarlyEnabled; }
    void SetReverbEarlyEnabled ( const bool bEnabled ) { bReverbEarlyEnabled = bEnabled; }

    int  GetReverbWidth() const { return iReverbWidth; }
    void SetReverbWidth ( const int iValue ) { iReverbWidth = iValue; }

    bool GetReverbFreeze() const { return bReverbFreeze; }
    void SetReverbFreeze ( const bool bEnabled ) { bReverbFreeze = bEnabled; }

    bool IsReverbOnLeftChan() const { return bReverbOnLeftChan; }
    void SetReverbOnLeftChan ( const bool bIL )
    {
        bReverbOnLeftChan = bIL;
        AudioReverb.Clear();
    }

    void SetDoAutoSockBufSize ( const bool bValue );
    bool GetDoAutoSockBufSize() const { return Channel.GetDoAutoSockBufSize(); }

    void  SetEQBypass ( const bool bNBypass ) { AudioEqualizer.SetBypass ( bNBypass ); }
    void  SetEQBandGainDb ( const int iBandIndex, const int iGainDb ) { AudioEqualizer.SetBandGainDb ( iBandIndex, iGainDb ); }
    void  SetEQBandFrequency ( const int iBandIndex, const float fFreqHz ) { AudioEqualizer.SetBandFrequency ( iBandIndex, fFreqHz ); }
    void  ResetEQ() { AudioEqualizer.Reset(); }
    bool  GetEQBypass() const { return AudioEqualizer.GetBypass(); }
    int   GetEQBandGainDb ( const int iBandIndex ) const { return static_cast<int> ( AudioEqualizer.GetBandGainDb ( iBandIndex ) ); }
    float GetEQBandFrequency ( const int iBandIndex ) const { return AudioEqualizer.GetBandFrequency ( iBandIndex ); }

    void  SetEQBandQ ( const int iBand, const float fQ ) { AudioEqualizer.SetBandQ ( iBand, fQ ); }
    float GetEQBandQ ( const int iBand ) const { return AudioEqualizer.GetBandQ ( iBand ); }

    void  SetEQBandDynEnabled ( const int iBand, const bool bEnabled ) { AudioEqualizer.SetBandDynEnabled ( iBand, bEnabled ); }
    bool  GetEQBandDynEnabled ( const int iBand ) const { return AudioEqualizer.GetBandDynEnabled ( iBand ); }
    void  SetEQBandDynThresholdDb ( const int iBand, const float fDb ) { AudioEqualizer.SetBandDynThresholdDb ( iBand, fDb ); }
    float GetEQBandDynThresholdDb ( const int iBand ) const { return AudioEqualizer.GetBandDynThresholdDb ( iBand ); }
    void  SetEQBandDynRatio ( const int iBand, const float fRatio ) { AudioEqualizer.SetBandDynRatio ( iBand, fRatio ); }
    float GetEQBandDynRatio ( const int iBand ) const { return AudioEqualizer.GetBandDynRatio ( iBand ); }
    void  SetEQBandDynAttackMs ( const int iBand, const float fMs ) { AudioEqualizer.SetBandDynAttackMs ( iBand, fMs ); }
    float GetEQBandDynAttackMs ( const int iBand ) const { return AudioEqualizer.GetBandDynAttackMs ( iBand ); }
    void  SetEQBandDynReleaseMs ( const int iBand, const float fMs ) { AudioEqualizer.SetBandDynReleaseMs ( iBand, fMs ); }
    float GetEQBandDynReleaseMs ( const int iBand ) const { return AudioEqualizer.GetBandDynReleaseMs ( iBand ); }
    float GetEQBandGainReductionDb ( const int iBand ) const { return AudioEqualizer.GetBandGainReductionDb ( iBand ); }

    void  SetCompressorBypass ( const bool bNBypass ) { AudioCompressor.SetBypass ( bNBypass ); }
    bool  GetCompressorBypass() const { return AudioCompressor.GetBypass(); }
    void  SetCompressorThresholdDb ( const float fDb ) { AudioCompressor.SetThresholdDb ( fDb ); }
    float GetCompressorThresholdDb() const { return AudioCompressor.GetThresholdDb(); }
    void  SetCompressorRatio ( const float fValue ) { AudioCompressor.SetRatio ( fValue ); }
    float GetCompressorRatio() const { return AudioCompressor.GetRatio(); }
    void  SetCompressorAttackMs ( const float fMs ) { AudioCompressor.SetAttackMs ( fMs ); }
    float GetCompressorAttackMs() const { return AudioCompressor.GetAttackMs(); }
    void  SetCompressorReleaseMs ( const float fMs ) { AudioCompressor.SetReleaseMs ( fMs ); }
    float GetCompressorReleaseMs() const { return AudioCompressor.GetReleaseMs(); }
    void  SetCompressorMakeupDb ( const float fDb ) { AudioCompressor.SetMakeupDb ( fDb ); }
    float GetCompressorMakeupDb() const { return AudioCompressor.GetMakeupDb(); }
    void  SetCompressorLimiterEnabled ( const bool bEnabled ) { AudioCompressor.SetLimiterEnabled ( bEnabled ); }
    bool  GetCompressorLimiterEnabled() const { return AudioCompressor.GetLimiterEnabled(); }
    float GetCompressorGainReductionDb() { return AudioCompressor.GetGainReductionDb(); }

    void SetSockBufNumFrames ( const int iNumBlocks, const bool bPreserve = false ) { Channel.SetSockBufNumFrames ( iNumBlocks, bPreserve ); }
    int  GetSockBufNumFrames() { return Channel.GetSockBufNumFrames(); }

    void SetServerSockBufNumFrames ( const int iNumBlocks )
    {
        iServerSockBufNumFrames = iNumBlocks;

        // if auto setting is disabled, inform the server about the new size
        if ( !GetDoAutoSockBufSize() )
        {
            Channel.CreateJitBufMes ( iServerSockBufNumFrames );
        }
    }
    int GetServerSockBufNumFrames() { return iServerSockBufNumFrames; }

    int GetUploadRateKbps() { return Channel.GetUploadRateKbps(); }

    // sound card device selection
    QStringList GetSndCrdDevNames() { return Sound.GetDevNames(); }

    QString SetSndCrdDev ( const QString strNewDev );
    QString GetSndCrdDev() { return Sound.GetDev(); }
    void    OpenSndCrdDriverSetup() { Sound.OpenDriverSetup(); }

    // sound card channel selection
    int     GetSndCrdNumInputChannels() { return Sound.GetNumInputChannels(); }
    QString GetSndCrdInputChannelName ( const int iDiD ) { return Sound.GetInputChannelName ( iDiD ); }
    void    SetSndCrdLeftInputChannel ( const int iNewChan );
    void    SetSndCrdRightInputChannel ( const int iNewChan );
    int     GetSndCrdLeftInputChannel() { return Sound.GetLeftInputChannel(); }
    int     GetSndCrdRightInputChannel() { return Sound.GetRightInputChannel(); }

    int     GetSndCrdNumOutputChannels() { return Sound.GetNumOutputChannels(); }
    QString GetSndCrdOutputChannelName ( const int iDiD ) { return Sound.GetOutputChannelName ( iDiD ); }
    void    SetSndCrdLeftOutputChannel ( const int iNewChan );
    void    SetSndCrdRightOutputChannel ( const int iNewChan );
    int     GetSndCrdLeftOutputChannel() { return Sound.GetLeftOutputChannel(); }
    int     GetSndCrdRightOutputChannel() { return Sound.GetRightOutputChannel(); }

    void SetSndCrdPrefFrameSizeFactor ( const int iNewFactor );
    int  GetSndCrdPrefFrameSizeFactor() { return iSndCrdPrefFrameSizeFactor; }

    void SetEnableOPUS64 ( const bool eNEnableOPUS64 );
    bool GetEnableOPUS64() { return bEnableOPUS64; }

    int GetSndCrdActualMonoBlSize()
    {
        // the actual sound card mono block size depends on whether a
        // sound card conversion buffer is used or not
        if ( bSndCrdConversionBufferRequired )
        {
            return iSndCardMonoBlockSizeSamConvBuff;
        }
        else
        {
            return iMonoBlockSizeSam;
        }
    }
    int GetSystemMonoBlSize() { return iMonoBlockSizeSam; }
    int GetSndCrdConvBufAdditionalDelayMonoBlSize()
    {
        if ( bSndCrdConversionBufferRequired )
        {
            // by introducing the conversion buffer we also introduce additional
            // delay which equals the "internal" mono buffer size
            return iMonoBlockSizeSam;
        }
        else
        {
            return 0;
        }
    }

    bool GetFraSiFactPrefSupported() { return bFraSiFactPrefSupported; }
    bool GetFraSiFactDefSupported() { return bFraSiFactDefSupported; }
    bool GetFraSiFactSafeSupported() { return bFraSiFactSafeSupported; }

    void SetMuteOutStream ( const bool bDoMute ) { bMuteOutStream = bDoMute; }

    void SetOutputBandLevelsEnabled ( const bool bEnabled ) { bOutputBandLevelsEnabled = bEnabled; }
    bool GetOutputBandLevelsEnabled() const { return bOutputBandLevelsEnabled; }

    void SetRemoteChanGain ( const int iId, const float fGain, const bool bIsMyOwnFader );
    void SetRemoteChanPan ( const int iId, const float fPan );
    void OnTimerRemoteChanGainOrPan();
    void StartTimerGainOrPan();

    void SetControllerInFaderLevel ( int iChannelIdx, int iValue ) { OnControllerInFaderLevel ( iChannelIdx, iValue ); }

    void SetInputBoost ( const int iNewBoost ) { iInputBoost = iNewBoost; }

    void SetRemoteInfo() { Channel.SetRemoteInfo ( ChannelInfo ); }

    void CreateChatTextMes ( const QString& strChatText ) { Channel.CreateChatTextMes ( strChatText ); }

    void CreateCLPingMes() { ConnLessProtocol.CreateCLPingMes ( Channel.GetAddress(), PreparePingMessage() ); }

    void CreateCLServerListPingMes ( const CHostAddress& InetAddr )
    {
        ConnLessProtocol.CreateCLPingWithNumClientsMes ( InetAddr, PreparePingMessage(), 0 /* dummy */ );
    }

    void CreateCLServerListReqVerAndOSMes ( const CHostAddress& InetAddr ) { ConnLessProtocol.CreateCLReqVersionAndOSMes ( InetAddr ); }

    void CreateCLServerListReqConnClientsListMes ( const CHostAddress& InetAddr ) { ConnLessProtocol.CreateCLReqConnClientsListMes ( InetAddr ); }

    void CreateCLReqServerListMes ( const CHostAddress& InetAddr ) { ConnLessProtocol.CreateCLReqServerListMes ( InetAddr ); }

    int EstimatedOverallDelay ( const int iPingTimeMs );

    void GetBufErrorRates ( CVector<double>& vecErrRates, double& dLimit, double& dMaxUpLimit )
    {
        Channel.GetBufErrorRates ( vecErrRates, dLimit, dMaxUpLimit );
    }

    //### TODO: BEGIN ###//
    // Refactor this to use signal/slot mechanism. https://github.com/jamulussoftware/jamulus/pull/3479/files#r1976382416
    CProtocol* getConnLessProtocol() { return &ConnLessProtocol; }
    //### TODO: END ###//

    // MIDI control
    void        EnableMIDI ( bool bEnable ) { Sound.EnableMIDI ( bEnable ); }
    bool        IsMIDIEnabled() const { return Sound.IsMIDIEnabled(); }
    QStringList GetMIDIDevNames() { return Sound.GetMIDIDevNames(); }
    QString     GetMIDIDevice() { return Sound.GetMIDIDevice(); }
    void        SetMIDIDevice ( const QString& strDevice ) { Sound.SetMIDIDevice ( strDevice ); }

    // settings
    CChannelCoreInfo ChannelInfo;
    QString          strClientName;

public:
    void SetSettings ( CClientSettings* settings );

protected:
    // Signal handler must be declared before pSettings for correct init order
    CSignalHandler* pSignalHandler;
    // Pointer to settings for MIDI and other config
    CClientSettings* pSettings;
    // callback function must be static, otherwise it does not work
    static void AudioCallback ( CVector<short>& psData, void* arg );

    void Init();
    void ProcessSndCrdAudioData ( CVector<short>& vecsStereoSndCrd );
    void ProcessAudioDataIntern ( CVector<short>& vecsStereoSndCrd );
    void UpdateOutputBandLevels ( const CVector<int16_t>& vecsStereoSndCrd );

    int  PreparePingMessage();
    int  EvaluatePingMessage ( const int iMs );
    void CreateServerJitterBufferMessage();

    void ClearClientChannels();
    void FreeClientChannel ( const int iServerChannelID );
    int  FindClientChannel ( const int iServerChannelID, const bool bCreateIfNew ); // returns a client channel ID or INVALID_INDEX
    bool ReorderLevelList ( CVector<uint16_t>& vecLevelList );                      // modifies vecLevelList, passed by reference

    // only one channel is needed for client application
    CChannel  Channel;
    CProtocol ConnLessProtocol;

    // client channels, indexed by client channel ID,
    // containing server channel ID (INVALID_INDEX if free)
    CClientChannel clientChannels[MAX_NUM_CHANNELS];

    // client channel IDs, indexed by server channel ID
    // unused channels will contain INVALID_INDEX
    int clientChannelIDs[MAX_NUM_CHANNELS];

    int    iActiveChannels; // number of active channels
    int    iJoinSequence;   // order of joining of session participants
    QMutex MutexChannels;

    // audio encoder/decoder
    OpusCustomMode*        Opus64Mode;
    OpusCustomEncoder*     Opus64EncoderMono;
    OpusCustomDecoder*     Opus64DecoderMono;
    OpusCustomEncoder*     Opus64EncoderStereo;
    OpusCustomDecoder*     Opus64DecoderStereo;
    OpusCustomMode*        OpusMode;
    OpusCustomEncoder*     OpusEncoderMono;
    OpusCustomDecoder*     OpusDecoderMono;
    OpusCustomEncoder*     OpusEncoderStereo;
    OpusCustomDecoder*     OpusDecoderStereo;
    OpusCustomEncoder*     CurOpusEncoder;
    OpusCustomDecoder*     CurOpusDecoder;
    EAudComprType          eAudioCompressionType;
    int                    iCeltNumCodedBytes;
    int                    iOPUSFrameSizeSamples;
    EAudioQuality          eAudioQuality;
    EAudChanConf           eAudioChannelConf;
    int                    iNumAudioChannels;
    bool                   bIsInitializationPhase;
    bool                   bMuteOutStream;
    float                  fMuteOutStreamGain;
    CVector<unsigned char> vecCeltData;

    bool            bIPv6Available; // must be before Socket - passed by reference to Socket
    CHighPrioSocket Socket;

    CSound                  Sound;
    CStereoSignalLevelMeter SignalLevelMeter;

    CVector<uint8_t> vecbyNetwData;

    int              iAudioInFader;
    bool             bReverbOnLeftChan;
    int              iReverbLevel;
    int              iReverbPreDelayMs;
    int              iReverbRoomSize;
    int              iReverbDamping;
    int              iReverbWetMix;
    int              iReverbEarlyLevel;
    int              iReverbWidth;
    bool             bReverbEarlyEnabled;
    bool             bReverbFreeze;
    bool             bReverbBypass;
    CAudioReverb     AudioReverb;
    CAudioEqualizer  AudioEqualizer;
    CAudioCompressor AudioCompressor;
    int              iInputBoost;

    int iSndCrdPrefFrameSizeFactor;
    int iSndCrdFrameSizeFactor;

    bool             bSndCrdConversionBufferRequired;
    int              iSndCardMonoBlockSizeSamConvBuff;
    CBuffer<int16_t> SndCrdConversionBufferIn;
    CBuffer<int16_t> SndCrdConversionBufferOut;
    CVector<int16_t> vecDataConvBuf;
    CVector<int16_t> vecsStereoSndCrdMuteStream;
    CVector<int16_t> vecZeros;

    bool bFraSiFactPrefSupported;
    bool bFraSiFactDefSupported;
    bool bFraSiFactSafeSupported;

    int iMonoBlockSizeSam;
    int iStereoBlockSizeSam;

    EGUIDesign  eGUIDesign;
    EMeterStyle eMeterStyle;
    bool        bEnableAudioAlerts;
    bool        bEnableOPUS64;

    bool              bJitterBufferOK;
    bool              bMuteMeInPersonalMix;
    QMutex            MutexDriverReinit;
    QMutex            MutexOutputBandLevels;
    float             afOutputBandLevels[8];
    std::atomic<bool> bOutputBandLevelsEnabled;

    // server settings
    int  iServerSockBufNumFrames;
    bool bRawAudioIsSupported;

    // for ping measurement
    QElapsedTimer PreciseTime;

    // for gain or pan rate limiting
    QMutex MutexGainOrPan;
    QTimer TimerGainOrPan;
    int    minGainOrPanId;
    int    maxGainOrPanId;
    int    iCurPingTime;

protected slots:
    void OnHandledSignal ( int sigNum );
    void OnSendProtMessage ( CVector<uint8_t> vecMessage );
    void OnInvalidPacketReceived ( CHostAddress RecHostAddr );

    void OnDetectedCLMessage ( CVector<uint8_t> vecbyMesBodyData, int iRecID, CHostAddress RecHostAddr );

    void OnReqJittBufSize() { CreateServerJitterBufferMessage(); }
    void OnJittBufSizeChanged ( int iNewJitBufSize );
    void OnReqChanInfo() { Channel.SetRemoteInfo ( ChannelInfo ); }
    void OnNewConnection();
    void OnCLDisconnection ( CHostAddress InetAddr )
    {
        if ( InetAddr == Channel.GetAddress() )
        {
            emit Disconnected();
        }
    }
    void OnCLPingReceived ( CHostAddress InetAddr, int iMs );

    void OnSendCLProtMessage ( CHostAddress InetAddr, CVector<uint8_t> vecMessage );

    void OnCLPingWithNumClientsReceived ( CHostAddress InetAddr, int iMs, int iNumClients );

    void OnSndCrdReinitRequest ( int iSndCrdResetType );
    void OnControllerInFaderLevel ( int iChannelIdx, int iValue );
    void OnControllerInPanValue ( int iChannelIdx, int iValue );
    void OnControllerInFaderIsSolo ( int iChannelIdx, bool bIsSolo );
    void OnControllerInFaderIsMute ( int iChannelIdx, bool bIsMute );
    void OnControllerInMuteMyself ( bool bMute );
    void OnClientIDReceived ( int iServerChanID );
    void OnRawAudioSupported();
    void OnMuteStateHasChangedReceived ( int iServerChanID, bool bIsMuted );
    void OnCLChannelLevelListReceived ( CHostAddress InetAddr, CVector<uint16_t> vecLevelList );
    void OnConClientListMesReceived ( CVector<CChannelInfo> vecChanInfo );

signals:
    void ConClientListMesReceived ( CVector<CChannelInfo> vecChanInfo );
    void ChatTextReceived ( QString strChatText );
    void ClientIDReceived ( int iChanID );
    void MuteStateHasChangedReceived ( int iChanID, bool bIsMuted );
    void LicenceRequired ( ELicenceType eLicenceType );
    void VersionAndOSReceived ( COSUtil::EOpSystemType eOSType, QString strVersion );
    void PingTimeReceived ( int iPingTime );
    void RecorderStateReceived ( ERecorderState eRecorderState );

    void CLServerListReceived ( CHostAddress InetAddr, CVector<CServerInfo> vecServerInfo );

    void CLRedServerListReceived ( CHostAddress InetAddr, CVector<CServerInfo> vecServerInfo );

    void CLConnClientsListMesReceived ( CHostAddress InetAddr, CVector<CChannelInfo> vecChanInfo );

    void CLPingTimeWithNumClientsReceived ( CHostAddress InetAddr, int iPingTime, int iNumClients );

    void CLVersionAndOSReceived ( CHostAddress InetAddr, COSUtil::EOpSystemType eOSType, QString strVersion );

    void CLChannelLevelListReceived ( CHostAddress InetAddr, CVector<uint16_t> vecLevelList );

    void Disconnected();
    void SoundDeviceChanged ( QString strError );
    void ControllerInFaderLevel ( int iChannelIdx, int iValue );
    void ControllerInPanValue ( int iChannelIdx, int iValue );
    void ControllerInFaderIsSolo ( int iChannelIdx, bool bIsSolo );
    void ControllerInFaderIsMute ( int iChannelIdx, bool bIsMute );
    void ControllerInMuteMyself ( bool bMute );
    void MidiCCReceived ( int channel, int ccNumber, int midiValue );

private slots:
    void OnMidiCCReceived ( int channel, int ccNumber, int midiValue );
};
