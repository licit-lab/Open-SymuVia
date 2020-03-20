#pragma once
#ifndef SymuViaTripNodeH
#define SymuViaTripNodeH

#include "usage/TripNode.h"
#include "tools.h"
#include "MesoNode.h"

#include <boost/shared_ptr.hpp>
#include <boost/serialization/assume_abstract.hpp>

#include <deque>
#include <string>
#include <vector>
#include <map>

namespace boost {
    namespace serialization {
        class access;
    }
}

class Vehicule;
class Tuyau;
class Connexion;
class TypeVehicule;
struct AssignmentData;
class SymuViaTripNode;
class RepartitionTypeVehicule;
class CRepMotif;
class CMotifCoeff;
class CPlaque;

struct CreationVehicule
{
public:
    CreationVehicule() {dbInstantCreation = 0.0; pTypeVehicule = NULL; pDest = NULL; nVoie = 1;}
    double dbInstantCreation;
    TypeVehicule * pTypeVehicule;
    SymuViaTripNode * pDest;
    int nVoie;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


//***************************************************************************//
//                          class RepartitionEntree                      //
//***************************************************************************//
class RepartitionEntree
{
public:

    std::vector<double> pCoefficients;      // Liste des coefficients ordonnés de la répartion par voie    

	RepartitionEntree(){}
	~RepartitionEntree(){}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


//***************************************************************************//
//                          class VectDest                               //
//***************************************************************************//
// Permet de stocker le vecteur destination (liste des coefficients de répartition > 0 
// pour chaque destination)
class VectDest
{
public:
    SymuViaTripNode*    pDest;                                                  // Destination
    double          dbCoeff;                                                // Coefficient de répartition vers la destination
    bool            bHasTetaLogit;                                          // Indique si une valeur particulière du teta logit est précisée
    double          dbTetaLogit;                                            // Valeur particulière du teta logit
    bool            bHasNbPlusCourtsChemins;                                // Indique si une valeur particulière du nombre de plus courts chemins à calculer
    int             nNbPlusCourtsChemins;                                   // Valeur particulière du nombre de plus courts chemins à calculer
    bool            bHasCustomCommonalityParameters;                        // Indique si des paramètres particuliers sont définis pour le calcul du commonality factor
    double          dbCommonalityAlpha;
    double          dbCommonalityBeta;
    double          dbCommonalityGamma;
    std::vector<std::pair<double, std::pair<std::pair<std::vector<Tuyau*>, Connexion*>, std::string> > > lstItineraires;    // Liste des itinéraires et routes id prédéfinis pour cette destination

    std::vector<std::string>                            lstRouteNames;     // Liste des identifiants de routes correspondants aux itinéraires prédéfinis
    double          dbRelicatCoeff;                                         // relicat de coefficient non affecté à un itinéraire prédéfini

	VectDest(){pDest=NULL;dbCoeff=0;bHasTetaLogit=false;dbTetaLogit=0;bHasNbPlusCourtsChemins=false;nNbPlusCourtsChemins=0;dbRelicatCoeff=1;bHasCustomCommonalityParameters=false;
               dbCommonalityAlpha=0;dbCommonalityBeta=0;dbCommonalityGamma=0;}
	~VectDest(){};

    bool operator==(const VectDest& rhs)const;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

//***************************************************************************//
//                          class SimMatOrigDest                         //
//***************************************************************************//
// Permet de stocker la matrice des origines destination pour une durée donnée 
class SimMatOrigDest
{    
public:
    std::deque<VectDest*>       MatOrigDest;                 // Tableau des listes des vecteurs destination   

	SimMatOrigDest(){};
	~SimMatOrigDest()
	{
		std::deque<VectDest*>::iterator itM;
		for( itM = MatOrigDest.begin(); itM != MatOrigDest.end(); itM++ )
		{
			if( (*itM) )
				delete (*itM);
			(*itM) = NULL;
		}
		MatOrigDest.clear();
	};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

class SymuViaTripNode : public TripNode
{
public:

    // ************************************
    // Constructeurs/Destructeur
    // ************************************
    SymuViaTripNode();
    SymuViaTripNode(const std::string & strID, Reseau * pNetwork);
    virtual ~SymuViaTripNode();

    // ************************************
    // Traitements publics
    // ************************************

