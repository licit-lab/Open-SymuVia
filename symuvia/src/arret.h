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
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// La classe arr�t d�crit les diff�rents param�tres relatifs aux lieux d'arr�t des
// autobus sur le r�seau
class Arret : public TripNode
{
public:
    // Constructeurs, destructeurs et assimil�s
    Arret();
    Arret(const std::string & strID, Reseau * pNetwork, Tuyau*, double distance, double tps, bool bSurVoie);
    virtual ~Arret();

    // Initialisation des variables
    virtual void Initialize(std::deque< TimeVariation<TraceDocTrafic> > & docTrafics);

    // Traitement sur entr�e d'un v�hicule dans le TripNode
    virtual void VehiculeEnter(boost::shared_ptr<Vehicule> pVehicle, VoieMicro * pDestinationEnterLane, double dbInstDestinationReached, double dbInstant, double dbTimeStep, bool bForcedOutside = false);

    // Traitement sur sortie d'un v�hicule du TripNode
    virtual void VehiculeExit(boost::shared_ptr<Vehicule> pVehicle);

    // Fonctions permettant la gestion des lignes d'autobus
    void AjoutLigne(Trip* ligne, bool bDureeArretDynamique, bool bPoisson, double dbRatioDescente, bool bBinomial, int nStockInit, RepartitionTypeVehicule * pRepTypesVeh, int num_voie = -1);   // ajoute la ligne de bus a la liste des lignes concern�es par cet arr�t

    // Fonctions de renvoi des param�tres de l'arr�t
    double  getTempsArret();         // retourne le temps d'arret
    void    setTempsArret(double dbTempsArret){tps_arret=dbTempsArret;};

    // V�rifie si une ligne passe par l'arr�t
    bool hasLine(PublicTransportLine * pLine);

    // Indique si le TripNode correspond � un num�ro de voie en particulier ou non
    virtual bool hasNumVoie(Trip * pLigne);

    // Indique le num�ro de voie du TripNode
    virtual int getNumVoie(Trip * pLigne, TypeVehicule * pTypeVeh);

    std::deque<Trip*> & getLstLTPs() { return m_LstLTPs; }

    std::deque<TimeVariation<tracked_double>>* GetLstDemandeTV() { return &m_LstDemandes; }

    // Renvoie l'objet ligne de bus (diff�rent du Trip associ� aux v�hicules)
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

    // Calcule le temps d'arr�t du v�hicule dans le TripNode
    virtual double GetStopDuration(boost::shared_ptr<Vehicule> pVehicle, bool bIsRealStop);

    // Renvoie une map d'attributs � sortir de fa�on sp�cifique pour un v�hicule dans le TripNode
    virtual std::map<std::string, std::string> GetOutputAttributes(Vehicule * pV);

    // Acc�s aux instants de passage des bus
    std::map<Trip*, std::map<int, std::pair<double, double> > > & GetPassingTimes() { return m_LastPassingTimes; }

    std::vector<Passenger> & getRealStockRestant(Trip * pLigne);
    double getLastRatioMontee(Trip * pLigne);
    void setLastRatioMontee(Trip * pLigne, double dbLastRatio);

    // m�thode pour la sauvegarde et restitution de l'�tat des arr�ts (affectation dynamique convergente).
    virtual TripNodeSnapshot * TakeSnapshot();
    virtual void Restore(Reseau * pNetwork, TripNodeSnapshot * backup);

private:

    void GestionDescente(LoadUsageParameters & loadParams);
    void GetionMontee(Trip * pLigne, boost::shared_ptr<Vehicule> pVehicle, LoadUsageParameters & loadParams, int nbPietonsMontant);


private:
    // Variables caract�ristiques de l'arr�t
    double tps_arret ;                      // temps d'arret des bus passant a cet arret
    
    bool m_bEligibleParcRelais;             // vrai si l'arr�t peut �tre associ� � un parc relais proche
	std::deque<Trip*> m_LstLTPs ;           // liste des lignes concern�es par cet arret
    std::vector<int> m_LstVoies;            // liste du num�ro de la voie pour chaque ligne associ�e
    std::vector<bool>   m_LstDureeArretDynamique;   // liste des flags indiquant si la dur�e des arr�ts est dynamique pour chaque ligne de l'arr�t
    std::vector<bool>   m_LstPoissonCreation;       // Liste indiquant si on doit utiliser une distribution de poisson pour g�rer le nombre de pi�tons cr��s
    std::vector<double> m_LstRatioDescente; // liste des ratios de descente pour chaque ligne passant par l'arr�t
    std::vector<bool>   m_LstBinomialDescente;      // Liste indiquant si on doit utiliser une loi binomiale pour g�rer le nombre de descentes � l'arr�t
    std::vector<int> m_LstStockRestant;     // liste des stocks initial de pi�tons en attente d'un bus de la ligne � cet arr�t
    std::deque<TimeVariation<tracked_double>> m_LstDemandes; // liste des variations de demande pour cet arr�t et pour chaque ligne

    std::vector<double> m_LstDernierRamassage; // Sauvegarde de l'instant de dernier ramassage des passagers � l'arr�t pour chaque ligne

    std::map<Trip*, std::map<int, std::pair<double, double> > > m_LastPassingTimes;
    std::vector<std::vector<Passenger> > m_LstWaitingUsers;
    std::vector<double> m_LstLastRatioMontee;



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif

 
