#pragma once
#ifndef tuyauH
#define tuyauH

#pragma warning(push)
#pragma warning(disable : 4251)

#include "SymubruitExports.h"

#include "troncon.h"

#include <float.h>
#include <algorithm>

#define     RESOTUYAU_MACRO     1
#define     RESOTUYAU_MICRO     2
#define		RESOTUYAU_MESO		3

#define     FACT_DEMANDENULLE   1      // Constante correspondant au facteur multiplicatif
                                       // de 1/Keps permettant de définir la zone d'un tyuau
                                       // microscopique où la demande est nulle
#define _CRTDBG_MAP_ALLOC

#ifdef USE_SYMUCORE
class EdieSensor;
namespace SymuCore {
    class MacroType;
	class RobustTravelIndicatorsHelper;
}
#endif // USE_SYMUCORE

//---------------------------------------------------------------------------
class Arret;
class Voie;
class ConnectionPonctuel;
class Loi_emission;
class BriqueDeConnexion;
class Connexion;
class CDiagrammeFondamental;
class Segment;
class PonctualSensor;
class AbstractSensor;
class SensorsManager;
class Vehicule;

// Structure de description d'une zone de dépassement interdite
struct ZoneDepassementInterdit
{
public:
    ZoneDepassementInterdit() {};
    ZoneDepassementInterdit(double debut, double fin)
    {
        dbPosDebut = debut;
        dbPosFin = fin;
    };

    double  dbPosDebut;
    double  dbPosFin;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure de description d'une zone de définition de zLevel le long d'un tronçon
struct ZoneZLevel
{
public:
    ZoneZLevel() {};
    ZoneZLevel(double debut, double fin, int zLevel)
    {
        dbPosDebut = debut;
        dbPosFin = fin;
        nZLevel = zLevel;
    };

    double  dbPosDebut;
    double  dbPosFin;
    int     nZLevel;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Définition de la notion de vitesse reglementaire sur une portion de tronçon
typedef Portion<double> VitRegPortion;

// Définition détaillée des vitesses réglementaires pour une variante temporelle d'un tronçon 
struct VitRegDescription
{
public:
    VitRegDescription() {}

    // organisation de la description des vitesses réglementaires par type de véhicule, puis par voie
    std::map<TypeVehicule*, std::map<int, std::vector<VitRegPortion> > >  vectPortions;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


// La classe tuyau gère les objets tuyaux du réseau qui sont les lieux où
// les véhicules s'écoulent. Les tuyaux se décomposent en voie puis en segments
// pour le calcul effectif du trafic
class SYMUBRUIT_EXPORT Tuyau : public Troncon
{
public:
    //! Différents types de tuyaux existants
    enum ETuyauType
    {
        TT_MICRO,
        TT_MACRO,
        TT_MESO
    };

protected:

   // Variables caractéristiques des tuyaux

   int     m_nResolution;   // Résolution du tuyau ( RESOTUYAU_MACRO , RESOTUYAU_MICRO, RESOTUYAU_MESO )

   // Coutures du réseau de voirie
   char Type_amont;							// Type de l'élément amont (R: Répartiteur, E: Entrée)
   char Type_aval;							// Type de l'élement aval (R: Répartiteur, S: Sortie, D: Divergent, C: Convergent)
   ConnectionPonctuel *p_aval;				// Pointeur sur la connection aval 
   ConnectionPonctuel *p_amont;				// Pointeur sur la connection amont

   BriqueDeConnexion   *m_pBriqueAmont;    // Pointeur sur la brique amont
   BriqueDeConnexion   *m_pBriqueAval;     // Pointeur sur la brique aval

   // Coutures du réseau d'affectation
   Connexion *m_pCnxAssAm;					// Pointeur sur la connection aval
   Connexion *m_pCnxAssAv;					// Pointeur sur la connection amont 

