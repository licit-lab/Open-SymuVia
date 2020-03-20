#pragma once

#ifndef SYMUCORE_PUBLICTRANSPORTPATTERN_H
#define SYMUCORE_PUBLICTRANSPORTPATTERN_H

#include "SymuCoreExports.h"

#include "Graph/Model/Pattern.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class PublicTransportLine;
class SYMUCORE_DLL_DEF PublicTransportPattern : public Pattern {

public:

    PublicTransportPattern();
    PublicTransportPattern(Link* pLink, PatternType ePatternType, PublicTransportLine* pLine);
    virtual ~PublicTransportPattern();

    //getters
    PublicTransportLine* getLine() const;
    const std::vector<ListTimeFrame<Cost> > & getTemporalCosts() const;
    std::vector<ListTimeFrame<Cost> > & getTemporalCosts();


    //setters
    void setLine(PublicTransportLine* pLine);

    virtual void addTimeFrame(double tBeginTime, double tEndTime);
    virtual void prepareTimeFrames(double startPeriodTime, double endPeriodTime, double travelTimesUpdatePeriod, const std::vector<SubPopulation*>& listSubPopulations, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions, int nbSimulationInstances, int iInstance);
    virtual Cost* getPatternCost(int iSimulationInstance, double t, SubPopulation* pSubPopulation);
    virtual std::string toString() const;

    virtual void fillFromSecondaryInstance(Pattern * pFrom, int iInstance);

protected:

    PublicTransportLine*                m_pLine; //the public transport line use for this pattern
    std::vector<ListTimeFrame<Cost> >   m_TemporalCosts; //cost to use this pattern for each simulation instance, depends on time

};
}

#pragma warning(pop)

#endif // SYMUCORE_PUBLICTRANSPORTPATTERN_H
