#pragma once
#ifndef controleurDeFeuxH
#define controleurDeFeuxH
///////////////////////////////////////////////////////////
//  ControleurDeFeux.h
//  Implementation of the Class ControleurDeFeux
//  Created on:      23-oct.-2008 13:38:10
//  Original author: becarie
///////////////////////////////////////////////////////////

#include "tools.h"
#include "StaticElement.h"

#include <boost/shared_ptr.hpp>

class Tuyau;
class Trip;
class Reseau;
class Vehicule;

typedef enum { FEU_ROUGE = 0, FEU_VERT = 1, FEU_ORANGE = 2} eCouleurFeux;

/**
 * Classe de modélisation d'un contrôleur de feux
 */
struct CoupleEntreeSortie
{
    Tuyau*  pEntree;
    Tuyau*  pSortie;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure de défintion d'une Ligne Guidée Prioritaire du contrôleur de feux
struct LGP
{
    Tuyau* pTCptAm;             // Tronçon du capteur en amont du controleur de feu
	double dbPosCptAm;          // Position du capteur en amont du controleur de feu

    Tuyau* pTCptAv;             // Tronçon du capteur en aval du controleur de feu
	double dbPosCptAv;          // Position du capteur en aval du controleur de feu

	Tuyau* pTEntreeZone;        // Tronçon de la zone d'entrée du contrôleur
	double dbPosEntreeZone;     // Position de la zone d'entrée du contrôleur
	
	bool   bPrioriteTotale;     // Indique si la priorité est totale (ou partagée)
    int    nSeqPartagee;        // Si priorité partagée, indique la séquence partagée
	
    std::deque<Trip*> LstLTG;   // Liste des lignes LTG (lignes de transport guidés) concernées

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

/**
 * Classe de modélisation d'un signal actif d'une séquence : il est associé soit à un couple d'entrée / sortie
   soit à une ligne de VGP
 */
class SignalActif
{
public:
	SignalActif();
    SignalActif(Tuyau *pTE, Tuyau *pTS, double dbDureeVert, double dbDureeOrange, double dbRetardAllumage);

    virtual ~SignalActif(){}

private:    
    double      m_dbDureeRetardAllumage;        // durée du retard à l'allumage
    double      m_dbDureeVert;                  // durée du vert    
	double		m_dbDureeOrange;				// durée de l'orange

    Tuyau*      m_pTuyEntree;                   // Tronçon d'entrée concerné
    Tuyau*      m_pTuySortie;                   // Tronçon de sortie concerné

public:
    double      GetActivationDelay(){return m_dbDureeRetardAllumage;}
    double      GetGreenDuration(){return m_dbDureeVert;}
    void        SetGreenDuration(double dbDureeVert){m_dbDureeVert = dbDureeVert;}
    double      GetOrangeDuration(){return m_dbDureeOrange;}
    void        SetOrangeDuration(double dbDureeOrange){m_dbDureeOrange = dbDureeOrange;}
    Tuyau*      GetInputLink(){return m_pTuyEntree;}
    Tuyau*      GetOutputLink(){return m_pTuySortie;}

	eCouleurFeux GetCouleurFeux(double dbInstant);
    double      GetRapVertOrange(double dbInstant, double dbPasTemps, bool bVertOuOrange);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

/**
 * Classe de modélisation d'une séquence
 */
class CDFSequence
{
public:
    CDFSequence(){m_nNum=0;}
    CDFSequence(double dbDureeTotale, int nNum);
    ~CDFSequence();

private:
    double      m_dbDureeTotale;                // durée totale de la séquence
    int                         m_nNum;             // Numéro d'orde de la séquence dans le plan de feux associé

public:
	std::vector<SignalActif*>    m_LstSignActif;     // Liste des signaux actifs de la séquence

    std::vector<SignalActif*>    GetLstActiveSignals(){return m_LstSignActif;};

    double                  GetTotalLength(){return m_dbDureeTotale;};
    void                    SetTotalLength(double dbDuree){m_dbDureeTotale = dbDuree;};

    SignalActif*            GetSignalActif(Tuyau* pTuyEntree, Tuyau* pTuySortie);

    void                    AddActiveSignal(SignalActif *pS){m_LstSignActif.push_back(pS);};
    int                     GetNum(){return m_nNum;}
    void                    SetNum(int nNum){m_nNum = nNum;}

    double                  GetMinDuree(double dbDureeVertMin);

    double                  GetMinDureeVert();

    double                  GetInstMaxFeuxVerts();

    void                    Copy(CDFSequence *pSeqSrc);

    //! Retourne le moment du vert suivant dans la séquence en cours
    double GetInstantVertSuivant(double dInstant, Tuyau *pTuyauAmont, Tuyau * pTuyauAval );
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

/**
 * Classe de modélisation d'un plan de feux
 */
class PlanDeFeux
{
public:
    PlanDeFeux();
	PlanDeFeux(char *sID, STime tHrDeb);
    ~PlanDeFeux();

public:
	STime					m_tHrDeb;               // Heure de début du plan    

private:    
    std::string             m_strID;            // Identifiant unique du plan
    double                  m_dbDureeCycle;         // Durée en seconde du cycle de feu du plan        