   int     m_nNbVoies;             // Nb de voies de circulation
   int     m_nNbVoiesDis;          // Nb de voies de circulation que l'on traite de façon dissociées

   //parametres géographiques
   std::vector<double> m_largeursVoie; // largeurs des voies

   std::deque<double>   m_dqRepartitionEntreeVoie;  // Liste des répartitions en entrée pour chaque chaque voie

   std::vector<Voie*>   m_pLstVoie;                      // Liste des voies

   BriqueDeConnexion*   m_pBriqueParente;       // Brique de connexion parente si tuyau virtuel    

   std::vector<PonctualSensor*>    m_LstCapteursConvergents;   // Liste des capteurs ponctuels liés à un convergent et positionnés sur ce tuyau

   // Variables de simulation
   int     m_nNbPasTempsHist;              // Nombre de pas de temps historisé duarnt la simulation

   // Utile pour l'exploration de graphe
   bool		m_bExpGrTaboo;

   int      m_nRealNbVoies;     // taille réelle de m_pLstVoies

   // Pour gestion du tronçon opposé
   Tuyau*   m_pTuyauOppose;
   std::vector<ZoneDepassementInterdit> m_LstZonesDepassementInterdit;

   // Capacité de stationnement en mètres
   double   m_dbCapaciteStationnement;
   // Stock actuel de véhicules garés en mètres
   double   m_dbStockStationnement;

   // gestion du zLevel
   std::vector<ZoneZLevel> m_LstZonesZLevel;

   // Coefficient de pénalisation (Evolution SymuVia n°75)
   double   m_dbPenalisation;

   // Functional class du tronçon (notion navstreet)
   int      m_iFunctionalClass;

   // Flag indiquant si le temps de parcours a été mesuré ou non (i.e., si la fonction ComputeCost renvoie un cout estimé théorique ou
   // un cout obtenu empiriquement)
   bool     m_bHasRealTravelTime;

   // Liste des capteurs liés à ce tuyau (ie à potentiellement mettre à jour si un véhicule enprunte le tuyau)
   std::map<SensorsManager*, std::vector<AbstractSensor*> > m_LstRelatedSensorsBySensorsManagers;

   // Permet d'interdire arbitrairement un tuyau en cours de simulation, cf. nouvelles méthodes exportées type SymForbidLink
   bool		m_bIsForbidden;

public:
    ListOfTimeVariation<VitRegDescription> m_LstVitReg;    // Liste des variantes de définition de la vitesse réglementaire sur le tronçon
    std::map<TypeVehicule*,  std::map<int, ListOfTimeVariation<tracked_bool> > >	m_mapVoiesReservees;   // Map des voies reservees par véhicule et par voie
    std::map<TypeVehicule*, std::map<int, bool> >                                   m_mapVoiesInterdites;  // Map des voies interdites par véhicule et par voie

        // Constructeurs, destructeurs et assimilées
    virtual ~Tuyau(void);  // Destructeur
	Tuyau(void); // Constructeur

	Tuyau(      Reseau* pReseau, std::string _Nom,char _Type_amont ,char _Type_aval,ConnectionPonctuel *_p_aval,ConnectionPonctuel *_p_amont,
                 char typevoie, std::string nom_rev, std::vector<double> larg_voie, double _Abscisse_amont,double _Ordonnee_amont,
                 double _Abscisse_aval,double _Ordonnee_aval, double _Hauteur_amont,double _Hauteur_aval, int _Nb_cel, int _Nb_voies,
                 double pastemps, double dbVitReg, double dbVitCat, const std::string & strRoadLabel);

                // Constructeur complet
                // (Nom du troncon, Type ConnectionPonctuel amont, Type connection aval, p sur connection aval, p sur connection amont,
                //  Type de voie <urbain, périurbain, dense>, présence d'une voie bus (1: oui, 0: non), Nom du revêtement,
                //  largeur de la voie, Abscisse amont, Ordonnee amont, Abscisse aval, Ordonnee aval, Nb de cellules, Nb de cellules,
                //  Type de diagramme fondamental, vitesse libre, vitesse critique, Débit max, concentration max, Nb de voies,
                //  Mode de gestion des voies, accélération maximale, pas de temps)

