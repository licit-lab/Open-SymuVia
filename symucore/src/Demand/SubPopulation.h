#pragma once

#ifndef SYMUCORE_SubPopulation_H
#define SYMUCORE_SubPopulation_H

#include "SymuCoreExports.h"

#include "Utils/SymuCoreConstants.h"

#include <vector>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class Trip;
class Motive;
class Population;

class SYMUCORE_DLL_DEF SubPopulation{

public:

    SubPopulation();
    SubPopulation(const std::string& strName);
    virtual ~SubPopulation();

    const std::vector<Trip *> & GetListUsers() const;
    std::vector<Trip *> & GetListUsers();
    std::string GetStrName() const;
    Population *GetPopulation() const;
    Motive* GetMotive() const;
    double GetInitialWalkSpeed() const;
    double GetInitialWalkRadius() const;

    void SetListUsers(const std::vector<Trip *> &listUsers);
    void SetPopulation(Population *pPopulation);
    void SetStrName(const std::string &strName);
    void SetMotive(Motive *Motive);
    void SetInitialWalkSpeed(double dbWalkSpeed);
    void SetInitialWalkRadius(double dbWalkRadius);



protected:
    std::string                    m_strName; // name of this SubPopulation
    Population*                    m_pPopulation; //Population that own this SubPopulation
    std::vector<Trip*>             m_listUsers; //list of Users for this SubPopulation
    Motive*                        m_pMotive; //Motive for the SubPopulation
    double                         m_dbInitialWalkSpeed; //Initial walking speed (default defined in Population)
    double                         m_dbInitialWalkRadius;//Initial walk radius (default defined in Population)
};
}

#pragma warning(pop)

#endif // SYMUCORE_SubPopulation_H
