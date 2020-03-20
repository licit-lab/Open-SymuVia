#include "PublicTransportPattern.h"
#include "PublicTransportLine.h"
#include "Graph/Model/Cost.h"
#include <boost/make_shared.hpp>

using namespace SymuCore;


PublicTransportPattern::PublicTransportPattern() : Pattern(), m_pLine(NULL)
{
    //default constructor
}

PublicTransportPattern::PublicTransportPattern(Link* pLink, PatternType ePatternType, PublicTransportLine* pLine)
    : Pattern(pLink, ePatternType)
{
    m_pLine = pLine;
}

PublicTransportPattern::~PublicTransportPattern()
{
    
}

PublicTransportLine* PublicTransportPattern::getLine() const
{
    return m_pLine;
}

const std::vector<ListTimeFrame<Cost> > & PublicTransportPattern::getTemporalCosts() const
{
    return m_TemporalCosts;
}

std::vector<ListTimeFrame<Cost> > & PublicTransportPattern::getTemporalCosts()
{
    return m_TemporalCosts;
}

void PublicTransportPattern::setLine(PublicTransportLine *pLine)
{
    m_pLine = pLine;
}

Cost * PublicTransportPattern::getPatternCost(int iSimulationInstance, double t, SubPopulation* pSubPopulation)
{
    return m_TemporalCosts[iSimulationInstance].getData(t);
}

void PublicTransportPattern::addTimeFrame(double tBeginTime, double tEndTime)
{
    m_TemporalCosts.resize(1);
    m_TemporalCosts[0].addTimeFrame(tBeginTime, tEndTime, boost::make_shared<Cost>());
}

void PublicTransportPattern::prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation *> &listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance)
{
    m_TemporalCosts.clear();
    m_TemporalCosts.resize(iInstance==0?nbSimulationInstances:1);

    int nbTravelTimesUpdatePeriodsByPeriod = (int)ceil((endPeriodTime - startPeriodTime) / travelTimesUpdatePeriod);
    double currentTime = startPeriodTime;
    for (int iTravelTimesUpdatePeriod = 0; iTravelTimesUpdatePeriod < nbTravelTimesUpdatePeriodsByPeriod; iTravelTimesUpdatePeriod++)
    {
        double startTravelTimesUpdatePeriodTime = currentTime;
        double endTravelTimesUpdatePeriodTime = std::min<double>(startTravelTimesUpdatePeriodTime + travelTimesUpdatePeriod, endPeriodTime);

        ListTimeFrame<Cost> & temporalCosts = m_TemporalCosts[0];
        temporalCosts.addTimeFrame(startTravelTimesUpdatePeriodTime, endTravelTimesUpdatePeriodTime, boost::make_shared<Cost>());
        fillMeasuredCostsForTravelTimesUpdatePeriod(iTravelTimesUpdatePeriod, listSubPopulations, mapCostFunctions);

        currentTime += travelTimesUpdatePeriod;
    }
}

std::string PublicTransportPattern::toString() const
{
    return m_pLine->getStrName();
}

void PublicTransportPattern::fillFromSecondaryInstance(Pattern * pFrom, int iInstance)
{
    PublicTransportPattern * pOriginalPattern = (PublicTransportPattern*)pFrom;
    m_TemporalCosts[iInstance] = std::move(pOriginalPattern->m_TemporalCosts.front());
}


