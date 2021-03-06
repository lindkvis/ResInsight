/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017- Statoil ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RimEnsembleCurveSetCollection.h"

#include "RiaApplication.h"
#include "RiaColorTables.h"

#include "RifReaderEclipseSummary.h"

#include "RimEnsembleCurveSet.h"
#include "RimProject.h"
#include "RimSummaryCase.h"
#include "RimSummaryCrossPlot.h"
#include "RimSummaryCurve.h"
#include "RimSummaryCurveAppearanceCalculator.h"
#include "RimSummaryPlot.h"
#include "RimSummaryPlotSourceStepping.h"

#include "RiuLineSegmentQwtPlotCurve.h"
#include "RiuSummaryQwtPlot.h"

CAF_PDM_SOURCE_INIT(RimEnsembleCurveSetCollection, "RimEnsembleCurveSetCollection");

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimEnsembleCurveSetCollection::RimEnsembleCurveSetCollection()
{
    CAF_PDM_InitObject("Ensemble Curve Sets", ":/EnsembleCurveSets16x16.png", "", "");

    CAF_PDM_InitFieldNoDefault(&m_curveSets, "EnsembleCurveSets", "Ensemble Curve Sets", "", "", "");
    m_curveSets.uiCapability()->setUiHidden(true);
    m_curveSets.uiCapability()->setUiTreeChildrenHidden(false);

    CAF_PDM_InitField(&m_showCurves, "IsActive", true, "Show Curves", "", "", "");
    m_showCurves.uiCapability()->setUiHidden(true);
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimEnsembleCurveSetCollection::~RimEnsembleCurveSetCollection()
{
    m_curveSets.deleteAllChildObjects();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimEnsembleCurveSetCollection::isCurveSetsVisible()
{
    return m_showCurves();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleCurveSetCollection::loadDataAndUpdate(bool updateParentPlot)
{
    for (RimEnsembleCurveSet* curveSet : m_curveSets)
    {
        curveSet->loadDataAndUpdate(updateParentPlot);
    }

    if (updateParentPlot)
    {
        RimSummaryPlot* parentPlot;
        firstAncestorOrThisOfTypeAsserted(parentPlot);
        if (parentPlot->qwtPlot())
        {
            parentPlot->updatePlotTitle();
            parentPlot->qwtPlot()->updateLegend();
            parentPlot->updateAxes();
            parentPlot->updateZoomInQwt();
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleCurveSetCollection::setParentQwtPlotAndReplot(QwtPlot* plot)
{
    for (RimEnsembleCurveSet* curveSet : m_curveSets)
    {
        curveSet->setParentQwtPlotNoReplot(plot);
    }

    if (plot) plot->replot();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleCurveSetCollection::detachQwtCurves()
{
    for (const auto& curveSet : m_curveSets)
    {
        curveSet->detachQwtCurves();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimEnsembleCurveSet* RimEnsembleCurveSetCollection::findRimCurveSetFromQwtCurve(const QwtPlotCurve* qwtCurve) const
{
    for (RimEnsembleCurveSet* curveSet : m_curveSets)
    {
        for (RimSummaryCurve* rimCurve : curveSet->curves())
        {
            if (rimCurve->qwtPlotCurve() == qwtCurve)
            {
                return curveSet;
            }
        }
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleCurveSetCollection::addCurveSet(RimEnsembleCurveSet* curveSet)
{
    static int nextAutoColorIndex = 1;
    static int numberOfColors     = (int)RiaColorTables::summaryCurveDefaultPaletteColors().size();

    if (curveSet)
    {
        curveSet->setColor(RimSummaryCurveAppearanceCalculator::cycledPaletteColor(nextAutoColorIndex));
        m_curveSets.push_back(curveSet);

        nextAutoColorIndex = (++nextAutoColorIndex) % numberOfColors;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleCurveSetCollection::deleteCurveSet(RimEnsembleCurveSet* curveSet)
{
    if (curveSet)
    {
        m_curveSets.removeChildObject(curveSet);
        delete curveSet;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimEnsembleCurveSet*> RimEnsembleCurveSetCollection::curveSets() const
{
    return m_curveSets.childObjects();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimEnsembleCurveSet*> RimEnsembleCurveSetCollection::visibleCurveSets() const
{
    std::vector<RimEnsembleCurveSet*> visible;

    if (m_showCurves())
    {
        for (const auto& c : m_curveSets)
        {
            if (c->isCurvesVisible())
            {
                visible.push_back(c);
            }
        }
    }

    return visible;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleCurveSetCollection::deleteAllCurveSets()
{
    m_curveSets.deleteAllChildObjects();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleCurveSetCollection::setCurrentSummaryCurveSet(RimEnsembleCurveSet* curveSet)
{
    m_currentEnsembleCurveSet = curveSet;

    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleCurveSetCollection::fieldChangedByUi(const caf::PdmFieldHandle* changedField,
                                                     const QVariant&            oldValue,
                                                     const QVariant&            newValue)
{
    if (changedField == &m_showCurves)
    {
        loadDataAndUpdate(true);

        RimSummaryPlot* summaryPlot = nullptr;
        this->firstAncestorOrThisOfTypeAsserted(summaryPlot);
        summaryPlot->updateConnectedEditors();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmFieldHandle* RimEnsembleCurveSetCollection::objectToggleField()
{
    return &m_showCurves;
}
