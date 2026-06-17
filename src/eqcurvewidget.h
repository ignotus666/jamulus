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

#pragma once

#include <QWidget>
#include <QVector>
#include "plugins/audioequalizer.h"

class CEQCurveWidget : public QWidget
{
    Q_OBJECT

public:
    static constexpr int   kNumBands   = CAudioEqualizer::NUM_BANDS;
    static constexpr float kGainMinDb  = -12.0f;
    static constexpr float kGainMaxDb  = 12.0f;
    static constexpr float kDisplayMin = -15.0f; // Extra range for gain-reduction visualisation
    static constexpr float kDisplayMax = 15.0f;
    static constexpr float kFreqMin    = 20.0f;
    static constexpr float kFreqMax    = 20000.0f;

    explicit CEQCurveWidget ( QWidget* parent = nullptr );

    // Band data (called from the dialog to keep widget in sync)
    void SetBandGain ( const int iBand, const float fGainDb );
    void SetBandFrequency ( const int iBand, const float fFreqHz );
    void SetBandGainReduction ( const int iBand, const float fReductionDb );
    void SetSpectrumLevels ( const QVector<float>& vecLevels );
    void SetSampleRate ( const int iSampleRateHz );
    void SetDarkTheme ( const bool bEnable );
    void SetBypassed ( const bool bBypassed );
    void SetBandQ ( const int iBand, const float fQ );

    int   GetSelectedBand() const { return iSelectedBand; }
    float GetBandFrequency ( const int iBand ) const;

    // Coordinate mapping (used by the dialog for output-band-meter alignment)
    int FreqToX ( const float fHz ) const;

signals:
    void bandGainChanged ( int iBand, int iGainDb );
    void bandFrequencyChanged ( int iBand, float fFreqHz );
    void bandSelected ( int iBand );
    void bandGainReset ( int iBand );

protected:
    void paintEvent ( QPaintEvent* pEvent ) override;
    void mousePressEvent ( QMouseEvent* pEvent ) override;
    void mouseMoveEvent ( QMouseEvent* pEvent ) override;
    void mouseReleaseEvent ( QMouseEvent* pEvent ) override;
    void mouseDoubleClickEvent ( QMouseEvent* pEvent ) override;
    void wheelEvent ( QWheelEvent* pEvent ) override;
    void resizeEvent ( QResizeEvent* pEvent ) override;
    void leaveEvent ( QEvent* pEvent ) override;

private:
    // Coordinate transforms (plot area ↔ data space)
    float  FreqToXf ( const float fHz ) const;
    float  DbToYf ( const float fDb ) const;
    float  XToFreq ( const float fX ) const;
    float  YToDb ( const float fY ) const;
    QRectF PlotRect() const;

    // Biquad transfer-function evaluation for curve rendering
    float EvalBandMagnitudeDb ( const int iBand, const float fGainDb, const float fFreqHz ) const;
    void  ComputeResponseCurve ( const float* afGains, QVector<QPointF>& vecPoints ) const;

    int  FindNearestBand ( const QPointF& pos, float* pfDistOut = nullptr ) const;
    void UpdateBandTooltip ( const int iBand, const bool bVisible = true );
    void UpdateBandTooltipStyle();

    // State
    float afBandGainDb[kNumBands];
    float afBandFrequencies[kNumBands];
    float afBandGainReductionDb[kNumBands];
    float afBandQ[kNumBands];
    float afSpectrumLevels[kNumBands];
    int   iSampleRateHz;
    int   iSelectedBand;
    bool  bDragging;
    bool  bDarkTheme;
    bool  bEQBypassed;

    // Cached curves
    QVector<QPointF> vecStaticCurveCache;
    QVector<QPointF> vecEffectiveCurveCache;
    bool             bStaticCurveDirty;
    bool             bEffectiveCurveDirty;
    class QLabel*    pBandTooltip;

    // Layout constants
    static constexpr int kMarginLeft   = 42;
    static constexpr int kMarginRight  = 16;
    static constexpr int kMarginTop    = 12;
    static constexpr int kMarginBottom = 28;
    static constexpr int kNodeRadius   = 6;
    static constexpr int kHitRadius    = 16;
};
