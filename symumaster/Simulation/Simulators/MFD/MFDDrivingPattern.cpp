#include "MFDDrivingPattern.h"

#include "MFDHandler.h"

#include <boost/log/trivial.hpp>

using namespace SymuCore;
using namespace SymuMaster;


MFDDrivingPattern::MFDDrivingPattern(Link* pLink, int routeID, MFDHandler * pMFDHandler)
    : DrivingPattern(pLink),
    m_iRouteID(routeID),
    m_pMFDHandler(pMFDHandler)
{

}

MFDDrivingPattern::~MFDDrivingPattern()
{

}

int MFDDrivingPattern::getRouteID() const
{
    return m_iRouteID;
}

std::string MFDDrivingPattern::toString() const
{
    return std::to_string(m_iRouteID);
}

void MFDDrivingPattern::fillMeasuredCostsForTravelTimesUpdatePeriod(int iTravelTimesUpdatePeriodIndex,
    const std::vector<SubPopulation*>& listSubPopulations,
    const std::map<SymuCore::SubPopulation*,
    SymuCore::CostFunction> & mapCostFunctions)
{
    const std::map<CostFunction, Cost> & routeCost = m_pMFDHandler->GetMapCost(0).at(m_iRouteID);

    const TimeFrame<std::map<SubPopulation*, Cost>> & costVariation = getTemporalCosts().at(0).getTimeFrame((size_t)iTravelTimesUpdatePeriodIndex);

    for (size_t iSubPop = 0; iSubPop < listSubPopulations.size(); iSubPop++)
    {
        SubPopulation * pSubPop = listSubPopulations.at(iSubPop);

        CostFunction costFunc = mapCostFunctions.at(pSubPop);
        costVariation.getData()->operator[](pSubPop) = routeCost.at(costFunc);
    }
}