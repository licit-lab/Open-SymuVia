#pragma once
#ifndef SegmentNewH
#define SegmentNewH

#include "loi_emission.h"
#include "troncon.h"
#include "XMLDocSirane.h"

class SegmentMacro;
class Frontiere;
class CDiagrammeFondamental;

//***************************************************************************//
//                          structure Voisin                                 //
//***************************************************************************//
struct Voisin
{
    SegmentMacro    *pSegment;       // Pointeur sur le segment voisin
    double          dbDebit;        // Débit injecté par le segment courant vers le segment voisin
    double          dbDebitEqui;    // Débit injecté par le segment courant vers le segment voisin lors d'unaclaul de débit par phase (traversée de DM)
    double          dbDemande;      // Demande du segment courant
    double          dbDebitInjecteCumule; // Débit injecté cumulé (utile pour le calcul de la vitesse pour le processus déterministe)
    double          dbDebitAnt;     // Débit injecté au cours du pas de temps précédent

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

//***************************************************************************//
//                          classe Segment                                   //
//***************************************************************************//
class Segment : public Troncon
{
public:
      Segment();
      Segment(int   nID, double dbVitReg);
      ~Segment();

        // Variables relatives au calcul des émissions acoustiques du segment        
        double* Lw_cellule;     // tableau contenant le niveau de puissance acoustique de la cellule en global et pour les 8 octaves
        double* W_cellule;      // tableau contenant la puissance acoustique de la cellule en global et pour les 8 octaves

        std::map<TypeVehicule*, std::vector<double> > m_LstCumulsSirane;  // liste des durées pour chaque type de véhicule et chaque plage de vitesse

        int     m_nID;          // Identifiant unique du segment pour tout le réseau        
public:
        // Fonctions de calcul des émissions acoustiques
        virtual void Calcul_emission_cellule(Loi_emission*, char*){};   // Calcul de l'émission acoustique d'une cellule (p sur la l'objet loi d'émission, p sur l'objet rapport de boîte, type de revêtement du tronçon)
        virtual void Calcul_emission_vehicule(Loi_emission*, char*){};  // Calcul de l'émission acoustique (puissance) d'un véhicule de la cellule (p sur la l'objet loi d'émission, p sur l'objet rapport de boîte, type de revêtement du tronçon)

        void EmissionOutput(); // Fonction de restitutions des emissions dans les fichiers texte et dans les matrices pour le fichier .mat

		virtual bool    InitSimulation(bool bAcou, bool bSirane, std::string strName);
        virtual void    InitSimulationSirane(ePhaseCelluleSirane iPhase);
        virtual void    ComputeTraffic(double dbInstant){};
        virtual void    ComputeTrafficEx(double dbInstant){};
        virtual void    TrafficOutput(){};
        virtual void    InitAcousticVariables();

        void            AddPuissanceAcoustique(double* dbPuissAcoustique);

        int             GetID(){return m_nID;};

        // Gestion des cellules pour Sirane
        virtual void    AddSpeedContribution(TypeVehicule * pTypeVeh, double dbVitStart, double dbVitEnd, double dbPasDeTemps);
        virtual void    SortieSirane();
        virtual void    InitVariablesSirane();

protected:
        virtual void    GetSpeedBounds(int nPlage, double& dbVitMin, double& dbVitMax);
        virtual int     GetSpeedRange(double dbVit);
        virtual void    AddSpeedContribution(TypeVehicule * pType, int nPlage, double  dbDuree);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

//***************************************************************************//
//                          classe SegmentMacro                              //
//***************************************************************************//

// La classe segment correpond aux élements resultant du processus de discrétisation
// des tronçons macroscopiques. Les segments contiennent toutes les informations trafic simulés dynamiquement
// par le modèle de trafic.

class SegmentMacro : public Segment
{
private:
        // Variables caractéristiques des segments
        int     Nb_Voies;                   // Nb de voies


protected :

        // Variables descriptives des segments
        static const double EPSILON_01;

        SegmentMacro *cell_aval; // pointeur vers segment aval au segment actuel
        SegmentMacro *cell_amont; // pointeur vers segment amont au segment actuel

        Frontiere   *m_pFrontiereAmont;  // Pointeur sur la frontière amont
        Frontiere   *m_pFrontiereAval;   // Pointeur sur la frontière aval

        double  m_dbConc;           // Concentration dans le segment
        double  m_dbDebit;          // Débit dans le segment (TEMPORAIRE ???)