    // Initialisation des variables
    virtual void Initialize(std::deque< TimeVariation<TraceDocTrafic> > & docTrafics);

    // Création des véhicules
    virtual void CreationVehicules(const double dbInst, std::vector<boost::shared_ptr<Vehicule>> & listInfoVeh);

    
    // Connexions associées, ou "équivalentes" dans le cas des zones de terminaison
    virtual Connexion* GetInputConnexion();
    virtual Connexion* GetOutputConnexion();

    // Retourne la liste des itinéraires et leurs coefficients d'affectation pour le couple OD défini et le type de véhicule
    bool GetItineraires(TypeVehicule *pTV, SymuViaTripNode *pDst, std::deque<AssignmentData> &dqAssData);	

    virtual std::map<TypeVehicule*, std::map< std::pair<Tuyau*, SymuViaTripNode*>, std::deque<AssignmentData> > > & GetMapAssignment();

    bool	AddEltAssignment(TypeVehicule *pTV, Tuyau *pTAm, SymuViaTripNode* pS, const std::vector<Tuyau*> & Ts, double dbCout, double dbCoutPenalise, double dbCommonalityFactor, bool bPredefini, const std::string & strRouteId, Connexion * pJunction);


    // Calcule la destination
    SymuViaTripNode* GetDestination(double  dbTime, int nVoie, TypeVehicule * pTypeVeh);

    std::deque<SymuViaTripNode*>* GetLstDestinations(){return &m_LstDestinations;}
    void AddDestinations(SymuViaTripNode* pDst);
    
    void AddCreationVehicule(double dbInstant, TypeVehicule * pTV, SymuViaTripNode * pDest, int nVoie);
    
    // Gestion du log de la matrice OD de l'origine
    int                 GetLogMatriceODSize(){return (int)m_LogMatriceOD.size();};
    int                 GetLogMatriceOD(int i,int j);

    int                 GetMatriceODInitSize(bool bIsUsedODMatrix);
    void                InitLogMatriceOD(bool bIsUsedODMatrix);
    void                MAJLogMatriceOD(bool bIsUsedODMatrix, double dbInstCreation, SymuViaTripNode *pDest);
    void                DeleteLogMatriceOD();

    // Gestion de la création des véhicules
	virtual void	    UpdateCritCreationVeh(double dbInst, bool bDueToCreation);   // Mise à jour du critère de création des véhicules

    // Création arbitraire de véhicules
    boost::shared_ptr<Vehicule> GenerateVehicle(int nID, TypeVehicule *pTV, int nVoie, double dbInst, double dbInstCreat, SymuViaTripNode *pDst, std::vector<Tuyau*> * pIti, Connexion * pJunction, CPlaque * pPlaqueOrigin, CPlaque * pPlaqueDestination, bool bForceNonResidential); // Création effective d'un véhicule

    // Gestion de l'agressivité
    void				AddAgressivite(TypeVehicule *pTV, double dbVal){ m_mapTauxAgressivite.insert( std::pair<TypeVehicule*, double>(pTV, dbVal));};
	bool				IsAgressif(TypeVehicule *pTV);

    void				InsertionVehEnAttente(double dbInst);						// Module d'insertion sur le réseau des véhicules en attente
	bool				InsertionVehEnAttenteMeso(double dbInst, Vehicule * pVeh);	// Module d'insertion sur le réseau des véhicules en attente pour le mésoscopique
    // retourne vrai si le véhicule à été inséré

    void				FinSimuTrafic();

    // Indique si on peut créer des véhicules à cette origine (par défaut, on peut toujours)
    virtual bool        IsVehiculeDisponible() {return true;}

    // Peut-on créer un véhicule au vu de l'instant courant et du dernier instant d'insertion ?
    virtual bool        CanInsertVehicle(double dbInst, double dbLastInstantInsertion) {return true;}

    // Gestion la valeur de la repartition des voies pour l'origine considérée
    std::vector<double> GetRepVoieValue(double dbInst);

    // Renvoie la valeur de la demande pour l'origine considérée
    virtual double      GetDemandeValue(double dbInst, double & dbVariationEndTime);

    // teste si un véhicule est susceptible de rallier cette origine à la destination passée en paramètre
    bool                IsLinkedToDestination(SymuViaTripNode * pDest, TypeVehicule * pTV, double dbStartTime, double dbEndTime);

    void	            InitAssignmentPeriod(TypeVehicule *pTV);


    std::map<TypeVehicule*, std::deque<TimeVariation<SimMatOrigDest> > >&      GetLstCoeffDest(){return m_LstCoeffDest;}
	std::map<TypeVehicule*, std::deque<TimeVariation<SimMatOrigDest> > >&      GetLstCoeffDestInit(){return m_LstCoeffDestInit;}
    std::deque<TimeVariation<SimMatOrigDest>>&      GetLstCoeffDest(TypeVehicule * pTypeVeh){return m_LstCoeffDest[pTypeVeh];}
	std::deque<TimeVariation<SimMatOrigDest>>&      GetLstCoeffDestInit(TypeVehicule * pTypeVeh){return m_LstCoeffDestInit[pTypeVeh];}

    
    // Accès aux listes des variantes
    ListOfTimeVariation<tracked_double>*    GetLstDemande(TypeVehicule * pTypeVeh);
	ListOfTimeVariation<tracked_double>*    GetLstDemandeInit(TypeVehicule *pTypeVeh);
    std::map<TypeVehicule*, ListOfTimeVariation<tracked_double>* > GetLstDemandeInit() const ;
    std::map<TypeVehicule*, ListOfTimeVariation<tracked_double>* > GetLstDemande() const ;

    std::deque<TimeVariation<RepartitionEntree>>*   GetLstRepVoie(TypeVehicule * pTypeVehicle);
	std::deque<TimeVariation<RepartitionEntree>>*   GetLstRepVoieInit(TypeVehicule * pTypeVehicle);
    std::map<TypeVehicule*, std::deque<TimeVariation<RepartitionEntree> >* >  GetLstRepVoieInit() const;
    std::map<TypeVehicule*, std::deque<TimeVariation<RepartitionEntree> >* >  GetLstRepVoie() const;

    std::deque<TimeVariation<CRepMotif>>&      GetLstRepMotif(TypeVehicule * pTypeVeh){ return m_lstRepMotif[pTypeVeh]; }
    std::deque<TimeVariation<CRepMotif>>&      GetLstRepMotifInit(TypeVehicule * pTypeVeh){ return m_lstRepMotifInit[pTypeVeh]; }

    void   SetLstCoeffDestInit(SymuViaTripNode * pDest, TypeVehicule * pTypeVeh, const std::vector<std::pair<double, std::pair<std::vector<Tuyau*>, Connexion*> > > & routes);

    void   SetTypeDemande(bool bDemande){m_bDemande = bDemande;}
    bool   IsTypeDemande(){return m_bDemande;}

    void   SetTypeDistribution(bool bDistri){m_bDistribution = bDistri;}
    bool   IsTypeDistribution(){return m_bDistribution;}  

    int     GetNbVariations(){return Nb_variations;}
    int     GetNbVehicule(int nVar){return m_dqNbVehicules[nVar];}

    RepartitionTypeVehicule* GetLstRepTypeVeh(){return m_pLstRepTypeVeh;}

    TypeVehicule*   CalculTypeNewVehicule(double dbInstant, int nVoie);
    int             CalculNumVoie(double dbInstant, Tuyau * pOutputLink = NULL);

    void CopyDemandeInitToDemande(std::deque<TypeVehicule *> typeVehicles); 
    void CopyCoeffDestInitToCoeffDest(std::deque<TypeVehicule *> typeVehicles);
    void CopyRepVoieInitToRepVoie(std::deque<TypeVehicule *> typeVehicles);
    void CopyRepMotifDestInitToRepMotifDest(std::deque<TypeVehicule *> typeVehicles);

    // Calcule le motif d'un déplacement
	CMotifCoeff* GetMotif(double  dbTime, TypeVehicule * pTypeVeh, SymuViaTripNode * pDest, CPlaque * pPlaqueOrigin = NULL, CPlaque * pPlaqueDestination = NULL);

    std::deque<CreationVehicule> & GetLstCreationsVehicule() {return m_LstCreationsVehicule;}

    void AddCreationFromStationnement(double dbInstantCreation);

    std::vector<double> & GetLstSortiesStationnement() { return m_lstSortiesStationnement; }

    int SetLstRepVoie(TypeVehicule* pTypeVeh, const std::vector<double> & coeffs);

	std::map<TypeVehicule*, std::map<SymuViaTripNode*, std::vector< std::pair<double, std::vector<Tuyau*> > > > > & GetMapDrivenPaths() { return m_mapDrivenPaths; }

    // ************************************
    // Champs publics
    // ************************************
public:

    // Variables de simulation de l'entrée
	std::map<Tuyau*, std::map<int, std::deque<boost::shared_ptr<Vehicule>>>>  m_mapVehEnAttente;			// Map de la liste des véhicules en attente par voie (numéro de voie, pointeur sur un objet véhicule)
	std::map<Tuyau*, std::map<int, int > >		 				              m_mapVehPret;				// Map du véhicule pret à partir par voie (numéro de voie, identifiant du véhicule)
    double            			                                              m_dbCritCreationVeh;		// critère de création des véhicule pour l'instant courant
																		// le critère peut s'exprimer sous forme d'un instant à atteindre ou d'un nombre (<=1) de véhicule 

    // ************************************
    // Champs privés
    // ************************************
protected:

    // Liste des variantes des répartitions des destinations
    std::map<TypeVehicule*, std::deque<TimeVariation<SimMatOrigDest> > >   m_LstCoeffDest;
	std::map<TypeVehicule*, std::deque<TimeVariation<SimMatOrigDest> > >   m_LstCoeffDestInit;

    RepartitionTypeVehicule*        m_pLstRepTypeVeh;   // Liste des variantes des répartitions des types de véhicule à la création (uniquement pour un flux global)

    std::deque<SymuViaTripNode*>    m_LstDestinations;        // Liste des destinations possibles pour cette 
    std::deque<int*>                m_LogMatriceOD;           // Permet de tracer les destinations des véhicules créés

    std::map<TypeVehicule*, double> m_mapTauxAgressivite;	// Map des taux d'agressivité par classe de véhicule

     // Liste des variantes des demandes        
    std::map<TypeVehicule*, ListOfTimeVariation<tracked_double> *>      m_LTVDemandeInit;		// Liste initiale des demandes
	std::map<TypeVehicule* , ListOfTimeVariation<tracked_double> *>     m_LTVDemande;			// Liste en cours des demandes

    // Liste des variantes des répartitions des véhicules par voie
    std::map<TypeVehicule*, std::deque<TimeVariation<RepartitionEntree> >* >               m_lstRepVoie;            
	std::map<TypeVehicule*, std::deque<TimeVariation<RepartitionEntree> >* >               m_lstRepVoieInit;

	int                 Nb_variations;          // Nb de variations de la demande durant la simulation
    std::deque<int>     m_dqNbVehicules;        // Liste du nombre de véhicules créés durant chaque variation

    bool                m_bDemande;             // Indique si les véhicules sont générés à l'aide d'une demande ou par une liste de véhicule (demande par défaut)
    bool                m_bDistribution;        // Indique si les véhicules sont générés à l'aide d'une distribution exponentielle

    double              m_dbLastInstCreation;   // Dernier instant de création d'un véhicule à cette origine

    // Liste des création de véhicules à effectuer (mode de création par liste de véhicule prédéfinie)
    std::deque<CreationVehicule>    m_LstCreationsVehicule;
    TypeVehicule*                   m_pTypeVehiculeACreer;
    SymuViaTripNode*                m_pDestinationVehiculeACreer;
    int                             m_nVoieACreer;

    // Liste des variantes des distributions de motifs
    std::map<TypeVehicule*, std::deque<TimeVariation<CRepMotif> > >               m_lstRepMotif;
    std::map<TypeVehicule*, std::deque<TimeVariation<CRepMotif> > >               m_lstRepMotifInit;

    // Liste des instants de sortie de stationnement de véhicule, dans le cas où on est dans le mode de génération
    // automatique d'une demande liée au stationnement non résidentiel en zone.
    std::vector<double>                                                           m_lstSortiesStationnement;

	// Chemins pilotés de façon externe via SymSetODPaths()
	std::map<TypeVehicule*, std::map<SymuViaTripNode*, std::vector< std::pair<double, std::vector<Tuyau*> > > > > m_mapDrivenPaths;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(SymuViaTripNode) // la classe est pure virtual

#endif // SymuViaTripNodeH
