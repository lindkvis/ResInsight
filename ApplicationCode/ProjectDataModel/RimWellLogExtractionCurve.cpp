/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2015-     Statoil ASA
//  Copyright (C) 2015-     Ceetron Solutions AS
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

#include "RimWellLogExtractionCurve.h"

#include "RiaApplication.h"
#include "RiaSimWellBranchTools.h"

#include "RigCaseCellResultsData.h"
#include "RigEclipseCaseData.h"
#include "RigEclipseWellLogExtractor.h"
#include "RigFemPartResultsCollection.h"
#include "RigGeoMechCaseData.h"
#include "RigGeoMechWellLogExtractor.h"
#include "RigResultAccessorFactory.h"
#include "RigSimulationWellCenterLineCalculator.h"
#include "RigSimulationWellCoordsAndMD.h"
#include "RigSimWellData.h"
#include "RigWellLogCurveData.h"
#include "RigWellPath.h"

#include "RimEclipseCase.h"
#include "RimEclipseCellColors.h"
#include "RimEclipseResultDefinition.h"
#include "RimEclipseView.h"
#include "RimGeoMechCase.h"
#include "RimGeoMechResultDefinition.h"
#include "RimGeoMechView.h"
#include "RimMainPlotCollection.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimTools.h"
#include "RimWellLogCurve.h"
#include "RimWellLogPlot.h"
#include "RimWellLogPlotCollection.h"
#include "RimWellLogTrack.h"
#include "RimWellPath.h"
#include "RimWellPathCollection.h"

#include "RiuLineSegmentQwtPlotCurve.h"
#include "RiuWellLogTrack.h"

#include "cafPdmUiTreeOrdering.h"
#include "cafUtils.h"

#include <cmath>

//==================================================================================================
///  
///  
//==================================================================================================

CAF_PDM_SOURCE_INIT(RimWellLogExtractionCurve, "RimWellLogExtractionCurve");


namespace caf
{
template<>
void AppEnum< RimWellLogExtractionCurve::TrajectoryType >::setUp()
{
    addItem(RimWellLogExtractionCurve::WELL_PATH,       "WELL_PATH",        "Well Path");
    addItem(RimWellLogExtractionCurve::SIMULATION_WELL, "SIMULATION_WELL",  "Simulation Well");
    setDefault(RimWellLogExtractionCurve::WELL_PATH);
}
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellLogExtractionCurve::RimWellLogExtractionCurve()
{
    CAF_PDM_InitObject("Well Log Curve", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_trajectoryType, "TrajectoryType", "Trajectory", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_wellPath, "CurveWellPath", " ", "", "", "");
    m_wellPath.uiCapability()->setUiTreeChildrenHidden(true);

    CAF_PDM_InitField(&m_simWellName, "SimulationWellName", QString("None"), " ", "", "", "");
    CAF_PDM_InitField(&m_branchDetection, "BranchDetection", true, "Branch Detection", "", 
                      "Compute branches based on how simulation well cells are organized", "");
    CAF_PDM_InitField(&m_branchIndex,  "Branch", 0, "Branch Index", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_case, "CurveCase", "Case", "", "", "");
    m_case.uiCapability()->setUiTreeChildrenHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_eclipseResultDefinition, "CurveEclipseResult", "", "", "", "");
    m_eclipseResultDefinition.uiCapability()->setUiHidden(true);
    m_eclipseResultDefinition.uiCapability()->setUiTreeChildrenHidden(true);
    m_eclipseResultDefinition = new RimEclipseResultDefinition;
    m_eclipseResultDefinition->findField("MResultType")->uiCapability()->setUiName("Result Type");

    CAF_PDM_InitFieldNoDefault(&m_geomResultDefinition, "CurveGeomechResult", "", "", "", "");
    m_geomResultDefinition.uiCapability()->setUiHidden(true);
    m_geomResultDefinition.uiCapability()->setUiTreeChildrenHidden(true);
    m_geomResultDefinition = new RimGeoMechResultDefinition;

    CAF_PDM_InitField(&m_timeStep, "CurveTimeStep", 0,"Time Step", "", "", "");