    std::vector<CDFSequence*>  m_LstSequence;          // Liste ORDONNEE des séquences du plan

public:
	STime					GetStartTime(){return m_tHrDeb;}

    double                  GetCycleLength(){return m_dbDureeCycle;}
    void                    SetCycleLength(double dbDuree){m_dbDureeCycle = dbDuree;}
    
    CDFSequence*               GetSequence( double dbInstCycle, double &dbTpsPrevSeqs,  double &dbTpsCurSeq );

    std::vector<CDFSequence*>  GetLstSequences(){return m_LstSequence;}

    CDFSequence*               GetNextSequence(CDFSequence *pSeq);

    void                    AddSequence(CDFSequence *pSeq){ m_LstSequence.push_back(pSeq); m_dbDureeCycle+= pSeq->GetTotalLength(); }

    double                  GetDureeCumSeq(int nNum);

    void                    RemoveAllSequences(){m_LstSequence.clear(); m_dbDureeCycle = 0;}

    std::string             GetID(){return m_strID;}

	void                    SetHrDeb(STime tHrDeb){m_tHrDeb = tHrDeb;}

    void                    CopyTo(PlanDeFeux * pTarget);

	std::string				GetJsonDescription();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


class ControleurDeFeux : public StaticElement
{
public:
    ControleurDeFeux(){m_PDFActif=NULL;m_pReseau=NULL;m_pLstPlanDeFeux=NULL;m_pLstPlanDeFeuxInit=NULL;m_pSeqPrePriorite=NULL;m_pSeqRetour1=NULL;m_pSeqRetour2=NULL;};
    ControleurDeFeux(Reseau *pReseau, char *sID, double dbRougeDegagement, double dbVertMin);
    ~ControleurDeFeux();	 

private:
    std::string                              m_strID;                   // Identifiant unique du plan
    ListOfTimeVariationEx<PlanDeFeux>       *m_pLstPlanDeFeux;          // Liste des variantes temporelles des plans de feux
	ListOfTimeVariationEx<PlanDeFeux>       *m_pLstPlanDeFeuxInit;      // Liste des variantes temporelles des plans de feux
    Reseau                                  *m_pReseau;                 // Pointeur sur le réseau

    PlanDeFeux*                             m_PDFActif;                 // Plan de feux actif (si pilotage)

    double                                  m_dbDureeVertMin;           // durée minimum du vert (tous les signaux actifs doivent respecter ce temps de vert)
	double                                  m_dbDureeVertMinInit;       // durée minimum du vert (tous les signaux actifs doivent respecter ce temps de vert)
    double                                  m_dbDureeRougeDegagement;   // durée du rouge de dégagement (tous les signaux actifs doivent respecter ce rouge)
	double                                  m_dbDureeRougeDegagementInit;  // durée du rouge de dégagement (tous les signaux actifs doivent respecter ce rouge)

    std::vector<CoupleEntreeSortie>         m_LstCoupleEntreeSortie;    // Liste des couples entrée/sortie traité par le contrôleur de feux

    char                                    m_cModeFct;                 // Mode de fonctionnement du contrôleur (N : normal, O : pré-priorité, P : priorité, Q : post-priorité)

    std::deque<LGP*>                        m_LstLGP;                   // Liste des lignes guidées prioritaires du contrôleur

    std::deque<std::pair<boost::shared_ptr<Vehicule>,LGP*> > m_LstVGPs; // Liste des VGP en cours de traitement

    boost::shared_ptr<PlanDeFeux>           m_pPDFEx;                   // Plan de feu actif durant une phase de VGP prioritaire
    double                                  m_dbDebPDFEx;               // Début de l'application du plan de feux de la phase VGP prioritaire
    double                                  m_dbFinPDFEx;               // Fin de l'application du plan de feux de la phase VGP prioritaire (rteour au plan de feux normal)

    // Gestion du mode pre-priorite
    CDFSequence        *m_pSeqPrePriorite;                 // Séquence à appliquer
    CDFSequence        *m_pSeqRetour1;                     // Séquence de retour 1 après la sortie du CDF du VGP
    CDFSequence        *m_pSeqRetour2;                     // Séquence de retour 2 après la sortie du CDF du VGP
    int             m_nTypeGestionPrePriorite;          // Type de gestion ( 1: 'respect temps min', 2: 'séquence actuelle écourtée', 3 : 'prolongement séquence actuelle')

    // Cache des durées d'attente moyennes estimées par tronçon amont pour ne pas les recalculer sans arrêt (non sérialisée car pas besoin)
    std::map<Tuyau*, double> m_MeanWaitTimes;

    // Méthodes privées
private:
    // Détermine les tuyaux amont et aval du CAF controlé par lequel va arriver le véhicule guidé prioritaire passé en paramètre
    void GetTuyauAmAv(Vehicule * pBus, Tuyau ** pTuyAm, Tuyau ** pTuyAv);

