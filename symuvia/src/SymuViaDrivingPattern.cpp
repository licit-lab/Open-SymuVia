#include "stdafx.h"
#include "SymuViaDrivingPattern.h"

#include "reseau.h"


using namespace SymuCore;

SymuViaDrivingPattern::SymuViaDrivingPattern(SymuCore::Link* pLink, Reseau * pNetwork)
: DrivingPattern(pLink)
{
    m_pNetwork = pNetwork;
}

SymuViaDrivingPattern::~SymuViaDrivingPattern()
{

}


void SymuViaDrivingPattern::prepareTimeFrame()
{
    getTemporalCosts().resize(1);
    getTemporalCosts()[0].addTimeFrame(0, m_pNetwork->GetDureeSimu(), boost::make_shared<std::map<SubPopulation*, Cost>>());
}
