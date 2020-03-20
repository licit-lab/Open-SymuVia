#pragma once
#ifndef affectationH
#define affectationH

#include "AffectationSave.h"

class SimulationSnapshot;

class SymuViaTripNode;
class TraceDocTrafic;
class Reseau;
class TuyauMicro;
class Connexion;

// Classe d'implantation du module d'affectation
class Affectation
{
private:

	CPeriode m_Periode;							// Stocke les information d'une période de simulation avec les itérations

	// Paramétrage du module d'affectation
	bool	m_bLastInstSimu;					// Indique si calcul d'affectataion du dernier instant de la simu

	int		m_nPeriodeAffectation;				// Coefficient multiplicateur du pas de temps définissant la période d'affectation

	int		m_nMode;							// Mode d'affectation : 1 -> logit, 2 -> wardrop

	double	m_dbSeuilConvergence;				// Seuil de convergence

	int		m_nNbItMax;							// Nombre d'itération max

	bool	m_bConvergent;						// Indique si le processus d'affectation est convergent (par une méthode itérative) ou au fil de l'eau
	int		m_nAffectEqui;						// Type d'équilibre voulu (1: sans, 2 : équilibre sans mémoire, 3 : équilibre approché par la méthode des moyennes successives)

	char	m_cTypeTpsParcours;					// Type de calcul du temps de parcours : 'P' pour prédictif 1 (Nico), 'Q' pour prédictif 2 (Ludo) et 'E' pour estimé

	char	m_cLieuAffectation;					// Type de lieu d'affectation	- 'E' : uniquement aux entrées (lors de la création du véhicule, son itinéraire lui est affecté et il le suit jusqu'à la fin)
												//								- 'C' : à toutes les connexions (l'itinéarire du véhicule est calculé à chaque connexion en fonction des coefficients d'affecation de la connexion)

	double	m_dbVariationTempsDeParcours;		// Limite inférieure de la variation du temps de parcours pour laquelle un recalcul des coefficients d'affectation au niveau des connexions internes est effectué
	int		m_nNbNiveauAmont;					// Nombre de niveau du réseau d'affectation à remonter pour recalcul des coefficients d'affectation suite à une variation importante du temps de parcours

	double	m_dbTetaLogit;						// Paramètre teta du modèle logit

	double	m_dbSeuilTempsParcours;				// Seuil maximal d'écart des temps de parcours réalisés et prédits pour considérer un tronçon comme correctement affecté

	bool	m_bDijkstra;						// Mode de calcul des k plus courts chemins

	int		m_nNbPlusCourtCheminCalcule;		// Nombre de plus court chemin à calculer

	int		m_nIteration;

    double  m_dbStartPeriodInstant;

    DOMLSSerializerSymu*		m_XmlWriterAffectation;		// XmlWriter pour les affectations

    bool m_bReroutageVehiculesExistants;
    double m_dbDstMinAvantReroutage;
    double m_dbWardropTolerance;

    // Map de stockage de la longueur des itinéraires (pour ne pas les recalculer à chaque fois (non sérialisée car pas nécessaire)
    std::map<std::vector<Tuyau*>, double> m_mapItineraryLengths;

    // Etats de sauvegarde de la simulation au début de la période d'affectation (pour affectation dynamique convergente)
    std::vector<SimulationSnapshot*> m_SimulationSnapshots;

public:

	Affectation();
	Affectation(int nPeriodeAffectation, int nAffectEqui, double dbVariationTempsDeParcours, int nNbNiveauAmont, 
				double dbTetaLogit, char cTypeTpsParcours, int nMode, double dbSeuilConvergence, int nNbItMax, bool bDijkstra,
				int nNbPlusCourtCheminCalcule, double dbSeuilTempsParcours, bool bReroutageVehiculesExistants, double dbDstMinAvantReroutage,
                double dbWardropTolerance);
    ~Affectation();

	bool	Run(Reseau *pReseau, double dbInstant, char cTypeEvent, bool bForceGraphReset, TypeVehicule * pTVA = NULL, SymuViaTripNode * pDestinationA = NULL, SymuViaTripNode * pOrigineA = NULL);	

    void    TakeSimulationSnapshot(Reseau *pReseau);
    void    SimulationCommit(Reseau *pReseau);
    void    SimulationRollback(Reseau *pReseau, size_t snapshotIdx);
    
    void    CalculTempsDeParcours(Reseau * pReseau, double dbInstant, char cTypeEvent, double dbPeriode);
    void    FinCalculTempsDeParcours(Reseau * pReseau, double dbInstant, char cTypeEvent, double dbPeriode);

	bool	IsConvergent() const{return m_bConvergent;};
	char	GetLieuAffectation(){return m_cLieuAffectation;};

	int		GetMode(){return m_nMode;};
	double	GetSeuilConvergence(){return m_dbSeuilConvergence;};
	int		GetNbItMax(){return m_nNbItMax;};
	
	// Gestion de la sauvegarde des périodes d'affectation (public)
	void	InitSaveAffectation(bool bSave, Reseau *pReseau);		// Initialisation de la sauvegarde des affectations
	void	CloseSaveAffectation();									// Fermeture du fichier de sauvegarde des affectations
	void	IncreaseNBVehEmis(TypeVehicule*pVeh, const std::string & sOrig, const std::string & sDest); // Augmentation du nombre de type de vehicule émis
	void	IncreaseNBVehRecus(TypeVehicule*pVeh, const std::string & sOrig, const std::string & sDest); // Augmentation du nombre de type de vehicule reçus
	void	AddRealiseItineraire(TypeVehicule * pTVeh, const std::string & sOrig, const std::string  & sDest, const std::vector<std::string> & listIti, const std::vector<Tuyau*> & tuyaux,
                                 bool bPredefini, int vehID);       // Ajout d'un itinéraire réalisé
	bool	IsSaving() { return (m_XmlWriterAffectation != NULL); } // Renvoie true si la sauvegarde de l'affectation est programmée
	// Fin gestion de la sauvegarde des périodes d'affectation (public)

    TraceDocTrafic * GetCurrentDocTraficTmp();

private:

	// Gestion des la sauvegarde des périodes d'affectation (private)
	void	SaveAffectationSimulation(Reseau *pReseau); // Sauvegarde du noeud "SIMULATION" de la sauvegarde des affectations

	void	GetCnxAmont( TuyauMicro* pT, int &nNiveau, std::set<Connexion*> &dqCnx);
	double	GetProportionCritereOK(Reseau *pReseau, TypeVehicule *pTV);

    double  GetItineraryLength(const std::vector<Tuyau*> & itinerary);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif