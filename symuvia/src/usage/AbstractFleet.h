#pragma once
#ifndef AbstractFleetH
#define AbstractFleetH

#include "tools.h"
#include "VehicleTypeUsageParameters.h"
#include "AbstractFleetParameters.h"

#include <boost/shared_ptr.hpp>

#include <map>
#include <vector>

class Vehicule;
class Trip;
class TripNode;
class TripLeg;
class Reseau;
class DocTrafic;
class VoieMicro;
class Tuyau;
class AbstractFleetParameters;
class Schedule;
class ScheduleElement;
class ScheduleParameters;
class RepartitionTypeVehicule;
class Logger;
class TraceDocTrafic;
class TripNodeSnapshot;
class TripSnapshot;
class VehicleToCreate;


class AbstractFleetSnapshot {

public:
    AbstractFleetSnapshot();
    virtual ~AbstractFleetSnapshot();

    std::vector<int>                            lstVehicles;        // Liste des véhicules associés à la flotte
    std::map<Trip*, int>                        mapScheduleState;   // Map des variables de travail des schedules
    std::map<TripNode*, TripNodeSnapshot*>      mapTripNodeState;   // Map des variables de travail des TripNodes
    std::map<Trip*, TripSnapshot*>              mapTripState;       // Map des variables de travail des trips

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


class AbstractFleet
{
public:

    // Constructeurs / Destructeurs
    AbstractFleet();
    AbstractFleet(Reseau * pNetwork);
    virtual ~AbstractFleet();

    // Accesseurs
    ///////////////////////////////////

    const std::vector<TripNode*> & GetTripNodes();
    const std::vector<Trip*> & GetTrips();
    Trip* GetTrip(const std::string & tripID);
    TripNode* GetTripNode(const std::string & tripNodeID);

    std::vector<boost::shared_ptr<Vehicule> > GetVehicles(Trip * pTrip);

    Schedule * GetSchedule(Trip * pTrip);

    // Chargement de la flotte à partir du scénario XML (ne fait rien par défaut : cas de la flotte SymuVia)
    virtual bool Load(XERCES_CPP_NAMESPACE::DOMNode * pXMLNetwork, Logger & loadingLogger) {return true;}

    // Méthodes de traitement
    ///////////////////////////////////

    // Instanciation de l'objet spécifique à la flotte contenant les paramètres d'un véhicule liés à celle-ci
    virtual AbstractFleetParameters * CreateFleetParameters() {return new AbstractFleetParameters();}

    // Initialisation de la flotte pour la simulation de trafic
    virtual void InitSimuTrafic(std::deque< TimeVariation<TraceDocTrafic> > & docTrafics);

    // Sorties spécifiques à la flotte dans les fichiers résultats
    virtual void SortieTrafic(DocTrafic *pXMLDocTrafic) {}

    // Traitements spécifiques à la flotte appelés lors du FinCalculTrafic
    virtual void FinCalculTrafic(Vehicule * pVeh) {}

    // Activation des véhicules pour le pas de temps courant en fonction du calendrier
    virtual void ActivateVehicles(double dbInstant, double dbTimeStep);

    // Activation d'un véhicule sur ordre : à spécialiser dans les différentes flottes
    virtual void ActivateVehicle(double dbInstant, VehicleToCreate * pVehicleToCreate);

    // Test de l'atteinte de la destination
    virtual bool IsCurrentLegDone(Vehicule * pVehicle, TripLeg * pCurrentLeg,
                                  double dbInstant, VoieMicro * pLane, double laneLength, double startPositionOnLane, double endPositionOnLane, bool bExactPosition);

    // Gestion de la terminaison d'une étape par un véhicule de la flotte.
    virtual void OnCurrentLegFinished(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep);

    // Met à jour le Trip en fonction des tuyaux parcourus
    virtual void SetLinkUsed(double dbInstant, Vehicule * pVeh, Tuyau * pLink);

    // Effectue les opérations à faire à l'activation d'un véhicule
    virtual void OnVehicleActivated(boost::shared_ptr<Vehicule> pVeh, double dbInstant);

    // Associe le véhicule à la flotte et vice versa
    virtual void DoVehicleAssociation(boost::shared_ptr<Vehicule> pVeh);

    // Crée et associé au véhicule les paramètres liés à la flotte
    virtual void AssignFleetparameters(boost::shared_ptr<Vehicule> pVeh);

