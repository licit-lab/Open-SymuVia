#pragma once
#ifndef BriqueDeConnexionH
#define BriqueDeConnexionH

#include "Connection.h"
#include "TempsCritique.h"

#include <boost/serialization/assume_abstract.hpp>

#include <deque>
#include <vector>

#ifdef USE_SYMUCORE
namespace SymuCore {
    class MacroType;
	class RobustTravelIndicatorsHelper;
}
#endif // USE_SYMUCORE

class Reseau;
class Tuyau;
class Vehicule;
struct PtConflitTraversee;
class Segment;


class BriqueDeConnexion : public Connexion
{
public:
    // Constructeurs, destructeurs et assimilés
    BriqueDeConnexion(); 
    BriqueDeConnexion(char *strID, Reseau *pReseau, char cType, bool bSimulationTraversees); // Constructeur par défaut        
    ~BriqueDeConnexion(); // Destructeur

protected:

    // Variables caractéristiques du giratoire
    char                m_cType;                        // Type de brique ( 'G' : giratoire , 'C' : carrefour à feux )
    
    double              m_dbVitMax;                     // Vitesse max à l'intérieur de la brique

	XERCES_CPP_NAMESPACE::DOMNode*  m_XmlNodeRepartitionDestination;			// Noeud XML d'affectation des sorties en fonction des destinations (saisie par l'utilisateur)

    Segment *           m_pCellSirane;                  // Cellule Sirane correspondant à la brique de connexion

    bool                m_bSimulationTraversees;        // active ou non la simulation des conflits de traversée pour la brique

public:
    // Variables de simulation
    std::deque<TempsCritique>   m_LstTfTra;                // Liste des temps d'insertion   (traversée)   
    std::deque<TempsCritique>   m_LstTfCvg;                // Liste des temps d'insertion   (convergent) 

    std::deque<boost::shared_ptr<PtConflitTraversee> > m_LstPtsConflitTraversee;   // Liste des points de conflit de type traversée

    std::map<std::pair<Tuyau*,Tuyau*>, std::map<int, std::pair< TypeVehicule*, std::pair<double,double> > > > m_mapVeh;	// Map des véhicules sortant du mouvement autorisé au cours de la période d'affectation courante
																					                                    // avec instant de sortie et durée de traversée du mouvement

    std::map<std::pair<Tuyau*,Tuyau*>, std::map<TypeVehicule*, double> >  m_LstCoutsMesures;    // Liste des couts mesurés des mouvements de la brique pour affectation

#ifdef USE_SYMUCORE
    virtual void CalculTempsParcours(double dbInstFinPeriode, SymuCore::MacroType * pMacroType, double dbPeriodDuration, bool bPollutantEmissionComputation);
    virtual double ComputeCost(SymuCore::MacroType * pMacroType, Tuyau * pUpstreamLink, Tuyau * pDownstreamLink);
    virtual double GetMarginal(SymuCore::MacroType* pMacroType, Tuyau* pTuyauAmont, Tuyau * pTuyauAval);
    virtual double GetMeanNbVehiclesForTravelTimeUpdatePeriod(SymuCore::MacroType * pMacroType, Tuyau* pTuyauAmont, Tuyau * pTuyauAval);

    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> > m_mapTTByMacroType;          // temps de parcours calculé pour SymuMaster par mouvement et par macro-type de véhicule
    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> > m_mapMarginalByMacroType;    // marginals calculé pour SymuMasyer par mouvment et par macro-type de véhicule

    std::map<std::pair<Tuyau*, Tuyau*>, std::map<SymuCore::MacroType*, double> > m_dbMeanNbVehiclesForTravelTimeUpdatePeriod;
    std::map<std::pair<Tuyau*, Tuyau*>, double>                                  m_dbTTForAllMacroTypes;
	std::map<std::pair<Tuyau*, Tuyau*>, double>                                  m_dbEmissionsForAllMacroTypes;
	std::map<std::pair<Tuyau*, Tuyau*>, SymuCore::RobustTravelIndicatorsHelper*>		 m_mapRobustTravelTimesHelper;		// map par mouvement de la structure utile pour calcul des temps de parcours robustes (et des marginals associées) sans tri par macro-type
	std::map<std::pair<Tuyau*, Tuyau*>, SymuCore::RobustTravelIndicatorsHelper*>		 m_mapRobustTravelSpeedsHelper;		// map par mouvement de la structure utile pour calcul des vitesses robustes (et des marginals associées) sans tri par macro-type
#endif

public:  
    char    GetType(){return m_cType;};

    virtual bool    GetTuyauxInternes( Tuyau *pTAm, Tuyau *pTAv, std::vector<Tuyau*> &dqTuyaux) = 0;
	virtual bool    GetTuyauxInternes( Tuyau *pTAm, int nVAm, Tuyau *pTAv, int nVAv, std::vector<Tuyau*> &dqTuyaux) = 0;
    virtual bool    GetTuyauxInternesRestants( TuyauMicro *pTInt, Tuyau *pTAv, std::vector<Tuyau*> &dqTuyaux) = 0;
    virtual std::set<Tuyau*> GetAllTuyauxInternes( Tuyau *pTAm, Tuyau *pTAv ) = 0;

    double      GetVitMax(){return m_dbVitMax;};

    void                        AddTfTra(TypeVehicule *pTV, double dbTf);    
    double                      GetTfTra( TypeVehicule *pTV );

    void                        AddTfCvg(TypeVehicule *pTV, double dbTf);    
    double                      GetTfCvg( TypeVehicule *pTV );

    Segment*                    GetCellSirane() {return m_pCellSirane;};

    virtual void    CalculTraversee(Vehicule *pVeh, double dbInstant, std::vector<int> & vehiculeIDs)=0;
    
    virtual void    InitSimulationSirane();
	virtual Point*	CalculBarycentre();

    //! Calcule le cout d'un mouvement autorisé
    virtual double ComputeCost(TypeVehicule* pTypeVeh, Tuyau* pTuyauAmont, Tuyau * pTuyauAval);
    virtual double ComputeEmptyCost(TypeVehicule* pTypeVeh, Tuyau* pTuyauAmont, Tuyau * pTuyauAval);

    // Méthode similaire à celle de nom identique pour les tronçons, mais s'applique ici aux mouvements des briques
    virtual void CalculTempsParcours();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(BriqueDeConnexion)

#endif
