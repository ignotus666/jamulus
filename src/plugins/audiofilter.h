/******************************************************************************\
* Audio Filters                                                               *
\******************************************************************************/

#pragma once

#include "util.h"

class CAudioFilter
{
public:
    CAudioFilter();

    void Init ( const int iNSampleRateHz );
    void SetBypass ( const bool bNBypass ) { bBypass = bNBypass; }
    bool GetBypass() const { return bBypass; }

    void SetHighPassEnabled ( const bool bEnabled );
    void SetLowPassEnabled ( const bool bEnabled );
    bool GetHighPassEnabled() const { return bHighPassEnabled; }
    bool GetLowPassEnabled() const { return bLowPassEnabled; }

    void SetHighPassCutoffHz ( const int iHz );
    void SetLowPassCutoffHz ( const int iHz );
    int  GetHighPassCutoffHz() const { return iHighPassCutoffHz; }
    int  GetLowPassCutoffHz() const { return iLowPassCutoffHz; }

    void Process ( CVector<int16_t>& vecsStereoInOut, const int iStereoBlockSizeSam );

private:
    struct SCoeff
    {
        float b0;
        float b1;
        float b2;
        float a1;
        float a2;
    };

    struct SState
    {
        float x1[2];
        float x2[2];
        float y1[2];
        float y2[2];
    };

    void UpdateHighPassCoeff();
    void UpdateLowPassCoeff();
    void ClearState ( SState& state );

    bool   bBypass;
    bool   bHighPassEnabled;
    bool   bLowPassEnabled;
    int    iSampleRateHz;
    int    iHighPassCutoffHz;
    int    iLowPassCutoffHz;
    float  fQ;
    SCoeff sHighPassCoeff;
    SCoeff sLowPassCoeff;
    SState sHighPassState;
    SState sLowPassState;
};
