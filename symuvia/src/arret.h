#pragma once
#ifndef arretH
#define arretH

#include "usage/TripNode.h"
#include "usage/Passenger.h"
#include "tools.h"

#include <deque>
#include <vector>
#include <map>

class Tuyau;
class Trip;
class RepartitionTypeVehicule;
class PublicTransportLine;
class LoadUsageParameters;


class ArretSnapshot : public TripNodeSnapshot
{
public:
	std::vector<double> lstDernierRamassage;
    std::vector<int>    lstStockRestant;
    std::vector<std::vector<Passenger> >  lstWaitingUsers;
    std::map<Trip*, std::map<int, std::pair<double, double> > > lastPassingTimes;
    std::vector<double> lastRatioMontee;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// La classe arrêt décrit les différents paramètres relatifs aux lieux d'arrêt des
// autobus sur le réseau
class Arret : public TripNode
{
public:
    // Constructeurs, destructeurs et assimilés
    Arret();
    Arret(const std::string & strID, Reseau * pNetwork, Tuyau*, double distance, double tps, bool bSurVoie);
    virtual ~Arret();

    // Initialisation des variables
    virtual void Initialize(std::deque< TimeVariation<TraceDocTrafic> > & docTrafics);

    // Traitement sur entrée d'un véhicule dans le TripNode
    virtual void VehiculeEnter(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep, bool bForcedOutside = false);

    // Traitement sur sortie d'un véhicule du TripNode
    virtual void VehiculeExit(boost::shared_ptr<Vehicule> pVehicle);

    // Fonctions permettant la gestion des lignes d'autobus
    void AjoutLigne(Trip* ligne, bool bDureeArretDynamique, bool bPoisson, double dbRatioDescente, bool bBinomial, int nStockInit, RepartitionTypeVehicule * pRepTypesVeh, int num_voie = -1);   // ajoute la ligne de bus a la liste des lignes concernées par cet arrêt

    // Fonctions de renvoi des paramètres de l'arrêt
    double getTempsArret();         // retourne le temps d'arret

    // Vérifie si une ligne passe par l'arrêt
    bool hasLine(PublicTransportLine * pLine);

    // Indique si le TripNode correspond à un numéro de voie en particulier ou non
    virtual bool hasNumVoie(Trip * pLigne);

    // Indique le numéro de voie du TripNode
    virtual int getNumVoie(Trip * pLigne, TypeVehicule * pTypeVeh);

    std::deque<Trip*> & getLstLTPs() { return m_LstLTPs; }

    std::deque<TimeVariation<tracked_double>>* GetLstDemandeTV() { return &m_LstDemandes; }

    // Renvoie l'objet ligne de bus (différent du Trip associé aux véhicules)
    Trip * getLine(Trip * pTrip);

    void    setInstantDernierRamassage(Trip * pLigne, double dbInstant);
    double  getInstantDernierRamassage(Trip * pLigne);
    int     getStockRestant(Trip * pLigne);
    void    setStockRestant(Trip * pLigne, int nStock);
    double  getRatioDescente(Trip * pLigne);
    bool    isBinomial(Trip * pLigne);
    bool    isPoisson(Trip * pLigne);
    bool    isDureeArretDynamique(Trip * pLigne);

    void    SetEligibleParcRelais(bool bEligibleParcRelais) { m_bEligibleParcRelais = bEligibleParcRelais; }
    bool    IsEligibleParcRelais() { return m_bEligibleParcRelais; }

    // Calcule le temps d'arrêt du véhicule dans le TripNode
    virtual double GetStopDuration(boost::shared_ptr<Vehicule> pVehicle, bool bIsRealStop);

    // Renvoie une map d'attributs à sortir de façon spécifique pour un véhicule dans le TripNode
    virtual std::map<std::string, std::string> GetOutputAttributes(Vehicule * pV);

    // Accès aux instants de passage des bus
    std::map<Trip*, std::map<int, std::pair<double, double> > > & GetPassingTimes() { return m_LastPassingTimes; }

    std::vector<Passenger> & getRealStockRestant(Trip * pLigne);
    double getLastRatioMontee(Trip * pLigne);
    void setLastRatioMontee(Trip * pLigne, double dbLastRatio);

    // méthode pour la sauvegarde et restitution de l'état des arrêts (affectation dynamique convergente).
    virtual TripNodeSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, TripNodeSnapshot * backup);

private:

    void GestionDescente(LoadUsageParameters & loadParams);
    void GetionMontee(Trip * pLigne, boost::shared_ptr<Vehicule> pVehicle, LoadUsageParameters & loadParams, int nbPietonsMontant);


private:
    // Variables caractéristiques de l'arrêt
    double tps_arret ;                      // temps d'arret des bus passant a cet arret
    bool m_bEligibleParcRelais;             // vrai si l'arrêt peut être associé à un parc relais proche
	std::deque<Trip*> m_LstLTPs ;           // liste des lignes concernées par cet arret
    std::vector<int> m_LstVoies;            // liste du numéro de la voie pour chaque ligne associée
    std::vector<bool>   m_LstDureeArretDynamique;   // liste des flags indiquant si la durée des arrêts est dynamique pour chaque ligne de l'arrêt
    std::vector<bool>   m_LstPoissonCreation;       // Liste indiquant si on doit utiliser une distribution de poisson pour gérer le nombre de piétons créés
    std::vector<double> m_LstRatioDescente; // liste des ratios de descente pour chaque ligne passant par l'arrêt
    std::vector<bool>   m_LstBinomialDescente;      // Liste indiquant si on doit utiliser une loi binomiale pour gérer le nombre de descentes à l'arrêt
    std::vector<int> m_LstStockRestant;     // liste des stocks initial de piétons en attente d'un bus de la ligne à cet arrêt
    std::deque<TimeVariation<tracked_double>> m_LstDemandes; // liste des variations de demande pour cet arrêt et pour chaque ligne

    std::vector<double> m_LstDernierRamassage; // Sauvegarde de l'instant de dernier ramassage des passagers à l'arrêt pour chaque ligne

    std::map<Trip*, std::map<int, std::pair<double, double> > > m_LastPassingTimes;
    std::vector<std::vector<Passenger> > m_LstWaitingUsers;
    std::vector<double> m_LstLastRatioMontee;



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif

 