    // Effectue les opérations à faire à la destruction d'un véhicule
    virtual void OnVehicleDestroyed(boost::shared_ptr<Vehicule> pVeh);

    // Construit le libellé du véhicule, utilisé par exemple pour les bus
    virtual std::string GetVehicleIdentifier(boost::shared_ptr<Vehicule> pVeh) {return std::string();}

    // Calcule la durée d'arrêt à un tripnode.
    virtual double GetStopDuration(boost::shared_ptr<Vehicule> pVehicle, TripNode * pTripNode, bool bIsRealStop);

    // Renvoie une map d'attributs à sortir de façon spécifique pour un véhicule dans le TripNode
    virtual std::map<std::string, std::string> GetOutputAttributes(Vehicule * pV, TripNode * pTripNode);

    // Renvoie une map d'attributs à sortir de façon spécifique pour un véhicule une fois la simulation terminée
    virtual std::map<std::string, std::string> GetOutputAttributesAtEnd(Vehicule * pV);

    // Accesseur vers les paramètres liés au calendrier
    virtual const std::map<ScheduleElement*, ScheduleParameters*> & GetScheduleParameters() {return m_MapScheduleParameters;}

    // Indique s'il faut sortir la charge courante du véhicule ou non
    virtual bool OutputVehicleLoad(Vehicule * pVehicle) {return false;}

    // méthode pour la sauvegarde et restitution de l'état des flottes (affectation dynamique convergente).
    // à spécialiser si des flottes utilisent des variables de simulation supplémentaires
    virtual AbstractFleetSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, AbstractFleetSnapshot * backup);

protected:

    // Charge les paramètres d'usage associés aux types de véhicules
    virtual bool LoadUsageParams(XERCES_CPP_NAMESPACE::DOMNode * pXMLFleetNode, Logger & loadingLogger);

    // Activation des véhicules pour le Trip passé en paramètres (si besoin)
    virtual std::vector<boost::shared_ptr<Vehicule> > ActivateVehiclesForTrip(double dbInstant, double dbTimeStep, Trip * pTrip);

    // Création d'un véhicule
    virtual boost::shared_ptr<Vehicule> CreateVehicle(double dbInstant, double dbTimeStep, Trip * pTrip, int nID, ScheduleElement * pScheduleElement);

    // Calcul l'itinéraire pour réaliser le trip, s'il n'est pas prédéfini
    virtual void ComputeTripPath(Trip * pTrip, TypeVehicule * pTypeVeh);

    // Ajoute un TripNode en tant que nouvelle destination intérmédiaire d'un Trip potentiellement réalisé par le véhicule pVehicle.
    virtual bool AddTripNodeToTrip(Trip * pTrip, TripNode * pTripNode, size_t tripLegIndex, Vehicule * pVehicle);

    // Supprime un TripLeg d'un Trip potentiellement réalisé par le véhicule pVehicle.
    virtual bool RemoveTripLegFromTrip(Trip * pTrip, size_t tripLegIndex, Vehicule * pVehicle);

protected:

    Reseau                                     *m_pNetwork;

    // Ensemble des TripNodes définis pour la flotte
    std::vector<TripNode*>                      m_LstTripNodes;

    // Ensemble des Trips définis pour la flotte
    std::vector<Trip*>                          m_LstTrips;

    // Schedules associés aux Trips définis pour la flotte
    std::map<Trip*, Schedule*>                  m_MapSchedules;

    // Répartition des types de véhicules pour la flotte
    std::map<Trip*, RepartitionTypeVehicule*>   m_MapRepartitionTypeVehicule;

    // Ensemble des véhicules associés à la flotte et présents sur le réseau.
    std::vector<boost::shared_ptr<Vehicule> >   m_LstVehicles;

    // Paramètres spécifiques à la flotte pour chaque type de véhicule
    std::map<TypeVehicule*, VehicleTypeUsageParameters> m_MapUsageParameters;

    // Paramètres liés à un élément de calendrier. Attention, ce champ n'a pas l'owership sur les objets ScheduleParameters (il faut les nettoyer
    // dans les spécialisations de la classe AbstractFleet (car il peut y avoir des ScheduleParameters non associés à un ScheduleElements).
    std::map<ScheduleElement*, ScheduleParameters*> m_MapScheduleParameters;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // AbstractFleetH