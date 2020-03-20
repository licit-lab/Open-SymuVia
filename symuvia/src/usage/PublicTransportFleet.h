#pragma once
#ifndef PublicTransportFleetH
#define PublicTransportFleetH

#include "AbstractFleet.h"

class TypeVehicule;

class PublicTransportFleet : public AbstractFleet
{
public:
    PublicTransportFleet();
    PublicTransportFleet(Reseau * pNetwork);
    virtual ~PublicTransportFleet();

    // Instanciation de l'objet spécifique à la flotte contenant les paramètres d'un véhicule liés à celle-ci
    virtual AbstractFleetParameters * CreateFleetParameters();

    // récupération du type de véhicule associé à une ligne de bus
    TypeVehicule * GetTypeVehicule(Trip * pLine);

    virtual bool Load(XERCES_CPP_NAMESPACE::DOMNode * pXMLNetwork, Logger & loadingLogger);

    // Indique s'il faut sortir la charge courante du véhicule ou non
    virtual bool OutputVehicleLoad(Vehicule * pVehicle);

    // Construit le libellé du véhicule, utilisé par exemple pour les bus
    virtual std::string GetVehicleIdentifier(boost::shared_ptr<Vehicule> pVeh);

protected:
    // Paramétres des véhicules liés à un élément de calendrier
    std::map<Trip*, std::map<std::string, ScheduleParameters*> > m_MapScheduleParametersByTrip;

    // Indique si la durée des arrêts est dynamique ou non pour chaque ligne de bus
    std::map<Trip*, bool> m_MapDureeArretsDynamique;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


#endif // PublicTransportFleetH