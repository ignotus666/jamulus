#pragma once

#include "../../global.h"
#include "../../util.h"
#include <vector>
#include <cmath>

// Stub class for CStereoSignalLevelMeter to avoid QWidget dependency
class CStereoSignalLevelMeter
{
public:
    CStereoSignalLevelMeter() :
        dLevelLeft ( 0.0 ),
        dLevelRight ( 0.0 ),
        dPeakLeft ( 0.0 ),
        dPeakRight ( 0.0 ),
        dDecay ( 0.001 )
    {
    }

    void Update ( CVector<int16_t>& vecsStereoSndCrd, int iMonoBlockSizeSam, bool bEnable )
    {
        if ( !bEnable ) return;

        double maxL = 0;
        double maxR = 0;

        for ( int i = 0; i < iMonoBlockSizeSam; i++ )
        {
            double l = std::abs ( static_cast<double> ( vecsStereoSndCrd[i * 2] ) / 32768.0 );
            double r = std::abs ( static_cast<double> ( vecsStereoSndCrd[i * 2 + 1] ) / 32768.0 );

            if ( l > maxL ) maxL = l;
            if ( r > maxR ) maxR = r;
        }

        // Apply decay
        dLevelLeft *= ( 1.0 - dDecay );
        dLevelRight *= ( 1.0 - dDecay );

        if ( maxL > dLevelLeft ) dLevelLeft = maxL;
        if ( maxR > dLevelRight ) dLevelRight = maxR;
    }

    void Reset()
    {
        dLevelLeft = 0.0;
        dLevelRight = 0.0;
    }

    // Getters for dB conversion if needed, or linear
    double GetLevelForMeterdBLeftOrMono()
    {
        return ( dLevelLeft > 0.000001 ) ? 20.0 * std::log10(dLevelLeft) : -100.0;
    }

    double GetLevelForMeterdBRight()
    {
        return ( dLevelRight > 0.000001 ) ? 20.0 * std::log10(dLevelRight) : -100.0;
    }

private:
    double dLevelLeft;
    double dLevelRight;
    double dPeakLeft;
    double dPeakRight;
    double dDecay;
};