        virtual ETuyauType GetType() const =0;
        int     GetResolution(){return m_nResolution;}
        void    SetResolution(int nResolution){m_nResolution = nResolution;}
        bool    IsMacro(){return m_nResolution == RESOTUYAU_MACRO;}
        bool    IsMicro(){return m_nResolution == RESOTUYAU_MICRO;}
		bool	IsMeso(){ return m_nResolution == RESOTUYAU_MESO; }

        // Fonctions liés à la discrétisation et la simulation des tronçons
        virtual void    InitVarSimu() = 0;     // Initialisation des variables de simulation
        virtual bool    Segmentation() = 0; // Décomposition des tronçons en voies puis segment
                      // Initialisation de la simulation
        virtual void    CalculEcoulementConnections(double);       // Simulation des écoulements aux connections

        void    GestionDiscontinuitesMobiles(double PasDeTps, bool    bOffreModifie);   // Gestion des discontinuités mobiles
        void    FinCalculEcoulementConnections(double);                             // Fin de la siulation des écoulements aux connections
                 // Simulation des écoulements internes du tuyau avant calcul de la contribution des DM
        virtual void    PostCalculEcoulementInterne(double, bool bOffreModifie){};

        virtual void    ComputeDemand(double dbInstSimu);                                // Calcul de la demande à la sortie du tronçon
        virtual void    ComputeOffer();                                  // Calcul de l'offre en entrée du tronçon

        virtual void    TrafficOutput()                                  = 0;
        virtual void    EmissionOutput()            = 0;

        virtual void    ComputeNoise         (Loi_emission* Loi)                               = 0;  

        virtual void    CalculDebitEntree(double h);
        virtual void    CalculDebitSortie(double dbInstant);

		virtual double	ComputeCost(double dbInst, TypeVehicule    *pTypeVeh, bool bUsePenalisation) = 0;  
        virtual void    SetPenalisation(double dbPenalisation) {m_dbPenalisation = dbPenalisation;}
        virtual double  GetPenalisation() { return m_dbPenalisation; }
        virtual void    SetFunctionalClass(int iFunctionalClass) { m_iFunctionalClass = iFunctionalClass; }
        virtual int     GetFunctionalClass() { return m_iFunctionalClass; }
        virtual double  GetTempsParcourRealise(TypeVehicule * pTypeVehicule) const = 0; 

        virtual bool    HasRealTravelTime() {return m_bHasRealTravelTime;}

        std::vector<PonctualSensor*> &getListeCapteursConvergents();

        std::map<SensorsManager*, std::vector<AbstractSensor*> > &getLstRelatedSensorsBySensorsManagers();

        // Fonctions d'affectation des variables des tronçons
        void setConnectionAval(ConnectionPonctuel*); // (p sur connection aval)
        void setConnectionAmont(ConnectionPonctuel*); // (p sur connection amont)

        // Fonctions de renvoi des variables des tronçons
        ConnectionPonctuel* getConnectionAval(void); // renvoi pointeur sur connection aval
        ConnectionPonctuel* getConnectionAmont(void); // renvoi pointeur sur connection amont

        char get_Type_aval(void);           // renvoie le type d'élement aval
        void SetTypeAval(char cType){Type_aval = cType;};
        char get_Type_amont(void);          // renvoie le type d'élement amont
        void SetTypeAmont(char cType){Type_amont = cType;};

        int getNb_voies(void) const; // renvoi le nb de voies
        void setNb_voies(int iNbVoies);
        int getNbVoiesDis(){return m_nNbVoiesDis;}; // renvoi le nb de voies

        double GetVitesseMaxSortie(void); // renvoi la vitesse maximale autorisée en sortie
        double GetVitesseMaxSortie(int nVoie); // retourne la vitesse maximale d'une voie
        double getLargeurVoie(int nNumVoie); // renvoi la largeur de la voie
        std::vector<double> getLargeursVoies() {return m_largeursVoie;};
        char getTypeVoie(); // renvoi le type de voie        

        double  GetDemande(int nVoie);  // Renvoie la demande pour une voie
        double  GetOffre(int nVoie);    // Renvoie l'offre pour une voie

        int     GetNbPasTempsHist(){return m_nNbPasTempsHist;};

virtual        char    GetLanesManagement() = 0;
virtual void    InitAcousticVariables(){};

        // Fonctions utilitaires      

virtual        void            DeleteLanes() = 0;

        void            AjouteRepartitionEntreeVoie(double dbRepart){m_dqRepartitionEntreeVoie.push_back(dbRepart);};
        void            VideRepartitionEntreeVoie(){ m_dqRepartitionEntreeVoie.erase(m_dqRepartitionEntreeVoie.begin(), m_dqRepartitionEntreeVoie.end());};

        double          GetRepartitionEntreeVoie(int i){return m_dqRepartitionEntreeVoie[i];};        

        virtual void    ComputeTrafficEx(double dbInstant)            {};
        
        std::vector<Voie*>&     GetLstLanes(){return m_pLstVoie;};

        BriqueDeConnexion*      GetBriqueAmont(){return m_pBriqueAmont;};
        BriqueDeConnexion*      GetBriqueAval(){return m_pBriqueAval;};

        void            SetBriqueAmont(BriqueDeConnexion *pBrique){m_pBriqueAmont = pBrique;};
        void            SetBriqueAval(BriqueDeConnexion *pBrique){m_pBriqueAval = pBrique;};

        void            SetBriqueParente(BriqueDeConnexion *pBrique){m_pBriqueParente = pBrique;}
        BriqueDeConnexion*      GetBriqueParente(){return m_pBriqueParente;};

		Connexion*		GetCnxAssAm(){return m_pCnxAssAm;};
		Connexion*		GetCnxAssAv(){return m_pCnxAssAv;};

		void			SetCnxAssAm(Connexion* pCnx){ m_pCnxAssAm = pCnx; };
		void			SetCnxAssAv(Connexion* pCnx){ m_pCnxAssAv = pCnx; };

		// Utile pour l'exploration de graphe
		void		SetExpGrTaboo( bool bTaboo ){m_bExpGrTaboo = bTaboo;};
		bool		IsExpGrTaboo(){return m_bExpGrTaboo;};

		Voie*		GetVoie(int i);

        Tuyau*      GetTuyauOppose() {return m_pTuyauOppose;};
        bool        SetTuyauOppose(Tuyau * pTuy, Logger * plog);

        void        AddZoneDepassementInterdit(double dbDebut, double dbFin);
        bool        IsDepassementInterdit(double dbPos);

        double      GetCapaciteStationnement() {return m_dbCapaciteStationnement;}
        void        SetCapaciteStationnement(double dbCapacite) {m_dbCapaciteStationnement = dbCapacite;}

        double      GetStockStationnement() {return m_dbStockStationnement;}
        void        IncStockStationnement(double dbValue) {m_dbStockStationnement = std::min<double>(m_dbStockStationnement + dbValue, m_dbCapaciteStationnement==-1?DBL_MAX:m_dbCapaciteStationnement);}
        void        DecStockStationnement(double dbValue) {m_dbStockStationnement = std::max<double>(m_dbStockStationnement - dbValue, 0);}

        bool        AddZoneZLevel(double dbDebut, double dbFin, int zLevel);
        std::vector<ZoneZLevel> GetZLevelCrossings() {return m_LstZonesZLevel;}

		void		SetForbidden(bool bForbiden);
		bool		GetIsForbidden() const;

        void        AddVoieInterditeByTypeVeh(std::vector<TypeVehicule*> typesInterdits, int voie = -1);
        void        AddVoieReserveeByTypeVeh(std::vector<TypeVehicule*> typesInterdits, double dbLag, double dbDuree, PlageTemporelle * pPlage, double dbDureeSimu, int nVoie, bool bActive = true);

        double      GetVitRegByTypeVeh(TypeVehicule *pTV, double dbInst, double dbPos, int nVoie);
        double      GetMaxVitRegByTypeVeh(TypeVehicule *pTV, double dbInst, double dbPos);
        double		GetMaxVitReg(double dbInst, double dbPos, int numVoie);

        // Pilotage de la vitesse limite
        bool        SendSpeedLimit(double dbInst, TypeVehicule *pTV, int numVoie, double dbDebutPortion, double dbFinPortion, double dbSpeedLimit);

        // Indique si le troncon est interdit globalement à la circulation
        bool        IsInterdit(TypeVehicule *pTV, double dbInst);
        // Indique si la voie spécifiée est interdite à la circulation du type de véhicule spécifié
        bool        IsVoieInterdite(TypeVehicule *pTV, int nVoie, double dbInst);

        // Indique si le troncon est interdit globalement à la circulation sur l'ensemble de la durée de simulation
        bool        IsInterdit(TypeVehicule *pTV);

        // Indique si la voie spécifiée est interdite à la circulation du type de véhicule spécifié sur l'ensemble de la durée de simulation
        bool        IsVoieInterdite(TypeVehicule *pTV, int nVoie);

		// Indique si la voie spécifiée est réservée pour un type de véhicule à un moment précis
		bool		IsVoieReservee(TypeVehicule *pTV, int nVoie, double dbInst);

        // construit une polyligne correspondant à la géométrie du tronçon, à une position transversale donnée
        std::deque<Point> GetLineString(double offset);



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Tuyau)

class SYMUBRUIT_EXPORT TuyauMacro : public Tuyau
{
private :

        bool    m_bCondVitAmont;        // Indique si la condition delta_x / delta_t est respectée (calcul de N en amont)
        bool    m_bCondVitAval;         // Indique si la condition "Vx / w est un entier" est respectée (calcul de N en aval)

        double  m_dbCoeffAmont1;        // Coefficent multiplicateur pour le calcul de la valeur de N en amont  (en t)
        double  m_dbCoeffAmont2;        // Coefficent multiplicateur pour le calcul de la valeur de N en amont  (en t - delta_t)
                                        //  m_dbCoeff1Amont + m_dbCoeff2Amont = 1

        double  m_dbCoeffAval1;         // Coefficent multiplicateur pour le calcul de la valeur de N en aval  (en t - n*delta_t)
        double  m_dbCoeffAval2;         // Coefficent multiplicateur pour le calcul de la valeur de N en aval  (en t - (n-1)*delta_t)
                                        //  m_dbCoeffAval1 + m_dbCoeffAval2 = 1

        CDiagrammeFondamental   *m_pDF;

public :

    TuyauMacro();
	TuyauMacro(     Reseau* pReseau, std::string _Nom,char _Type_amont ,char _Type_aval,ConnectionPonctuel *_p_aval,ConnectionPonctuel *_p_amont,
                    char typevoie, char nom_rev[256], std::vector<double> larg_voie, double _Abscisse_amont,double _Ordonnee_amont,
                    double _Abscisse_aval,double _Ordonnee_aval, double _Hauteur_amont, double _Hauteur_aval, int _Nb_cel, int _Nb_voies,
                    double pastemps, double dbVitReg, double dbVitCat, const std::string & strRoadLabel);

    virtual ~TuyauMacro();    

    virtual ETuyauType GetType() const { return TT_MACRO; }
    // Fonctions virtuelles
    virtual void    InitVarSimu();
    virtual bool    Segmentation();    

