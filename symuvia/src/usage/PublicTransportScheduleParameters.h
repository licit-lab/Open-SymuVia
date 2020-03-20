#pragma once
#ifndef PublicTransportScheduleParametersH
#define PublicTransportScheduleParametersH

#include "ScheduleParameters.h"

#include <vector>
#include <map>

class Arret;
class Trip;

class PublicTransportScheduleParameters : public ScheduleParameters
{

public:
    PublicTransportScheduleParameters();
    virtual ~PublicTransportScheduleParameters();

    // Chargement de du jeu de paramètres à partir du noeud XML correspondant
    virtual bool Load(XERCES_CPP_NAMESPACE::DOMNode * pXMLNode, Logger & loadingLogger);

    // Association des durées d'arrêts et des arrêts
    bool AssignStopTimesToStops(Trip * pLigne, Logger & loadingLogger);

    // Accesseur aux durées d'arrêt définies
    const std::map<Arret*, double> & GetStopTimes();

protected:

    // Liste des durées d'arrêts telles que définie dans le noeud XML
    std::vector<double> m_StopTimes;

    // Association entre les arrêts et les durées d'arrêts définies
    std::map<Arret*, double> m_mapStopTimes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // PublicTransportScheduleParametersH
