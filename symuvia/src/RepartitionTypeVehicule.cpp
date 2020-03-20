#include "stdafx.h"
#include "RepartitionTypeVehicule.h"

#include "SerializeUtil.h"
#include "RandManager.h"
#include "reseau.h"

#include <boost/make_shared.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

RepartitionTypeVehicule::RepartitionTypeVehicule()
{
}


RepartitionTypeVehicule::~RepartitionTypeVehicule()
{
    Clear();
}

std::deque<TimeVariation<std::vector<std::vector<double> > > > & RepartitionTypeVehicule::GetLstCoefficients()
{
    return m_LstCoefficients;
}

void RepartitionTypeVehicule::AddVariation(const std::vector<std::vector<double> > & coeffs, PlageTemporelle * pPlage)
{
    boost::shared_ptr<std::vector<std::vector<double> > > coeffsPtr = boost::make_shared<std::vector<std::vector<double> > >(coeffs);
    ::AddVariation(pPlage, coeffsPtr, &m_LstCoefficients);
}

void RepartitionTypeVehicule::AddVariation(const std::vector<std::vector<double> > & coeffs, double dbDuree)
{
    boost::shared_ptr<std::vector<std::vector<double> > > coeffsPtr = boost::make_shared<std::vector<std::vector<double> > >(coeffs);
    ::AddVariation(dbDuree, coeffsPtr, &m_LstCoefficients);
}

void RepartitionTypeVehicule::Clear()
{
    EraseListOfVariation(&m_LstCoefficients);
}

void RepartitionTypeVehicule::ClearFrom(double dbInstant, double dbLag)
{
    EraseVariation(&m_LstCoefficients, dbInstant, dbLag);
}


//================================================================
double RepartitionTypeVehicule::GetRepartitionTypeVehicule
//----------------------------------------------------------------
// Fonction  : Retourne la liste ordonnée des coeff de la
//             répartition par voie à un instant donné
// Version du: 04/01/2007
// Historique: 04/01/2007 (C.Bécarie - Tinea)
//              Création
//================================================================
(
    double dbTime,          // Instant où on veut connaître la répartition
    double dbLag,           // Décalage temporel associé au réseau
    int nTypeVehicule,      // Type du véhicule
    int nVoie               // Numéro de la voie concernée
)
{            
    std::vector<std::vector<double> > * pCoefficients = GetVariation(dbTime, &m_LstCoefficients, dbLag);

    if(pCoefficients)
        return pCoefficients->at(nTypeVehicule)[nVoie];

    return 0;
}

//================================================================
    TypeVehicule*   RepartitionTypeVehicule::CalculTypeNewVehicule
//----------------------------------------------------------------
// Fonction  : Calcule par un processus aléatoire le type du véhicule à créer
// Version du: 08/01/2007
// Historique: 08/01/2007 (C.Bécarie - Tinea)
//================================================================
(
    Reseau * pNetwork,
    double  dbInstant,
    int     nVoie
)
{
    double  dbRand, dbSum;
    int nType;

    // Tirage
    dbRand = pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;

    // Protection contre les erreurs d'arrondi :
    TypeVehicule * pLastNonNullType = NULL;

    dbSum = 0;
    nType = 0;

    for (size_t iTypeVeh = 0; iTypeVeh < pNetwork->m_LstTypesVehicule.size(); iTypeVeh++)
    {
		double dbRepTypeVeh = GetRepartitionTypeVehicule(dbInstant, pNetwork->GetLag(), nType, nVoie);
		nType++;

		if( dbRepTypeVeh <= 0 )
			continue;

        pLastNonNullType = pNetwork->m_LstTypesVehicule[iTypeVeh];

        dbSum += dbRepTypeVeh;

        if( dbSum >= dbRand )
            return pLastNonNullType;
    }

    return pLastNonNullType;
}

bool RepartitionTypeVehicule::HasVehicleType(Reseau * pNetwork, int indexVehicleType, double dbStartTime, double dbEndTime)
{
    bool bHasTypeVeh = false;
    std::vector<std::pair<TimeVariation<std::vector<std::vector<double> > > , std::pair<double,double> > > timeVariations = GetVariations(dbStartTime, dbEndTime, &m_LstCoefficients, pNetwork->GetLag());
    for (size_t i = 0; i < timeVariations.size() && !bHasTypeVeh; i++)
    {
        const std::vector<double> & coeffs = timeVariations[i].first.m_pData->at(indexVehicleType);
        size_t nbVoies = coeffs.size();
        for(size_t iVoie = 0; iVoie < nbVoies && !bHasTypeVeh; iVoie++)
        {
            if(coeffs[iVoie] > 0)
            {
                bHasTypeVeh = true;
            }
        }
    }
    return bHasTypeVeh;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void RepartitionTypeVehicule::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void RepartitionTypeVehicule::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void RepartitionTypeVehicule::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_LstCoefficients);
}