    virtual void    TrafficOutput();    

    virtual void    ComputeNoise         (Loi_emission* Loi);

    virtual void    EmissionOutput(){};     

	virtual        void            DeleteLanes();

    void            CalculDebit(bool    bOffreModifie);         // calcul du débit
    void            RecalculDebit(bool bOffreModifie);
    
    // Conditions de la vitesse et coefficient de pondération pour le calcul de N
    void            SetCondVitAmont(bool bCondVit){m_bCondVitAmont = bCondVit;};
    void            SetCondVitAval(bool bCondVit){m_bCondVitAval = bCondVit;};
    bool            GetCondVitAmont(){return m_bCondVitAmont;};
    bool            GetCondVitAval(){return m_bCondVitAval;};
    void            SetCoeffAmont(double dbPasEspace, double dbPasTemps, double dbVitmax);
    void            SetCoeffAval(double dbVal);
    double          GetCoeffAmont1(){return m_dbCoeffAmont1;};
    double          GetCoeffAmont2(){return m_dbCoeffAmont2;};
    double          GetCoeffAval1(){return m_dbCoeffAval1;};
    double          GetCoeffAval2(){return m_dbCoeffAval2;};

    virtual void    ComputeTraffic(double dbInstant);
    virtual void    ComputeTrafficEx(double dbInstant);

	virtual bool    InitSimulation(bool bAcou, bool bSirane, std::string strName);

    virtual char    GetLanesManagement(){return 'G';};

	virtual double	ComputeCost(double dbInst, TypeVehicule* pTypeVeh, bool bUsePenalisation);  
    virtual double    GetTempsParcourRealise(TypeVehicule * pTypeVehicule) const {
        assert(true); // TODO
        return DBL_MAX;
    }; 
    CDiagrammeFondamental*  GetDiagFonda(){return m_pDF;};    

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure de définition d'un véhicule d'un tuyau microscopique
struct structVehicule
{
    double      dbPosition;         // Position
    double      dbPositionAnt;      // Position à la fin du pas de temps précédent
    double      dbVitesse;          // Vitesse
    double      dbVitesseAnt;       // Vitesse à la fin du pas de temps précédent
    double      dbVitesseAntAnt;    // Vitesse à la fin du pas de temps précédent du précédent
    int         nId;                // Identifiant unique du véhicule
    bool        VehAutonome;        // Définit si le véhicule est autonome pour définir la demande ou pas
    bool        VehAccelere;        // Définit si le véhicule accélère ou pas
    double      dbDebitCreation;    // Valeur du débit quand le véhicule a été crée

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure de définition d'une traversée prioritaire
struct Traversee
{
    Tuyau*      pTuyauTraversant;           // Tronçon prioritaire à traverser
    double      dbPosition;                 // Position du point de conflit sur le tronçon non prioriataire
    double      dbPositionTraversant;       // Position du point de conflit sur le tronçon prioriataire
    double      dbInstLastTraversee;        // Instant de la dernière traversée
    //std::deque<TempsCritique>   LstTf;     // Liste des temps de traversées
    int         nVoieTraversant;            // Voie du tronçon non prioritaire que l'on étudie (0 par défaut)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


class SYMUBRUIT_EXPORT TuyauMicro : public Tuyau
{
private:    
    int             m_nNbCellAcoustique;            // Nombre de cellule acoustique
	double			m_dbAcousticCellLength;			// Acoustic cell length

	bool			m_bAgressif;					// Indique si les véhicules du tronçon peuvent être agressifs

    

    std::deque<Traversee*>   m_LstTraversee;        // Liste des traversées prioritaires à traiter

	bool			m_bChtVoieVersDroiteAutorise;	// Indique si le chnagment de voie vers la voie la plus à droite est autorisé