        // Gestion du voisinage
        std::deque<Voisin>   m_LstVoisinsAmont;      // Liste des voisins amont
        std::deque<Voisin>   m_LstVoisinsAval;       // Liste des voisins aval

public:
        // Constructeurs, destructeurs et associés
        ~SegmentMacro(void) ;
		SegmentMacro(std::string sLabel, double _pasespace, double _pastemps, int _nbVoies, Frontiere *pFAmont, Frontiere *pFAval, int &nLastID, double dbVitReg);
                // (Nom du segment, Pasespace, pastemps, NbVoies, type diagramme fluide (PL), type diagramme congestionné)
        SegmentMacro(void);

        void InitVarSimu();

        // Fonction d'affectation des variables
        void setAmont(SegmentMacro *); // (p sur segment amont)
        void setAval(SegmentMacro *); // (p sur segment aval)

        // Fonctions de renvoi d'informations sur les segments
        double getVitesse(void);
        double getVitesseMax(void);
        double getVitesseEntree();
        double getConc(void);
        double getConcAnt(void);
        double getDemande(void);
        double getDemande(double dbConc);
        double GetOffre(bool bOffreModifie, double dbConc);
        double getNbVoie();
        SegmentMacro* getSegmentAmont(void);
        SegmentMacro* getSegmentAval(void);


        // Fonctions utilitaires
        void TrafficOutput();        

        // Fonction de gestion du voisinage
        void    AddVoisinAmont (SegmentMacro *pSegment);
        void    AddVoisinAval  (SegmentMacro *pSegment, double dbDebit);

        double  GetDebitInjecteTo(SegmentMacro *pSegment, bool bAnt = false);
        void    SetDebitInjecteTo(SegmentMacro *pSegment, double dbDebit, bool bMAJAnt = true);

        double  GetDebitEquiTo(SegmentMacro *pSegment);
        void    SetDebitEquiTo(SegmentMacro *pSegment, double dbDebit, bool bAdd = false);

        double  GetDebitRecuFrom(SegmentMacro *pSegment, bool bAnt = false);

        double  GetSommeDebitInjecte();
        double  GetSommeDebitRecu(bool bAnt = false);

        double  GetDemandeTo(SegmentMacro *pSegment);
        void    SetDemandeTo(SegmentMacro *pSegment, double dbDemande);
        double  GetDemandeFrom(SegmentMacro  *pSegment);

        double  GetDebitInjecteCumuleTo(SegmentMacro *pSegment);
        void    SetDebitInjecteCumuleTo(SegmentMacro *pSegment, double dbDebit);

        double  GetDebitCumuleRecuFrom(SegmentMacro *pSegment);

        CDiagrammeFondamental*  GetDiagFonda();

        // Gestion des DMF
//        void    MiseAJourListeDMF(Tuyau *Tuyau, std::deque<DiscontinuiteMobileFictive*>*  pLstDMF);
//        void    AddDMFGestionFeux(Tuyau *Tuyau, std::deque<DiscontinuiteMobileFictive*>*  pLstDMF, double dbInstant);
        void    CalculVitessePositionDMF();

        std::deque<Voisin>   GetLstVoisinsAval(){return m_LstVoisinsAval;};
        std::deque<Voisin>   GetLstVoisinsAmont(){return m_LstVoisinsAmont;};

    virtual void    ComputeTraffic        (double dbInstant){};
    virtual void    ComputeTrafficEx(double dbInstant);

        double GetDebit(){return m_dbDebit;};

        Frontiere*  GetFrontiereAmont(){return m_pFrontiereAmont;};
        Frontiere*  GetFrontiereAval(){return m_pFrontiereAval;};

        double      CalculDemandeVar();
        double      CalculOffreVar();

        virtual void Calcul_emission_cellule(Loi_emission*, std::string); // Calcul de l'émission acoustique d'une cellule (p sur la l'objet loi d'émission, p sur l'objet rapport de boîte, type de revêtement du tronçon)
        virtual void Calcul_emission_vehicule(Loi_emission*, std::string); // Calcul de l'émission acoustique (puissance) d'un véhicule de la cellule (p sur la l'objet loi d'émission, p sur l'objet rapport de boîte, type de revêtement du tronçon)

        void EmissionOutput(); // Fonction de restitutions des emissions dans les fichiers texte et dans les matrices pour le fichier .mat        


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};
#endif