    void UpdatePosition();

public:
    ListOfTimeVariationEx<PlanDeFeux>*  GetLstTrafficLightCycles(){return m_pLstPlanDeFeux;};
	ListOfTimeVariationEx<PlanDeFeux>*  GetLstPlanDeFeuxInit(){return m_pLstPlanDeFeuxInit;};

    void    AddCoupleEntreeSortie(Tuyau* pEntree, Tuyau* pSortie);

    std::vector<CoupleEntreeSortie>* GetLstCoupleEntreeSortie(){return &m_LstCoupleEntreeSortie;};

    std::string GetLabel(){return m_strID;};

	PlanDeFeux* GetTrafficLightCycle( double dbInstant, STimeSpan &tsTpsEcPlan );

    eCouleurFeux GetStatutCouleurFeux  (double dbInstant, Tuyau *pTE, Tuyau *pTS, bool *pbPremierInstCycle = NULL, double *pdbPropVertOrange = NULL, bool *pbPrioritaire = NULL);

    eCouleurFeux GetCouleurFeux (double dbInstant, Tuyau *pTE, Tuyau *pTS, bool *pbPremierInstCycle = NULL, double *pdbPropVertOrange = NULL, bool  *pbPrioritaire = NULL);    

    CDFSequence*   GetSequence     (double dbInstant, double &dbTpsPrevSeqs, double &dbTpsCurSeq, bool &bPrioritaire);

    void    Update(double dbInstant);

    void    AddLGP(LGP *plgp){m_LstLGP.push_back(plgp);}

    char    GetModeFct(){return m_cModeFct;}
    
    std::deque<LGP*>* GetLstLGP(){return &m_LstLGP;}

    void    SetDureeRougeDegagement(double dbDuree){m_dbDureeRougeDegagement = dbDuree;}
    void    SetDureeVertMin(double dbDuree){m_dbDureeVertMin = dbDuree;}

	double    GetDureeRougeDegagementInit(){return m_dbDureeRougeDegagementInit;}
	double    GetDureeVertMinInit(){return m_dbDureeVertMinInit;}

	PlanDeFeux* GetPlanDeFeuxFromID(const std::string & sID);

	int     SendSignalPlan(const std::string & SP);

	std::string GetUnifiedID() { return m_strID; }
    Reseau*    GetReseau() {return m_pReseau;}

    //! Obtient l'instant en second du début du prochain feu vert
    double GetInstantVertSuivant(double dRefTime,  double dInstantMax, Tuyau *pTuyauAmont, Tuyau * pTuyauAval );

    //! Obtient l'instant du début du vert courant
    double GetInstantDebutVert(double dRefTime, Tuyau *pTuyauAmont, Tuyau * pTuyauAval);

    //! Estime le temps d'attente moyen théorique pour un tronçon amont donné, en ne tenant compte que des plans de feux définis dans le scénario initial.
    double GetMeanWaitTime(Tuyau *pTuyauAmont);

    // Accesseurs pour sauvegarde de l'état en cas d'affectation dynamique convergente
    void SetPDFActif(PlanDeFeux * pPDF) {m_PDFActif = pPDF;}
    PlanDeFeux * GetPDFActif() {return m_PDFActif;}
    void SetModeFonctionnement(char cModeFcnt) {m_cModeFct = cModeFcnt;}
    std::deque<std::pair<boost::shared_ptr<Vehicule>,LGP*> > & GetLstVGPs() {return m_LstVGPs;}
    boost::shared_ptr<PlanDeFeux> GetPDFEx() {return m_pPDFEx;}
    void SetPDFEx(boost::shared_ptr<PlanDeFeux> pPDF) {m_pPDFEx = pPDF;}
    double GetDebutPDFEx() {return m_dbDebPDFEx;}
    void SetDebutPDFEx(double dbDebut) {m_dbDebPDFEx = dbDebut;}
    double GetFinPDFEx() {return m_dbFinPDFEx;}
    void SetFinPDFEx(double dbFin) {m_dbFinPDFEx = dbFin;}
    CDFSequence * GetSeqPrePriorite() {return m_pSeqPrePriorite;}
    void SetSeqPrePriorite(CDFSequence * pSeq) {m_pSeqPrePriorite = pSeq;}
    CDFSequence * GetSeqRetour1() {return m_pSeqRetour1;}
    void SetSeqRetour1(CDFSequence * pSeq) {m_pSeqRetour1 = pSeq;}
    CDFSequence * GetSeqRetour2() {return m_pSeqRetour2;}
    void SetSeqRetour2(CDFSequence * pSeq) {m_pSeqRetour2 = pSeq;}
    int GetTypeGestionPrePriorite() {return m_nTypeGestionPrePriorite;}
    void SetTypeGestionPrePriorite(int nTypeGestionPrePriorite) {m_nTypeGestionPrePriorite = nTypeGestionPrePriorite;}

	bool ReplaceSignalPlan(std::string strJsonSignalPlan);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
    friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif

