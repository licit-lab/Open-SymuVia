#pragma once
#ifndef CarrefourAFeuxExH
#define CarrefourAFeuxExH

#include "BriqueDeConnexion.h"

#include <deque>

class TuyauMicro;
class VoieMicro;
class Reseau;
class Convergent;
struct GrpPtsConflitTraversee;

// Structure de description d'une entrée du CAF
struct EntreeCAFEx
{
    TuyauMicro*         pTAm;               // Tronçon
    int                 nVoie;              // Numéro de voie

    int                 nPriorite;          // Type de priorité ( 0 : par défaut = priorité à droite, 1 : cédez-le passage, 2 : stop)

    std::deque<TuyauMicro*> lstTBr;         // Liste ordonnée des tronçons internes de la branche

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure de description d'une sortie du CAF
struct SortieCAFEx
{
    TuyauMicro*         pTAv;               // Tronçon
    int                 nVoie;              // Numéro de voie

    std::deque<TuyauMicro*> lstTBr;         // Liste ordonnée des tronçons internes de la branche

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure de description d'un mouvement du CAF
struct MvtCAFEx
{
    MvtCAFEx() {ent=NULL;sor=NULL;}
    EntreeCAFEx*             ent;
    SortieCAFEx*             sor;

    std::deque<TuyauMicro*> lstTMvt;                                    // Liste ordonnée des tronçons internes du mouvement
    std::deque<GrpPtsConflitTraversee*> lstGrpPtsConflitTraversee;      // Liste ordonnée des groupes de points de conflit traversée
    std::deque<double> lstPtAttente;                                    // Liste ordonnées des positions points d'attente du mouvement
	std::deque<int>	   lstNbVehMax;                                     // Liste ordonnées des nombre de veh. max
    boost::shared_ptr<MouvementsSortie> pMvtSortie;                     // Liste des types de véhicules exclus du mouvement

    bool IsTypeExclu(TypeVehicule* pTV, SousTypeVehicule * pSousType) const {return pMvtSortie->IsTypeExclu(pTV, pSousType);}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

class CarrefourAFeuxEx : public BriqueDeConnexion
{
public:
    // Constructeurs, destructeurs et assimilés
    CarrefourAFeuxEx(){}; // Constructeur par défaut    
    CarrefourAFeuxEx(char *strID, double dbVitMax, double dbTagreg, double dbGamma, double dbMu , 
                    double  dbLong, bool    bTraversees, Reseau *pReseau);
    ~CarrefourAFeuxEx(); // Destructeur

public:

    // Variables caractéristiques du carrefour à feux
    std::deque<EntreeCAFEx*>        m_LstEntrees;           // Liste des entrées
    std::deque<SortieCAFEx*>        m_LstSorties;           // Liste des sorties
    std::deque<EntreeCAFEx*>        m_LstEntreesBackup;     // Liste des entrées sauvegardées
    std::deque<SortieCAFEx*>        m_LstSortiesBackup;     // Liste des sorties sauvegardées
    std::deque<MvtCAFEx*>           m_LstMouvements;        // Liste des mouvements
    std::deque<MvtCAFEx*>           m_LstMouvementsBackup;  // Liste des anciens mouvements conservés en mémoire
    std::deque<Convergent*>         m_LstCvgs;              // Liste des convergents du CAF

private:

    double      m_dbTagreg;                         // Période d'agrégation
    double      m_dbGamma;
    double      m_dbMu;
    
    double      m_dbLongueurCellAcoustique;         // Longueur désirée des cellules acoustiques    

    // Variables de simulation
public:

    EntreeCAFEx*     AddEntree(Tuyau*  pTAm, int nVAm);
    SortieCAFEx*     AddSortie(Tuyau*  pTAv, int nVAv);
    MvtCAFEx*        AddMouvement(TuyauMicro*  pTAm, int nVAm, TuyauMicro*  pTAv, int nVAv, boost::shared_ptr<MouvementsSortie> pMvtSortie);    
    
	bool    Init( XERCES_CPP_NAMESPACE::DOMNode *pXmlNodeCAF, Logger *pChargement);    // Initialisation du carrefour à feux   
    void    FinInit(Logger *pChargement);                                             // etape post initialisation
    bool    ReInit( XERCES_CPP_NAMESPACE::DOMNode *pXmlNodeCAF, Logger *pChargement);  // MAJ du carrefour à feux   
    
    void    UpdateConvergents(double dbInstant);

    std::deque<EntreeCAFEx*> GetLstEntree(){return m_LstEntrees;};

    virtual bool    GetTuyauxInternes( Tuyau *pTAm, Tuyau *pTAv, std::vector<Tuyau*> &dqTuyaux);
    virtual bool    GetTuyauxInternes( Tuyau *pTAm, int nVAm, Tuyau *pTAv, int nVAv, std::vector<Tuyau*> &dqTuyaux);
    virtual bool    GetTuyauxInternesRestants( TuyauMicro *pTInt, Tuyau *pTAv, std::vector<Tuyau*> &dqTuyaux);
    virtual std::set<Tuyau*> GetAllTuyauxInternes( Tuyau *pTAm, Tuyau *pTAv );

    VoieMicro*      GetNextTroncon(TuyauMicro *pTAm, int nVAm, TuyauMicro* pTAv, int nVAv, TuyauMicro *pTI = NULL);
	VoieMicro*      GetNextTroncon(TuyauMicro* pTAv, int nVAv, TuyauMicro *pTI);
    int             GetVoieAmont( Tuyau* pTInterne, Tuyau* pTuyau, TypeVehicule * pTV, SousTypeVehicule * pSousType);
    VoieMicro*      GetNextVoieBackup( Tuyau* pTInterne, Tuyau* pTuyau, TypeVehicule * pTV, SousTypeVehicule * pSousType);
    
    virtual void    CalculTraversee(Vehicule *pVeh, double dbInstant, std::vector<int> & vehiculeIDs);

    void            UpdatePrioriteTraversees(double dbInstant);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif
