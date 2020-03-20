#pragma once
#ifndef convergentH
#define convergentH

#include "ConnectionPonctuel.h"

#include "TempsCritique.h"

#include <list>

class Giratoire;
class TuyauMicro;
class Tuyau;
class VoieMicro;
struct MvtCAFEx;
class Vehicule;
class TypeVehicule;
class SensorsManager;
class PonctualSensor;
class Convergent;


// Structure de description d'un tronçon amont d'un convergent
struct DesVoieAmont
{    
    VoieMicro        *pVAm;
    int              nNivRef;        // Niveau de priorité de référence (sans logique priorité due au contrôleur de feux) : 1 = prioritaire (plus nNiveau augmente, plus la priorité diminue)
    int              nNiveau;        // Niveau de priorité pour le pas de temps considéré (dépend de la couleur du feu)    

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure de définition d'un point de conflit de type traversée du CAF
struct PtConflitTraversee
{
    PtConflitTraversee() {pT1=NULL; pT2=NULL; pTProp=NULL; mvt1=NULL; mvt2=NULL;}
    VoieMicro*      pT1;                   // Voie 1 en conflit
    VoieMicro*      pT2;                   // Voie 2 en conflit

    double          dbPos1;                 // Position du point de conflit sur le tronçon 1
    double          dbPos2;                 // Position du point de conflit sur le tronçon 2

    double          dbDst1;                 // Position à parcourir depuis l'entrée de CAF pour atteindre le point de conflit sur le mouvement 1
    double          dbDst2;                 // Position à parcourir depuis l'entrée de CAF pour atteindre le point de conflit sur le mouvement 1

    VoieMicro*      pTProp;                 // Voie prioritaire entre T1 et t2

    MvtCAFEx*       mvt1;
    MvtCAFEx*       mvt2;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure de définition d'un groupe de point de conflit de traversée (permet de regrouper les traversées non prioritaire d'un mouvement donné entre deux point d'attente)
struct GrpPtsConflitTraversee
{
    std::deque<boost::shared_ptr<PtConflitTraversee> > lstPtsConflitTraversee;

    VoieMicro*		pVPtAttente;         // Voie du point d'attente
    double          dbPosPtAttente;      // Position du point d'attente sur le tronçon ci-dessus
    double          dbDstPtAttente;      // Distance à parcourir pour atteindre le point d'attente à partir de l'entrée du véhicule dans le CAF

    double          dbInstLastTraversee;        // Instant de la dernière traversée
    std::deque<TempsCritique>   LstTf;     // Liste des temps de traversées

	int				nNbMaxVehAttente;		// Nombre max de véhicule en attente
	std::list<int>		listVehEnAttente;		// map des véhicules en attente

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Classe de modélisation d'un point de convergent
class PointDeConvergence
{    
public:
    std::deque<DesVoieAmont>         m_LstVAm;        // Liste des voies amont

    VoieMicro*                       m_pVAv;          // Voie aval
    TuyauMicro*                      m_pTAv;          // Tuyau aval

    double                      m_dbInstLastPassage;        // Instant exact du dernier franchissement du point de convergence    

    // Variables de simulation
private:
    Convergent*     m_pCvg;

    bool            m_bFreeFlow;                // Indique si le convergent est fluide en aval au pas de temps courant
    bool            m_bInsertion;               // Indique si l'insertion est posible au pas de temps courant

    int             m_nRandNumbers;             // Nombre de tirage aléatoire à effectuer lors d'une insertion en régime congestionné

public:
    PointDeConvergence();
    PointDeConvergence( VoieMicro *pVAv, Convergent *pCvg );
    ~PointDeConvergence(){};

    bool    IsInsertion(){return m_bInsertion;};
    bool    IsFreeFlow(){return m_bFreeFlow;};

    VoieMicro*   GetVoieAmontPrioritaire();

    void    SetInsertion(bool bInsertion){m_bInsertion = bInsertion;};

    void    InitPriorite(VoieMicro *pVAm, int nNiveau);
    int     GetPrioriteRef(VoieMicro *pVAm);
    int     GetPriorite(VoieMicro *pVAm);

    Convergent* GetCvg() {return m_pCvg;}

    void SetRandNumbers(int randNumbers)    {m_nRandNumbers = randNumbers;}
    int  GetRandNumbers()                   {return m_nRandNumbers;}

    void CalculInsertion(double dbInstant);

	std::string GetUnifiedID();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

class Convergent :  public ConnectionPonctuel
{
public:
   
    SensorsManager* m_pGestionCapteur;      // Gestion des capteurs du convergent

    double          m_dbGamma;    
    double          m_dbMu;

    std::deque<TempsCritique>   m_LstTf;             // Liste des temps d'insertion

    Giratoire*      m_pGiratoire;               // Pointeur sur le giratoire si le convergent appartient à un giratoire

    std::deque<PointDeConvergence*>  m_LstPtCvg;     // Liste des points de convergence

public:
    // Constructeurs, destructeurs et assimilés
    virtual ~Convergent(); // Destructeur
    Convergent() ; // Constructeur par défaut
	Convergent(std::string strID, double dbTagreg, double dbGamma, double dbMu, Reseau *pReseau);

    void    Init(Tuyau* pTAv);
    void    FinInit(Tuyau* pTAv,  double dbPosCptTAv);

    void    AddTf(TypeVehicule *pTV, double dbTf);
    double  GetTf(TypeVehicule *pTV);

    // Fonctions relatives à la simulation des convergents        
    void UpdateCapteurs();                   // Met à jour les données des capteurs

    PonctualSensor* GetCapteur(Tuyau *pT);   

    void        SetGiratoire(Giratoire *pGiratoire){m_pGiratoire = pGiratoire;};
    Giratoire*  GetGiratoire(){return m_pGiratoire;};

    //bool        IsConvergentEtendu(){return m_bConvergentEtendu;};

    boost::shared_ptr<Vehicule> GetVehiculeEnAttente(VoieMicro *pVAm);

    double      CalculVitesseLeader( Vehicule *pVehLeader, double dbInstant, double dbPasTemps);

    double      GetInstLastPassage(VoieMicro *pVoie);

    void        AddMouvement(VoieMicro* pVAm, VoieMicro* pVAv, int nPriorite =1);
    bool        IsVoieAmontNonPrioritaire(VoieMicro* pVAm);
    bool        IsVoieAmontPrioritaire(VoieMicro* pVAm);

    VoieMicro*  GetNextVoie(VoieMicro* pV);

    void        CalculInsertion(double dbInstant);

    PointDeConvergence*  GetPointDeConvergence(int nVoieAval);
	PointDeConvergence*  GetPointDeConvergence(VoieMicro* pVAm);

	void		InitSimuTrafic();

    virtual bool ComputeTraffic(boost::shared_ptr<Vehicule> pV, VoieMicro * pUpstreamLane,
                                double dbInstant, double dbPasTemps, double &dbMinimizedGoalDistance);

    virtual double GetAdditionalPenaltyForUpstreamLink(Tuyau* pTuyauAmont);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif
