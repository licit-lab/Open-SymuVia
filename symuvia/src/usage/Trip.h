#pragma once
#ifndef TripH
#define TripH

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <vector>

class TripNode;
class TripLeg;
class Tuyau;
class Vehicule;
class Reseau;

namespace boost {
    namespace serialization {
        class access;
    }
}

class TripSnapshot {

public:
    TripSnapshot();
    virtual ~TripSnapshot();

    std::vector<int>                            assignedVehicles; // Liste des véhicules affectés au trip

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

class Trip
{
public:

    // Constructeurs / Destructeurs
    Trip();
    Trip(const Trip & other);
    virtual ~Trip();

    // Accesseurs
    void SetID(const std::string & strID);
    const std::string & GetID();
    void SetOrigin(TripNode * pOrigin);
    TripNode * GetOrigin();
    const std::vector<TripLeg*> & GetLegs();
    TripNode * GetFinalDestination();
    TripNode * GetCurrentDestination();
    TripNode * GetInitialDestination();
    std::vector<Tuyau*> * GetFullPath();
    std::vector<Tuyau*> GetFullPathThreadSafe();

	// Renvoie l'itinéraire restant dans le Trip
	std::vector<Tuyau*> GetRemainingPath();

    
    TripLeg * GetCurrentLeg();
    int GetCurrentLegIndex();
    void IncCurrentLegIndex();

    const std::vector<boost::shared_ptr<Vehicule> > & GetAssignedVehicles();
    void AddAssignedVehicle(boost::shared_ptr<Vehicule> pVeh);
    void RemoveAssignedVehicle(boost::shared_ptr<Vehicule> pVeh);
    void ClearAssignedVehicles();

    std::vector<TripNode*> GetTripNodes();
    TripNode * GetTripNode(const std::string & nodeName) const;

    // Traitements
    ///////////////////////////////////

    // Recopie le Trip vers le Trip passé en paramètres
    void CopyTo(Trip * pTrip);

    // Met à jour le Trip en fonction des tuyaux parcourus
    void SetLinkUsed(Tuyau * pLink);

    // Ajout d'une étape supplémentaire
    void AddTripLeg(TripLeg* pTripLeg);

    // Insertion d'une étape supplémentaire à une position donnée.
    // rmq. : ne met pas à jour les itinéraires correspondants.
    void InsertTripLeg(TripLeg * pTripLeg, size_t position);

    // Suppression d'une étape
    void RemoveTripLeg(size_t position);

    // Modification de l'itinéraire restant dans l'étape actuelle
    bool ChangeRemainingPath(const std::vector<Tuyau*> & newPath, Vehicule * pVeh);

    // Modification de l'itinéraire complet de l'étape Actuelle
    void ChangeCurrentPath(const std::vector<Tuyau*> & newPath, int currentLinkInPath);

    // Suppression des tronçons non encore parcourus du leg courant de l'itinéraire du leg
    // (appelé lorsqu'on passe au leg suivant)
    void EndCurrentPath();

    // Indique si le trajet implique de s'arrêter à la prochaine destination d'étape ou non
    bool ContinueDirectlyAfterDestination();

    // Indique si un itinéraire est défini pour le Trip
    bool IsPathDefined();

    // Renvoie le nombre de passage déjà effectués à un TripNode jusqu'au TripLeg courant (ne peut être > 0 que dans les cas où un Trip passe plusieurs fois par un même TripNode)
    int GetPassageNbToCurrentLeg(TripNode * pTripNode);

    // Renvoie le nombre de passage déjà effectués à un TripNode jusqu'au TripLeg d'indice tripLegIndex exclu (ne peut être > 0 que dans les cas où un Trip passe plusieurs fois par un même TripNode)
    int GetPassageNb(TripNode * pTripNode, size_t tripLegIndex);

    // méthode pour la sauvegarde et restitution de l'état des Trips (affectation dynamique convergente).
    virtual TripSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, TripSnapshot * backup);

    // Invalidation du chemin complet sur tous les legs calculé précédemment (il sera recalculé quand nécessaire)
    void InvalidateFullPath();

public:
    // Liste des véhicules assignés au trip (uniquement pour les instances de Trip associées aux flottes, pas aux véhicules)
    // rmq. mis en public pour conserver la compatibilité avec les scripts python existants
    std::vector<boost::shared_ptr<Vehicule> > m_AssignedVehicles;

private:

    std::vector<Tuyau*> * BuildFullPath();

private:

    // Identifiant du trip
    std::string             m_strID;

    TripNode               *m_pOrigin;
    std::vector<TripLeg*>   m_Path;

    // Position actuelle du véhicule dans le Trip
    int                     m_CurrentLegIndex;

    // Liste complète des tuyaux pour réaliser le Trip (équivalent de l'ancien m_Itineraire de la classe Vehicule)
    std::vector<Tuyau*>*    m_pFullPath;

    boost::mutex            m_Mutex;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif // TripH
