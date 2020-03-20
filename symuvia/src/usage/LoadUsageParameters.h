#pragma once
#ifndef LoadUsageParametersH
#define LoadUsageParametersH

#include "usage/Passenger.h"

#include <xercesc/util/XercesVersion.hpp>

#include <vector>
#include <map>

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
}

namespace boost {
    namespace serialization {
        class access;
    }
}

class Logger;
class TripNode;

class LoadUsageParameters
{

public:
    LoadUsageParameters();
    virtual ~LoadUsageParameters();

    void    Load(XERCES_CPP_NAMESPACE::DOMNode * pNode, Logger & loadingLogger);
    
    bool    IsMonteeDescenteSimultanee();
	void    SetMonteeDescenteSimultanee(bool bMonteeDescenteSimultanee);

    double  GetTempsMonteeIndividuel();
	void    SetTempsMonteeIndividuel(double dbTempsMontee);

    double  GetTempsDescenteIndividuel();
	void    SetTempsDescenteIndividuel(double dbTempsDescente);

    int     GetMaxLoad();
	void    SetMaxLoad(int iMaxLoad);

    int     GetCurrentLoad();
	void    SetCurrentLoad(int iCurrentLoad);

    double  ComputeLoadTime(int iLoadQuantity, int iUnloadQuantity);

    std::vector<Passenger> & GetPassengers(TripNode * pTripNode);

protected:

    // PARAMETRES D'ENTREE
    //////////////////////////////

    // true si le véhicule permet le chargement / déchargement simultané
    bool    m_bMonteeDescenteSimultanee;

    // durée (en secondes par unité de stock) de chargement/déchargement d'une unité de chargement (passager, marchandise, ...)
    double  m_dbTempsMonteeIndividuel;
    double  m_dbTempsDescenteIndividuel;

    // Charge maximum que le véhicule peut embarquer, sans unité (en passager, en marchandises, ...)
    int     m_MaxLoad;

    // VARIABLES DE CALCUL
    ///////////////////////////////
    int     m_CurrentLoad;

    // Passagers embarqués triés par destination
    std::map<TripNode* , std::vector<Passenger> > m_currentPassengers;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // LoadUsageParametersH
