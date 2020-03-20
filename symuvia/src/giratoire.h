#pragma once
#ifndef giratoireH
#define giratoireH

#include "BriqueDeConnexion.h"

class TuyauMicro;
class Reseau;
class VoieMicro;
struct GrpPtsConflitTraversee;

// Structure permettant d'associer un couple de voies origine destination
// à un groupe de traversées correspondant
struct GroupeTraverseeEntreeGir {
    VoieMicro * pVoieEntree;
    VoieMicro * pVoieInterne;
    GrpPtsConflitTraversee * pGrpTra;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure permettant d'associer un couple voie interne - troncon de sortie
// à un groupe de traversées correspondant
struct GroupeTraverseeSortieGir {
    TuyauMicro*  pTuyauSortie;
    VoieMicro*   pVoieInterne;
    GrpPtsConflitTraversee * pGrpTra;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure de stockage des coefficients d'insertion définis manuellement
struct CoeffsInsertion {

    Tuyau * pTuyauEntree;
    Tuyau * pTuyauSortie;
    int nVoieEntree;
    std::vector<double> coefficients;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

// Structure de stockage du tirage d'un choix de voie d'insertion dans le giratoire
struct MouvementsInsertion {
    Tuyau * pTuyauSortie;
    Voie *  pVoieEntree;
    std::map<int, boost::shared_ptr<MouvementsSortie> > mvtsInsertion;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

class Giratoire : public BriqueDeConnexion
{
public:
    // Constructeurs, destructeurs et assimilés
    Giratoire() ; // Constructeur par défaut    
    Giratoire(char *strID, double dbVitMax, std::string strRevetement,  double dbTagreg, double dbGamma, double dbMu, int nVoie, double dbLargeurVoie, char cType, int nm, double dbBeta, double dbBetaInt, bool    bTraversees, Reseau *pReseau);
    ~Giratoire(); // Destructeur

private:

    // Variables caractéristiques du giratoire
    char        m_strNom[128];                  // Identifiant

    Reseau*     m_pReseau;

    std::string m_strRevetement;               // Revêtement de l'anneau

    double      m_dbTagreg;                     // Période d'agrégation
    double      m_dbGamma;
    double      m_dbMu;

    double      m_dbLargeurVoie;                // Largeur de la couronne
    int         m_nVoie;                        // Nombre de voie de l'anneau
    char        m_cType;                        // Type (urbain, péri-urbain, urbain-dense)
    
    double      m_dbBeta;                       // Probabilité pour un véhicule voulant s'insérer de détecter un véhicule sortant du giratoire
    double      m_dbBetaInt;                    // Probabilité pour un véhicule voulant s'insérer d'être géné par les véhicules en amont sur la voie interne en régime fluide
                                                // (uniquement dans le cas d'un giratoire multivoie)

    std::deque<TuyauMicro*>     m_LstTAmAv;     // Liste des tuyaux amont et aval du giratoire ordonnés dans le sens du parcours
    std::deque<TuyauMicro*>     m_LstTInt;      // Liste des tuyaux internes du giratoire

    std::deque<TempsCritique>  m_LstTf;            // Liste des temps d'insertion
    std::deque<TempsCritique>  m_LstTt;            // Liste des temps de traversée    

    std::vector<CoeffsInsertion>    m_LstCoeffsInsertion;   // Liste des coefficients d'insertion définis manuellement

    std::vector<GroupeTraverseeEntreeGir> m_LstGrpTraEntree;   // Liste des goupes de traversées pour l'entrée sur le giratoire
    std::vector<GroupeTraverseeSortieGir> m_LstGrpTraSortie;   // Liste des goupes de traversées pour la sortie du giratoire

    // Variables de simulation
public:

    void    AddTuyauAmAv(TuyauMicro* pT){m_LstTAmAv.push_back(pT);};
    void    AddTf(TypeVehicule *pTV, double dbTf, double dbTt);

    void Init();        // Initialisation du giratoire
    void FinInit();     // Fin d'initialisation du giratoire

    char*   GetLabel(){return m_strNom;};    
    double  GetBeta(){return m_dbBeta;};
    double  GetBetaInt(){return m_dbBetaInt;};

    int     GetNbVoie(){return m_nVoie;};    

    virtual bool    GetTuyauxInternesRestants( TuyauMicro *pTInt, Tuyau *pTAv, std::vector<Tuyau*> &dqTuyaux);
    virtual bool    GetTuyauxInternes( Tuyau *pTAm, Tuyau *pTAv, std::vector<Tuyau*> &dqTuyaux);
	virtual bool    GetTuyauxInternes( Tuyau *pTAm, int nVAm, Tuyau *pTAv, int nVAv, std::vector<Tuyau*> &dqTuyaux);
    virtual std::set<Tuyau*> GetAllTuyauxInternes( Tuyau *pTAm, Tuyau *pTAv );
    const std::deque<TuyauMicro*>& GetTuyauxInternes() {return m_LstTInt;}

    virtual void    CalculTraversee(Vehicule *pVeh, double dbInstant, std::vector<int> & vehiculeIDs);

    // Gestion de l'insertion sur les giratoires à plusieurs voies
    bool	GetCoeffsMouvementsInsertion(Vehicule * pVeh, Voie *pVAm, Tuyau *pTAv, std::map<int, boost::shared_ptr<MouvementsSortie> > &mapCoeffs);
    // Ajout de coefficients manuels (prioritaires sur les coefficients calculés par défaut)
    void    AddCoefficientInsertion(Tuyau * pTuyauEntree, int nVoieEntree, Tuyau * pTuyauSortie, const std::vector<double> & coeffs);

	// calcul particulier du barycentre pour les briques de type Giratoire
	virtual Point*	CalculBarycentre();

    virtual double GetAdditionalPenaltyForUpstreamLink(Tuyau* pTuyauAmont);

    // Fonctions relatives à la simulation des giratoires    

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif
