#pragma once
#ifndef tronconH
#define tronconH

#pragma warning(push)
#pragma warning(disable : 4251)

#include "SymubruitExports.h"

#include "tools.h"

#include <boost/serialization/assume_abstract.hpp>

#include <vector>
#include <map>
#include <fstream>

class Reseau;
class TypeVehicule;

struct TerrePlein
{
public:
    TerrePlein() {};
    TerrePlein(double debut, double fin, std::vector<double> durees, std::vector<bool> actifs,
        std::vector<PlageTemporelle*> plages, std::vector<bool> plagesActives)
    {
        dbPosDebut = debut;
        dbPosFin = fin;
        dbDurees = durees;
        bActifs = actifs;
        pPlages = plages;
        bPlagesActives = plagesActives;
    };

    double  dbPosDebut;
    double  dbPosFin;
    std::vector<double>  dbDurees;
    std::vector<bool>    bActifs;
    std::vector<PlageTemporelle*> pPlages;
    std::vector<bool>             bPlagesActives;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


class SYMUBRUIT_EXPORT Troncon
{
public:
        // Constructeur
        Troncon     ();
		Troncon     (double dbVitReg, double dbVitCat, std::string sNom);

        // Destructeur
        virtual ~Troncon();

		std::string m_strLabel;				// Libellé

protected:
		
        int         m_nID;                  // Identifiant numérique du tronçon                    

        std::string m_strRevetement;        // Revêtement

        Troncon*    m_pParent;              // Parent du tronçon

        // Variables liées à la simulation
        int         m_nNbCell;              // Nombre de cellule de discrétisation
        double      m_dbPasEspace;          // Pas d'espace
        double      m_dbPasTemps;           // Pas de temps

        double      m_dbDemande;            // Demande du tronçon
        double      m_dbOffre;              // Offre du tronçon

        double      m_dbDebitEntree;        // Débit de l'entrée
        double      m_dbDebitSortie;        // Débit de la sortie

        double      m_dbVitMaxSortie;       // Vitesse max de sortie du tronçon
        double      m_dbVitMaxEntree;       // Vitesse max d'entrée du tronçon

        Reseau*     m_pReseau;              // Pointeur sur le réseau        

        Point       m_ptExtAmont;           // Extrémité amont
        Point       m_ptExtAval;            // Extrémité aval

        double      m_dbDstChgtVoie;        // Distance avant la fin du tronçon pour lequel on calcule le chgt de voie
                                            // d'un véhicule qui ne se trouve pas sur la bonne voie pour prendre le tronçon suivant

        double      m_dbDstChgtVoieForce;   // Comme m_dbDstChgtVoie, mais pour forcer dans ce cas encore plus le changement de voie avec un phi donné
        double      m_dbPhiChgtVoieForce;   // Phi imposé dans la zone de changement de voie forcée définie par m_dbDstChgtVoieForce

        // Variables intrinsèques
        double      m_dbVitReg;             // Vitesse réglementaire
        double      m_dbVitCat;             // Vitesse à utiliser pour l'estimation des temps de parcours à vide

        // Variables liées à la géographie
        bool                    m_bCurviligne;                          // Indique si le tronçon est curviligne (la longueur a été saisie par l'utilisateur)
        std::deque<Point*>      m_LstPtsInternes;                       // Tableau dynamique des points interne définissant la géométrie du tronçon

        double  m_dbLongueur;                       // Longueur réelle du tronçon (en mètre)
        double  m_dbLongueurProjection;             // Longueur projeté du tronçon (en mètre)

		std::deque<TypeVehicule*>       m_LstTypesVehicule;                     // Liste des types de véhicule pouvant emprunter le tronçon

        std::string m_strRoadLabel;         // Libellé de la route correspondante
                
public:
        std::map<int, std::vector<TerrePlein>* > m_mapTerrePleins; // map des terre-pleins (liste de variations temporelles par voie)

public:        	  

        // get - set
        int             GetID()         {return m_nID;};
        void            SetID(int nID)  {m_nID = nID;};
		std::string     GetLabel() const       {return m_strLabel;};
		void            SetLabel(std::string sLabel) {m_strLabel = sLabel;};
        int             GetNbCell()     {return m_nNbCell;};
        std::string     GetRevetement() {return m_strRevetement;};

        Troncon*        GetParent()     {return m_pParent;};

        void            SetOffre(double dbOffre){m_dbOffre = dbOffre;};
        double          GetOffre()      {return m_dbOffre;};

        double          GetDemande()    {return m_dbDemande;};
        void            SetDemande(double dbDem){m_dbDemande = dbDem;};

        void            SetDebitEntree            (double dbDebit){m_dbDebitEntree = dbDebit;};
        double          GetDebitEntree            (){return m_dbDebitEntree;}

        void            SetDebitSortie             (double dbDebit){m_dbDebitSortie = dbDebit;};
        double          GetDebitSortie(){return m_dbDebitSortie;}

        void            SetVitMaxSortie         (double dbVit){m_dbVitMaxSortie = dbVit;};
        double          GetVitMaxSortie()       {return m_dbVitMaxSortie;};

        void            SetVitMaxEntree         (double dbVit){m_dbVitMaxEntree = dbVit;};
        double          GetVitMaxEntree()       {return m_dbVitMaxEntree;};

        void            SetVitCat(double dbVit) { m_dbVitCat = dbVit; }
        double          GetVitCat()             { return m_dbVitCat; }

        double          GetDebitMax();
        double          GetVitesseMax();

        double          GetAbsAmont(){return m_ptExtAmont.dbX;};
        double          GetAbsAval (){return m_ptExtAval.dbX; };
        double          GetOrdAmont(){return m_ptExtAmont.dbY;};
        double          GetOrdAval (){return m_ptExtAval.dbY; };
        double          GetHautAmont(){return m_ptExtAmont.dbZ;};
        double          GetHautAval (){return m_ptExtAval.dbZ; };

        double          GetVitReg  (){return m_dbVitReg;  };
        void            SetVitReg  (double dbVitReg){m_dbVitReg = dbVitReg;};

        double          GetDstChgtVoie(double dbDstChgtVoieGlobal){return m_dbDstChgtVoie!=-1?m_dbDstChgtVoie:dbDstChgtVoieGlobal;}
        void            SetDstChgtVoie(double dbDstChgtVoie){m_dbDstChgtVoie = dbDstChgtVoie;}

        double          GetDstChgtVoieForce(){ return m_dbDstChgtVoieForce; }
        void            SetDstChgtVoieForce(double dbDstChgtVoieForce){ m_dbDstChgtVoieForce = dbDstChgtVoieForce; }

        double          GetPhiChgtVoieForce(){ return m_dbPhiChgtVoieForce; }
        void            SetPhiChgtVoieForce(double dbPhiChgtVoieForce){ m_dbPhiChgtVoieForce = dbPhiChgtVoieForce; }

        double          GetLength() const{return m_dbLongueur;};
        double          GetLongueurProjection(){return m_dbLongueurProjection;};

        Reseau*         GetReseau(){return m_pReseau;};

        double          GetPasEspace(){return m_dbPasEspace;};
        double          GetPasTemps(){return m_dbPasTemps;};

        void            SetNbCell(int nCell){m_nNbCell = nCell;};

        void            SetPasEspace(double dbPasEspace){ m_dbPasEspace = dbPasEspace;};

        void            SetPropCom(
                                        Reseau*         pReseau,
                                        Troncon*        pParent,
                                        std::string     strNom,
                                        std::string     strRevetement
                                        );
                                        
        void            SetPropSimu(    int         nNbCell,
                                        double      dbPasEspace,
                                        double      dbPasTemps
                                    );

        void            SetCurviligne(bool bCurv){m_bCurviligne=bCurv;};

        void            SetExtAmont(Point pt){m_ptExtAmont = pt;};
        void            SetExtAmont(double dbX, double dbY, double dbZ){m_ptExtAmont.dbX=dbX; m_ptExtAmont.dbY=dbY; m_ptExtAmont.dbZ = dbZ;};

        void            SetExtAval(Point pt){m_ptExtAval = pt;};
        void            SetExtAval(double dbX, double dbY, double dbZ){m_ptExtAval.dbX=dbX; m_ptExtAval.dbY=dbY; m_ptExtAval.dbZ=dbZ;};

        Point*          GetExtAmont(){return &m_ptExtAmont;};
        Point*          GetExtAval(){return &m_ptExtAval;};

		// Remplit les vecteurs passés par référence des coordonnées de la polyligne du tronçon
		void			BuildLineStringXY(std::vector<double> & x, std::vector<double> & y) const;

        void            AddPtInterne( double dbX, double dbY, double dbZ);    

        std::deque<Point*> &GetLstPtsInternes()              {return m_LstPtsInternes;};
        std::deque<TypeVehicule*>    &GetLstTypesVeh()       {return m_LstTypesVehicule;};

        void                    AddTypeVeh(TypeVehicule *pTV);
        
        void                    DeleteLstPoints();

        void                    CalculeLongueur(double dbL);
        void                    SetLongueur(double dbL){m_dbLongueur = dbL;};

        // Méthodes virtuelles        
        virtual bool    SortieGeoCell(std::ofstream Fic, bool bEmission){return true;};        

        // Méthodes virtuelles pures
		virtual bool    InitSimulation(bool bAcou, bool bSirane, std::string strName)   = 0;
        virtual void    ComputeTraffic(double nInstant)                      = 0;
        virtual void    ComputeTrafficEx(double nInstant)                    = 0;
        virtual void    TrafficOutput()                                  = 0;
        virtual void    EmissionOutput()            = 0;
        virtual void    InitAcousticVariables()                      = 0;

		void			SetReseau(Reseau *pR){m_pReseau = pR;};

        // Ajout de la définition d'un terre-plein
	    void            AddTerrePlein(int nVoie, double dbPosDebut, double dbPosFin, std::vector<double> durees, std::vector<bool> actifs,
            std::vector<PlageTemporelle*>   plages, std::vector<bool>   plagesActives);
        // teste la présence d'un terre plein
        bool            HasTerrePlein(double dbInst, double dbPos, int nVoie, int nVoie2);

        void            SetRoadLabel(const std::string & strRoadLabel) { m_strRoadLabel = strRoadLabel;}
        std::string     GetRoadLabel() { return m_strRoadLabel;}

public:

		std::string GetUnifiedID() { return m_strLabel; }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(Troncon) // la classe est pure virtual

#pragma warning(pop)

#endif
