#pragma once

#ifndef SymuViaDrivingPattern_H
#define SymuViaDrivingPattern_H

#include <Graph/Model/Driving/DrivingPattern.h>

class Reseau;
class TypeVehicule;

class SymuViaDrivingPattern : public SymuCore::DrivingPattern
{
public:

    SymuViaDrivingPattern(SymuCore::Link* pLink, Reseau * pNetwork);

    virtual ~SymuViaDrivingPattern();

    virtual void prepareTimeFrame();
    virtual void fillCost(const std::map<TypeVehicule*, SymuCore::SubPopulation *> &listSubPopulations) = 0;

protected:

    Reseau * m_pNetwork;
};

#endif // SymuViaDrivingPattern_H