   // Pour gestion des cellules d'agregation des sorties pour Sirane
   std::vector<Segment*> m_LstCellSirane;

public:
	std::map<TypeVehicule*, double>		m_mapTempsParcours;							// Map de stockage des temps de parcours par type de véhicule utilisé pour l'affectation
	std::map<TypeVehicule*, double>		m_mapTempsParcoursPrecedent;				// Map de stockage des temps de parcours utilisé pour l'affectation par type de véhicule de la précédente période de calcul d'affectation
	std::map<TypeVehicule*, double>		m_mapTempsParcoursRealise;					// Map de stockage des temps de parcours réalisé par type de véhicule	

	std::map<int, std::pair< TypeVehicule*, std::pair<double, double>> > m_mapVeh;	// Map des véhicules sortant du tuyau au cours de la période d'affectation courante
																					// avec instant exact d'entrée et de sortie du tuyau
	std::map<TypeVehicule*, std::pair<double, double>> m_mapDernierVehSortant;		// Map du dernier véhicule sorti par type de véhicule (lors de la période d'affectation précédente) avec instant exact d'entrée et de sorti

	std::map<TypeVehicule*, int>		m_mapNbVehEntre;							// Map du nombre cumulé de véhicule entré sur le tronçon depuis le début de la simulation
	std::map<TypeVehicule*, int>		m_mapNbVehSorti;							// Map du nombre cumulé de véhicule sorti du tronçon depuis le début de la simulation
	std::map<TypeVehicule*, int>		m_mapNbVehSortiPrev;						// Map du nombre cumulé de véhicule sorti sur le tronçon depuis le début de la simulation jusqu'à la précédente période d'affectation
	std::map<TypeVehicule*, int>		m_mapNbVehEntrePrev;						// Map du nombre cumulé de véhicule entré sur le tronçon depuis le début de la simulation jusqu'à la précédente période d'affectation

#ifdef USE_SYMUCORE
    std::map<SymuCore::MacroType*, double> m_mapTTByMacroType;                      // temps de parcours calculé pour SymuMaster par macro-type de véhicule
    std::map<SymuCore::MacroType*, double> m_mapMarginalByMacroType;                // marginals calculé pour SymuMasyer par macro-type de véhicule
    double                                 m_dbTTForAllMacroTypes;                  // temps de parcours sans tri par macro-types, pour calcul du marginal a postériori en mode spatial
	double								   m_dbEmissionForAllMacroTypes;			// émissions sans tri par macro-types, pour calcul du marginal a postériori en mode spatial

    std::map<SymuCore::MacroType*, double> m_dbMeanNbVehiclesForTravelTimeUpdatePeriod;
	SymuCore::RobustTravelIndicatorsHelper*	   m_pRobustTravelTimesHelper;			// Structure utile pour calcul des temps de parcours robustes (et des marginals associées) sans tri par macro-type
	SymuCore::RobustTravelIndicatorsHelper*	   m_pRobustTravelSpeedsHelper;			// Structure utile pour calcul des vitesses robustes sans tri par macro-type

    EdieSensor*                            m_pEdieSensor;                           // Capteur d'Edie utile pour les calculs de temps de parcours pour SymuMaster par méthode spatiale                    
#endif

    // Variable de travail par pas de temps (pas la peine de serialisation) pour le mode de dépassement pour changement de direction anti congestion de Charlotte.
    std::map<Tuyau*, Vehicule*>             m_mapVehiclesForDirChangeOvertakeMode;

public:

    TuyauMicro();
	TuyauMicro(     Reseau* pReseau, std::string _Nom,char _Type_amont ,char _Type_aval,ConnectionPonctuel *_p_aval,ConnectionPonctuel *_p_amont,
                    char typevoie, std::string nom_rev, std::vector<double> larg_voie, double _Abscisse_amont,double _Ordonnee_amont,
                    double _Abscisse_aval,double _Ordonnee_aval, double _Hauteur_amont, double _Hauteur_aval, int _Nb_voies,
                    double pastemps,
                    int nNbCellAcoustique, double dbVitReg, double dbVitCat, double dbCellAcousLength, const std::string & strRoadLabel);

