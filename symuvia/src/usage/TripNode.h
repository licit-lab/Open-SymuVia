#pragma once
#ifndef TripNodeH
#define TripNodeH

#include "Position.h"
#include "tools.h"

class Reseau;
class TypeVehicule;
class Vehicule;
class VoieMicro;
class TraceDocTrafic;
class Trip;

#include <string>
#include <vector>

class TripNodeSnapshot {

public:
    TripNodeSnapshot();
    virtual ~TripNodeSnapshot();

    std::vector<int>                            lstStoppedVehicles; // Liste des véhicules dans le TripNode

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

class TripNode
{
public:
    TripNode();
    TripNode(const std::string & strID, Reseau * pNetwork);
    virtual ~TripNode();

    // positionnement du TripNode (point d'entrée et de sortie)
    virtual void SetInputPosition(const Position & pos);
    virtual void SetOutputPosition(const Position & pos);

    virtual const Position & GetInputPosition() const;
    virtual const Position & GetOutputPosition() const;

    void SetStopNeeded(bool bStopNeeded);
    void SetListVehiclesInTripNode(bool bListVehicles);

	bool IsStopNeeded() { return m_bStopNeeded; }

    Reseau * GetNetwork() { return m_pNetwork; }

    // Identifiant de l'entrée dans le TripNode
    virtual const std::string & GetInputID() const {return GetID();}

    // Identifiant de la sortie du TripNode
    virtual std::string GetOutputID() const {return GetID();}

    // Identifiant du TripNode
    virtual const std::string & GetID() const {return m_strID;}

    // Initialisation du TripNode pour la simulation de trafic
    virtual void Initialize(std::deque<TimeVariation<TraceDocTrafic> > & docTrafics) {}

    // Indique si l'atteinte du TripNode implique l'arrêt du véhicule
    virtual bool ContinueDirectlyAfterDestination();

    // Indique si le type de véhicule est autorisé pour cette destination
    virtual bool IsAllowedVehiculeType(TypeVehicule * pTypeVehicule) {return true;}

    // Indique si le TripNode est plein (cas d'un parking par exemple)
    virtual bool IsFull() {return false;}

    // Par défaut, pas de restriction ou de temps intervéhiculaire à l'entrée dans un tripnode (à spécialiser pour les sorties ou parkings par exemple)
    virtual double GetNextEnterInstant(int nNbVoie, double  dbPrevInstSortie, double  dbInstant, double  dbPasDeTemps, const std::string & nextRouteID) { return dbInstant; }

    // Traitement sur entrée d'un véhicule dans le TripNode
    virtual void VehiculeEnter(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep, bool bForcedOutside = false);

    // Traitement sur sortie d'un véhicule du TripNode
    virtual void VehiculeExit(boost::shared_ptr<Vehicule> pVehicle);

    // Test de l'atteinte de la destination
    virtual bool IsReached(VoieMicro * pLane, double laneLength, double startPositionOnLane, double endPositionOnLane, bool bExactPosition);

    // Teste si le véhicule est en cours d'arrêt dans le TripNode
    virtual bool HasVehicle(boost::shared_ptr<Vehicule> pVeh);

    // Gestion de l'arrêt d'un véhicule dans le TripNode
    virtual bool ManageStop(boost::shared_ptr<Vehicule> pVeh, double dbInstant, double dbTimeStep);

    // Indique si on a besoin de lister les véhicules arrêtés dans ce TripNode
    virtual bool ListVehiclesInTripNode() {return m_bListVehiclesInTripNode;}

    // Indique si le TripNode correspond à un numéro de voie en particulier ou non
    virtual bool hasNumVoie(Trip * pTrip);

    // Indique le numéro de voie du TripNode
    virtual int getNumVoie(Trip * pTrip, TypeVehicule * pTypeVeh);

    // Calcule le temps d'arrêt du véhicule dans le TripNode
    virtual double GetStopDuration(boost::shared_ptr<Vehicule> pVehicle, bool bIsRealStop);

    // Renvoie une map d'attributs à sortir de façon spécifique pour un véhicule dans le TripNode
    virtual std::map<std::string, std::string> GetOutputAttributes(Vehicule * pV) {return std::map<std::string, std::string>();}

    // méthode pour la sauvegarde et restitution de l'état des TripNodes (affectation dynamique convergente).
    // à spécialiser si des flottes utilisent des variables de simulation supplémentaires
    virtual TripNodeSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, TripNodeSnapshot * backup);

protected:

    std::string     m_strID;
    Reseau*         m_pNetwork;

    Position        m_InputPosition;
    Position        m_OutputPosition;

    bool            m_bStopNeeded;
    bool            m_bListVehiclesInTripNode;

    // Liste des véhicules arrêtés au TripNode
    std::vector<boost::shared_ptr<Vehicule> > m_LstStoppedVehicles;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // TripNodeH