    // Add some space before name to indicate these belong to the Auto Name field
    CAF_PDM_InitField(&m_addCaseNameToCurveName, "AddCaseNameToCurveName", true, "   Case Name", "", "", "");
    CAF_PDM_InitField(&m_addPropertyToCurveName, "AddPropertyToCurveName", true, "   Property", "", "", "");
    CAF_PDM_InitField(&m_addWellNameToCurveName, "AddWellNameToCurveName", true, "   Well Name", "", "", "");
    CAF_PDM_InitField(&m_addTimestepToCurveName, "AddTimestepToCurveName", false, "   Timestep", "", "", "");
    CAF_PDM_InitField(&m_addDateToCurveName, "AddDateToCurveName", true, "   Date", "", "", "");
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellLogExtractionCurve::~RimWellLogExtractionCurve()
{
    clearGeneratedSimWellPaths();

    delete m_geomResultDefinition;
    delete m_eclipseResultDefinition;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setWellPath(RimWellPath* wellPath)
{
    m_wellPath = wellPath;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPath* RimWellLogExtractionCurve::wellPath() const
{
    return m_wellPath;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setFromSimulationWellName(const QString& simWellName, int branchIndex, bool branchDetection)
{
    m_trajectoryType = SIMULATION_WELL;
    m_simWellName = simWellName;
    m_branchIndex = branchIndex;
    m_branchDetection = branchDetection;

    clearGeneratedSimWellPaths();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setCase(RimCase* rimCase)
{
    m_case = rimCase;
    clearGeneratedSimWellPaths();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimCase* RimWellLogExtractionCurve::rimCase() const
{
    return m_case;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setPropertiesFromView(Rim3dView* view)
{
    m_case = view ? view->ownerCase() : nullptr;

    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());
    m_eclipseResultDefinition->setEclipseCase(eclipseCase);
    m_geomResultDefinition->setGeoMechCase(geomCase);

    RimEclipseView* eclipseView = dynamic_cast<RimEclipseView*>(view);
    if (eclipseView)
    {
        m_eclipseResultDefinition->simpleCopy(eclipseView->cellResult());

        m_timeStep = eclipseView->currentTimeStep();
    }

    RimGeoMechView* geoMechView = dynamic_cast<RimGeoMechView*>(view);
    if (geoMechView)
    {
        m_geomResultDefinition->setResultAddress(geoMechView->cellResultResultDefinition()->resultAddress());
        m_timeStep = geoMechView->currentTimeStep();
    }

    clearGeneratedSimWellPaths();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::clampTimestep()
{
    if (m_timeStep > 0 && m_case)
    {
        if (m_timeStep > m_case->timeStepStrings().size() - 1)
        {
            m_timeStep = m_case->timeStepStrings().size() - 1;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::clampBranchIndex()
{
    int branchCount = static_cast<int>(simulationWellBranches().size());
    if ( branchCount > 0 )
    {
        if      ( m_branchIndex >= branchCount ) m_branchIndex = branchCount - 1;
        else if ( m_branchIndex < 0 )           m_branchIndex = 0;
    }
    else
    {
        m_branchIndex = -1;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{
    RimWellLogCurve::fieldChangedByUi(changedField, oldValue, newValue);

    if (changedField == &m_case)
    {
        clampTimestep();
        
        auto wellNameSet = findSortedWellNames();
        if (!wellNameSet.count(m_simWellName())) m_simWellName = "None";

        clearGeneratedSimWellPaths();

        this->loadDataAndUpdate(true);
    }    
    else if (changedField == &m_wellPath)
    {
        this->loadDataAndUpdate(true);
    }
    else if (changedField == &m_simWellName)
    {
        clearGeneratedSimWellPaths();

        this->loadDataAndUpdate(true);
    }
    else if (changedField == &m_trajectoryType)
    {
        this->loadDataAndUpdate(true);
    }
    else if (changedField == &m_branchDetection ||
             changedField == &m_branchIndex ||
             changedField == &m_branchIndex)
    {
        clearGeneratedSimWellPaths();

        this->loadDataAndUpdate(true);
    }
    else if (changedField == &m_timeStep)
    {
        this->loadDataAndUpdate(true);
    }

    if (changedField == &m_addCaseNameToCurveName ||
        changedField == &m_addPropertyToCurveName ||
        changedField == &m_addWellNameToCurveName ||
        changedField == &m_addTimestepToCurveName ||
        changedField == &m_addDateToCurveName)
    {
        this->uiCapability()->updateConnectedEditors();
        updateCurveNameAndUpdatePlotLegend();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::onLoadDataAndUpdate(bool updateParentPlot)
{
    this->RimPlotCurve::updateCurvePresentation(updateParentPlot);

    if (isCurveVisible())
    {
        // Make sure we have set correct case data into the result definitions.
        bool isUsingPseudoLength = false;

        RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
        RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());
        m_eclipseResultDefinition->setEclipseCase(eclipseCase);
        m_geomResultDefinition->setGeoMechCase(geomCase);

        clampBranchIndex();

        RimMainPlotCollection* mainPlotCollection;
        this->firstAncestorOrThisOfTypeAsserted(mainPlotCollection);

        RimWellLogPlotCollection* wellLogCollection = mainPlotCollection->wellLogPlotCollection();

        cvf::ref<RigEclipseWellLogExtractor> eclExtractor;

        if (eclipseCase)
        {
            if (m_trajectoryType == WELL_PATH)
            {
                eclExtractor = wellLogCollection->findOrCreateExtractor(m_wellPath, eclipseCase);
            }
            else
            {
                std::vector<const RigWellPath*> simWellBranches = simulationWellBranches();
                if (m_branchIndex >= 0 && m_branchIndex < static_cast<int>(simWellBranches.size()))
                {
                    auto wellBranch = simWellBranches[m_branchIndex];
                    eclExtractor = wellLogCollection->findOrCreateSimWellExtractor(m_simWellName,
                                                                                   eclipseCase->caseUserDescription(),
                                                                                   wellBranch,
                                                                                   eclipseCase->eclipseCaseData());
                    if (eclExtractor.notNull())
                    {
                        m_wellPathsWithExtractors.push_back(wellBranch);
                    }

                    isUsingPseudoLength = true;
                }
            }
        }
        cvf::ref<RigGeoMechWellLogExtractor> geomExtractor = wellLogCollection->findOrCreateExtractor(m_wellPath, geomCase);

        std::vector<double> values;
        std::vector<double> measuredDepthValues;
        std::vector<double> tvDepthValues;

        RiaDefines::DepthUnitType depthUnit = RiaDefines::UNIT_METER;

        if (eclExtractor.notNull() && eclipseCase)
        {
            measuredDepthValues = eclExtractor->measuredDepth();
            tvDepthValues = eclExtractor->trueVerticalDepth();

            m_eclipseResultDefinition->loadResult();

            cvf::ref<RigResultAccessor> resAcc = RigResultAccessorFactory::createFromResultDefinition(eclipseCase->eclipseCaseData(),
                                                                                                      0,
                                                                                                      m_timeStep,
                                                                                                      m_eclipseResultDefinition);

            if (resAcc.notNull())
            {
                eclExtractor->curveData(resAcc.p(), &values);
            }

            RiaEclipseUnitTools::UnitSystem eclipseUnitsType = eclipseCase->eclipseCaseData()->unitsType();
            if (eclipseUnitsType == RiaEclipseUnitTools::UNITS_FIELD)
            {
                // See https://github.com/OPM/ResInsight/issues/538
                
                depthUnit = RiaDefines::UNIT_FEET;
            }
        }
        else if (geomExtractor.notNull()) // geomExtractor
        {

            measuredDepthValues =  geomExtractor->measuredDepth();
            tvDepthValues = geomExtractor->trueVerticalDepth();

            m_geomResultDefinition->loadResult();

            geomExtractor->curveData(m_geomResultDefinition->resultAddress(), m_timeStep, &values);
        }

        m_curveData = new RigWellLogCurveData;
        if (values.size() && measuredDepthValues.size())
        {
            if (!tvDepthValues.size())
            {
                m_curveData->setValuesAndMD(values, measuredDepthValues, depthUnit, true);
            }
            else
            {
                m_curveData->setValuesWithTVD(values, measuredDepthValues, tvDepthValues, depthUnit, true);
            }
        }

        RiaDefines::DepthUnitType displayUnit = RiaDefines::UNIT_METER;

        RimWellLogPlot* wellLogPlot;
        firstAncestorOrThisOfType(wellLogPlot);
        CVF_ASSERT(wellLogPlot);
        if (!wellLogPlot) return;

        displayUnit = wellLogPlot->depthUnit();

        if(wellLogPlot->depthType() == RimWellLogPlot::TRUE_VERTICAL_DEPTH)
        {
            m_qwtPlotCurve->setSamples(m_curveData->xPlotValues().data(), m_curveData->trueDepthPlotValues(displayUnit).data(), static_cast<int>(m_curveData->xPlotValues().size()));
            isUsingPseudoLength = false;
        }
        else if (wellLogPlot->depthType() == RimWellLogPlot::MEASURED_DEPTH)
        {
            m_qwtPlotCurve->setSamples(m_curveData->xPlotValues().data(), m_curveData->measuredDepthPlotValues(displayUnit).data(), static_cast<int>(m_curveData->xPlotValues().size()));
        }

        m_qwtPlotCurve->setLineSegmentStartStopIndices(m_curveData->polylineStartStopIndices());

        if (isUsingPseudoLength)
        {
            RimWellLogTrack* wellLogTrack;
            firstAncestorOrThisOfType(wellLogTrack);
            CVF_ASSERT(wellLogTrack);

            RiuWellLogTrack* viewer = wellLogTrack->viewer();
            if (viewer)
            {
                viewer->setDepthTitle("PL/" + wellLogPlot->depthPlotTitle());
            }
        }

        updateZoomInParentPlot();

        setLogScaleFromSelectedResult();

        if (m_parentQwtPlot) m_parentQwtPlot->replot();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::set<QString> RimWellLogExtractionCurve::findSortedWellNames()
{
    std::set<QString> sortedWellNames;
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());

    if ( eclipseCase && eclipseCase->eclipseCaseData() )
    {
        const cvf::Collection<RigSimWellData>& simWellData = eclipseCase->eclipseCaseData()->wellResults();

        for ( size_t wIdx = 0; wIdx < simWellData.size(); ++wIdx )
        {
            sortedWellNames.insert(simWellData[wIdx]->m_wellName);
        }
    }

    return sortedWellNames;
}

//--------------------------------------------------------------------------------------------------
/// Clean up existing generated well paths 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::clearGeneratedSimWellPaths()
{
    RimWellLogPlotCollection* wellLogCollection = nullptr;

    // Need to use this approach, and not firstAnchestor because the curve might not be inside the hierarchy when deleted.

    RimProject * proj = RiaApplication::instance()->project();
    if (proj && proj->mainPlotCollection() ) wellLogCollection = proj->mainPlotCollection()->wellLogPlotCollection();
    
    if (!wellLogCollection) return;

    for (auto wellPath : m_wellPathsWithExtractors)
    {
        wellLogCollection->removeExtractors(wellPath);
    }

    m_wellPathsWithExtractors.clear();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<const RigWellPath*> RimWellLogExtractionCurve::simulationWellBranches() const
{
    return RiaSimWellBranchTools::simulationWellBranches(m_simWellName, m_branchDetection);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimWellLogExtractionCurve::calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool * useOptionsOnly)
{
   QList<caf::PdmOptionItemInfo> options;

   options = RimWellLogCurve::calculateValueOptions(fieldNeedingOptions, useOptionsOnly);
   if (options.size() > 0) return options;

    if (fieldNeedingOptions == &m_wellPath)
    {
        RimTools::wellPathOptionItems(&options);

        options.push_front(caf::PdmOptionItemInfo("None", nullptr));
    }
    else if (fieldNeedingOptions == &m_case)
    {
        RimTools::caseOptionItems(&options);

        options.push_front(caf::PdmOptionItemInfo("None", nullptr));
    }
    else if (fieldNeedingOptions == &m_timeStep)
    {
        QStringList timeStepNames;

        if (m_case)
        {
            timeStepNames = m_case->timeStepStrings();
        }

        for (int i = 0; i < timeStepNames.size(); i++)
        {
            options.push_back(caf::PdmOptionItemInfo(timeStepNames[i], i));
        }
    }
    else if (fieldNeedingOptions == &m_simWellName)
    {
        std::set<QString> sortedWellNames = this->findSortedWellNames();

        QIcon simWellIcon(":/Well.png");
        for ( const QString& wname: sortedWellNames )
        {
            options.push_back(caf::PdmOptionItemInfo(wname, wname, false, simWellIcon));
        }

        if ( options.size() == 0 )
        {
            options.push_front(caf::PdmOptionItemInfo("None", "None"));
        }
    }
    else if (fieldNeedingOptions == &m_branchIndex)
    {
        auto branches = simulationWellBranches();

        options = RiaSimWellBranchTools::valueOptionsForBranchIndexField(branches);
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    RimPlotCurve::updateOptionSensitivity();

    caf::PdmUiGroup* curveDataGroup = uiOrdering.addNewGroup("Curve Data");

    curveDataGroup->add(&m_case);
    
    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());

    curveDataGroup->add(&m_trajectoryType);
    if (m_trajectoryType() == WELL_PATH)
    {
        curveDataGroup->add(&m_wellPath);
    }
    else 
    {
        curveDataGroup->add(&m_simWellName);

        RiaSimWellBranchTools::appendSimWellBranchFieldsIfRequiredFromSimWellName(curveDataGroup,
                                                                                  m_simWellName,
                                                                                  m_branchDetection,
                                                                                  m_branchIndex);
    }

    if (eclipseCase)
    {
        m_eclipseResultDefinition->uiOrdering(uiConfigName, *curveDataGroup);

    }
    else if (geomCase)
    {
        m_geomResultDefinition->uiOrdering(uiConfigName, *curveDataGroup);
  
    }

    if (   (eclipseCase && m_eclipseResultDefinition->hasDynamicResult())
        ||  geomCase)
    {
        curveDataGroup->add(&m_timeStep);
    }

    caf::PdmUiGroup* appearanceGroup = uiOrdering.addNewGroup("Appearance");
    RimPlotCurve::appearanceUiOrdering(*appearanceGroup);

    caf::PdmUiGroup* nameGroup = uiOrdering.addNewGroup("Curve Name");
    nameGroup->setCollapsedByDefault(true);
    nameGroup->add(&m_showLegend);
    RimPlotCurve::curveNameUiOrdering(*nameGroup);

    if (m_isUsingAutoName)
    {
        nameGroup->add(&m_addWellNameToCurveName);
        nameGroup->add(&m_addCaseNameToCurveName);
        nameGroup->add(&m_addPropertyToCurveName);
        nameGroup->add(&m_addDateToCurveName);
        nameGroup->add(&m_addTimestepToCurveName);
    }


    uiOrdering.skipRemainingFields(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::initAfterRead()
{
    RimWellLogCurve::initAfterRead();

    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());

    m_eclipseResultDefinition->setEclipseCase(eclipseCase);
    m_geomResultDefinition->setGeoMechCase(geomCase);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::defineUiTreeOrdering(caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName /*= ""*/)
{
    uiTreeOrdering.skipRemainingChildren(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setLogScaleFromSelectedResult()
{
    QString resVar = m_eclipseResultDefinition->resultVariable();

    if (resVar.toUpper().contains("PERM"))
    {
        RimWellLogTrack* track = nullptr;
        this->firstAncestorOrThisOfType(track);
        if (track)
        {
            if (track->curveCount() == 1)
            {
                track->setLogarithmicScale(true);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellLogExtractionCurve::createCurveAutoName()
{
    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());
    
    QStringList generatedCurveName;

    if (m_addWellNameToCurveName)
    {
        if (!wellName().isEmpty())
        {
            generatedCurveName += wellName();
            if (m_trajectoryType == SIMULATION_WELL && simulationWellBranches().size() > 1)
            {
                generatedCurveName.push_back(" Br" + QString::number(m_branchIndex + 1));
            }
        }
    }

    if (m_addCaseNameToCurveName && m_case())
    {
        generatedCurveName.push_back(m_case->caseUserDescription());
    }

    if (m_addPropertyToCurveName && !wellLogChannelName().isEmpty())
    {
        generatedCurveName.push_back(wellLogChannelName());
    }

    if (m_addTimestepToCurveName || m_addDateToCurveName)
    {
        size_t maxTimeStep = 0;

        if (eclipseCase)
        {
            if (eclipseCase->eclipseCaseData())
            {
                maxTimeStep = eclipseCase->eclipseCaseData()->results(m_eclipseResultDefinition->porosityModel())->maxTimeStepCount();
            }
        }
        else if (geomCase)
        {
            if (geomCase->geoMechData())
            {
                maxTimeStep = geomCase->geoMechData()->femPartResults()->frameCount();
            }
        }

        if (m_addDateToCurveName)
        {
            QString dateString = wellDate();
            if (!dateString.isEmpty())
            {
                generatedCurveName.push_back(dateString);
            }
        }

        if (m_addTimestepToCurveName)
        {
            generatedCurveName.push_back(QString("[%1/%2]").arg(m_timeStep()).arg(maxTimeStep));
        }
    }

    return generatedCurveName.join(", ");
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellLogExtractionCurve::wellLogChannelName() const
{
    RimGeoMechCase* geoMechCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());

    QString name;
    if (eclipseCase)
    {
        name = caf::Utils::makeValidFileBasename( m_eclipseResultDefinition->resultVariableUiName());
    }
    else if (geoMechCase)
    {
        QString resCompName = m_geomResultDefinition->resultComponentUiName();
        if (resCompName.isEmpty())
        {
            name = m_geomResultDefinition->resultFieldUiName();
        }
        else
        {
            name = m_geomResultDefinition->resultFieldUiName() + "." + resCompName;
        }
    }

    return name;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellLogExtractionCurve::wellName() const
{
    if ( m_trajectoryType() == WELL_PATH )
    {
        if ( m_wellPath )
        {
            return m_wellPath->name();
        }
        else
        {
            return QString();
        }
    }
    else
    {
        return m_simWellName;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellLogExtractionCurve::wellDate() const
{
    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());

    QStringList timeStepNames;

    if (eclipseCase)
    {
        if (eclipseCase->eclipseCaseData())
        {
            timeStepNames = eclipseCase->timeStepStrings();
        }
    }
    else if (geomCase)
    {
        if (geomCase->geoMechData())
        {
            timeStepNames = geomCase->timeStepStrings();
        }
    }

    return (m_timeStep >= 0 && m_timeStep < timeStepNames.size()) ? timeStepNames[m_timeStep] : "";
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimWellLogExtractionCurve::isEclipseCurve() const
{
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());
    if (eclipseCase)
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellLogExtractionCurve::caseName() const
{
    if (m_case)
    {
        return m_case->caseUserDescription();
    }

    return QString();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
double RimWellLogExtractionCurve::rkbDiff() const
{
    if (m_wellPath && m_wellPath->wellPathGeometry())
    {
        RigWellPath* geo = m_wellPath->wellPathGeometry();

        if (geo->hasDatumElevation())
        {
            return geo->datumElevation();
        }

        // If measured depth is zero, use the z-value of the well path points
        if (geo->m_wellPathPoints.size() > 0 && geo->m_measuredDepths.size() > 0)
        {
            double epsilon = 1e-3;

            if (cvf::Math::abs(geo->m_measuredDepths[0]) < epsilon)
            {
                double diff = geo->m_measuredDepths[0] - (-geo->m_wellPathPoints[0].z());

                return diff;
            }
        }
    }

    return HUGE_VAL;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
int RimWellLogExtractionCurve::currentTimeStep() const
{
    return m_timeStep();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setCurrentTimeStep(int timeStep)
{
    m_timeStep = timeStep;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setEclipseResultVariable(const QString& resVarname)
{
    m_eclipseResultDefinition->setResultVariable(resVarname);
}