    virtual ~TuyauMicro();    

    virtual ETuyauType GetType() const { return TT_MICRO; }
    // Méthodes virtuelles pures de Tuyau    
    virtual bool    Segmentation();
    virtual void    InitVarSimu(){};
	virtual bool    InitSimulation(bool bCalculAcoustique, bool bSirane, std::string sLabel);                         // Initialisation de la simulation
    virtual void    ComputeTraffic(double dbInstant);
    virtual void    TrafficOutput();
    virtual void    ComputeNoise         (Loi_emission* Loi)                               {};
    virtual void    EmissionOutput();
    virtual char    GetLanesManagement(){return 'D';};
    virtual void    InitAcousticVariables();

    virtual        void            DeleteLanes();

    void            EndComputeTraffic(double dbInstant);

    int             GetNbCellAcoustique(){return m_nNbCellAcoustique;};
	double          GetAcousticCellLength(){return m_dbAcousticCellLength;};

    virtual	double  ComputeCost(double dbInst, TypeVehicule    *pTypeVeh, bool bUsePenalisation);    
    double          GetCoutAVide(double dbInst, TypeVehicule    *pTypeVeh, bool bIgnoreVitCat, bool bIncludeDownstreamPenalty = false);
    virtual double  GetTempsParcourRealise(TypeVehicule * pTypeVehicule) const;



    void            SetNbCellAcoustique(int nNb){m_nNbCellAcoustique = nNb;};
	void			SetAcousticCellLength(double dbAcousticCellLength){ m_dbAcousticCellLength = dbAcousticCellLength;};

    Traversee*      AddTraversee(Tuyau *pT, double dbPosNonPrioritaire, double dbPosPrioritaire, int nVoieTraversant = 0);        

    double          GetMaxRapVitSurQx(double dbInst);

	bool			IsAgressif(){return m_bAgressif;};
	void			SetAgressif( bool bAgressif ){m_bAgressif = bAgressif;};

	virtual void	CalculTempsDeParcours( double dbInst, TypeVehicule *pTypeVeh, char cTypeEvent, double dbPeriode, char cTypeCalcul );
	void			FinCalculTempsDeParcours( double dbInst, TypeVehicule *pTypeVeh, char cTypeEvent, double dbPeriode, char cTypeCalcul );
#ifdef USE_SYMUCORE
    void            CalculTempsDeParcours(double dbInst, SymuCore::MacroType * pMacroType, double dbPeriod, bool bPollutantEmissionComputation);
    double          GetTravelTime(SymuCore::MacroType * pMacroType);
    double          GetMarginal(SymuCore::MacroType * pMacroType);
	double			GetEmissionForAllMacroTypes() { return m_dbEmissionForAllMacroTypes; };
    double          GetMeanNbVehiclesForTravelTimeUpdatePeriod(SymuCore::MacroType * pMacroType);
	SymuCore::RobustTravelIndicatorsHelper*	GetRobustTravelTimesHelper() { return m_pRobustTravelTimesHelper; };
	SymuCore::RobustTravelIndicatorsHelper*	GetRobustTravelSpeedsHelper() { return m_pRobustTravelSpeedsHelper; };

#endif // USE_SYMUCORE
	int				GetNbVehPeriodeAffectation(TypeVehicule *pTypeVeh);

	bool			IsChtVoieVersDroiteAutorise(){return m_bChtVoieVersDroiteAutorise;};
	void			SetChtVoieVersDroiteAutorise(bool b){m_bChtVoieVersDroiteAutorise = b;};

    const std::vector<Segment*> & GetSiraneCells() {return m_LstCellSirane;};

    virtual void    DiscretisationSirane();

protected:
    virtual void    SupprimeCellSirane();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#pragma warning(pop)

#endif
