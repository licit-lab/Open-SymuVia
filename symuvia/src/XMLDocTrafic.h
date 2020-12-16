#pragma once
#ifndef XMLDocTraficH
#define XMLDocTraficH

#include "DocTrafic.h"
#pragma warning(disable: 4005)
#include "ogr_spatialref.h"
#pragma warning(default: 4005)

namespace XERCES_CPP_NAMESPACE {
    class DOMDocument;
    class DOMNode;
    class DOMElement;
};

#include <map>

class DOMLSSerializerSymu;
class SDateTime;
class Reseau;

/*===========================================================================================*/
/* Classe de modélisation du document XML des données trafic								 */
/*===========================================================================================*/
class XMLDocTrafic : public DocTrafic
{
private:
    Reseau * m_pNetwork;

	// Le document XML
    XERCES_CPP_NAMESPACE::DOMDocument * pXMLDoc;
    // Le document XML temporaire utilisé pour l'instant courant
    XERCES_CPP_NAMESPACE::DOMDocument * m_pTemporaryInstXMLDoc;

public:	

	std::string			    m_strFilename;

    bool                    m_bSave;                // Indique si le document XML est sauvegardé
    bool                    m_bDebug;               // Indique si les informations de debbugage sont inscrites
    bool                    m_bTraceStocks;         // Indique si les informations sur les stocks sotn inscrites
    bool                    m_bSortieLight;         // Indique si sortie 'light'
    bool                    m_bSortieRegime;        // Indique si on doit sortir l'indicateur "free" = 0 ou 1 pour les trajectoires
    bool                    m_bReinitialized;       // Indique si le writer courant est issu d'un restore de snapshot
    bool                    m_bAtLeastOneInstant;   // Pour reprise de snapshot, permet de savoir comment le noeud instants doit être refermé

	OGRCoordinateTransformation *m_pCoordTransf;	// Transformation

private:

    std::string			    m_strTraceFile;
	DOMLSSerializerSymu		*m_XmlWriter;

    XERCES_CPP_NAMESPACE::DOMElement    *m_XmlNodeSimulation;
	XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeInstants;
	XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeInstant;		// Noeud de l'instant courant
    XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeCreations;    // Noeud des créations de véhicules à l'instant courant
	XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeSorties;		// Noeud des sorties de réseau des véhicules à l'instant courant
    XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodetraj; 		// Noeud des trajectoires de l'instant courant
    XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeStream; 		// Noeud des flux de l'instant courant
    XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeLinks; 		// Noeud des tuyaux de l'instant courant
    XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeFeux; 		// Noeud des feux de l'instant courant
    XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeEntrees; 		// Noeud des entrees de l'instant courant
    XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeRegulations;  // Noeud des briques de regulations
    XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeParkings; 	// Noeud des Parkings de l'instant courant
    XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeTronconsStationnement; 	// Noeud des tronçons de stationnement de l'instant courant
	XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeVehicules;
	XERCES_CPP_NAMESPACE::DOMNode       *m_XmlNodeCellules; 
    XERCES_CPP_NAMESPACE::DOMNode       *m_XmlNodeArrets;     
    XERCES_CPP_NAMESPACE::DOMNode       *m_XmlNodeTroncons;     // Noeud d'accès à la collection des tronçons (utilsateur ou Symubruit)
    XERCES_CPP_NAMESPACE::DOMElement	*m_XmlNodeSymCpts;      // Noeud d'accès à la collection des descriptions des capteurs (utilisateur ou Symubruit)
    XERCES_CPP_NAMESPACE::DOMElement    *m_XmlNodeSimuCpts;		// Noeud d'accès à la collection des mesures des capteurs (utilsateur ou Symubruit)
    XERCES_CPP_NAMESPACE::DOMElement    *m_XmlNodeConnexions;   // Noeud d'accès à la collection des connexions
    XERCES_CPP_NAMESPACE::DOMNode       *m_XmlNodeSymEntrees;   // Noeud d'accès à la collection des entrées
    XERCES_CPP_NAMESPACE::DOMNode       *m_XmlNodeSymSorties;   // Noeud d'accès à la collection des sorties

    // map des noeuds de chaque véhicule pour ne pas avoir à faire de recherche sans arret
    std::map<int, XERCES_CPP_NAMESPACE::DOMElement*>  m_VehNodes;

public:    

    // Constructeur par défaut
    XMLDocTrafic();

    // Constructeur
    XMLDocTrafic(Reseau * pNetwork, XERCES_CPP_NAMESPACE::DOMDocument * pXMLDocument);

	virtual ~XMLDocTrafic();

    XERCES_CPP_NAMESPACE::DOMDocument * getTemporaryInstantXMLDocument()
    {
        return m_pTemporaryInstXMLDoc;
    }

	void setDebug(bool bDebug)
	{
        m_bDebug = bDebug;
	}
	void setSave(bool bSave)
	{
        m_bSave=bSave; 
	}
    void setTraceStocks(bool bTraceStocks)
	{
        m_bTraceStocks = bTraceStocks;
	}
	void setSortieLight(bool bSortieLight)
	{
        m_bSortieLight = bSortieLight;
	}
    void setSortieRegime(bool bSortieRegime)
    {
        m_bSortieRegime = bSortieRegime;
    }

    // Initialisation
	virtual void Init(const std::string & strFilename, const std::string & strVer, SDateTime dtDeb, SDateTime dtFin, double dbPerAgrCpts, XERCES_CPP_NAMESPACE::DOMDocument * xmlDocReseau, unsigned int uiInit, std::string simulationID, std::string traficID, std::string reseauID, OGRCoordinateTransformation *pCoordTransf=NULL);

    // Re-initialisation (après une désérialisation par exemple)
    virtual void ReInit(std::string result);

	virtual void PostInit();

    // Mise en veille (prise snapshot pour reprise plus tard)
    virtual void Suspend();

    // Fin
	virtual void Terminate();

    // Suppression du fichier
    virtual void Remove();

    // Ajout d'un instant
	virtual void AddInstant(double dbInstant, double dbTimeStep, int nNbVeh);

    // Ajout d'une période de surveillance des capteurs
    virtual void AddPeriodeCapteur(double dbDeb, double dbFin, const std::string & nodeName);

	/*DOMNode**/ PCAPTEUR AddInfoCapteur(const std::string & sIdCpt, const std::string & sTypeVeh, const std::string & sVitGlob, const std::string & sNbCumGlob, const std::string & sVitVoie, const std::string & sNbCumVoie, const std::string & sFlow, bool bExtractVehicleList);

    /*DOMNode**/ PCAPTEUR AddInfoCapteurLongitudinal(const std::string & sIdCpt, const std::string & sVoieOrigine, const std::string & sVoieDestination, const std::string & sCount);

	/*DOMNode**/ PCAPTEUR AddInfoCapteurEdie(const std::string & sIdCpt, const std::string & sConcentrationCum, const std::string & sDebitCum, const std::string & sConcentration, const std::string & sDebit);

    /*DOMNode**/ PCAPTEUR AddInfoCapteurMFD(const std::string & sIdCpt, bool bIncludeStrictData, const std::string & sGeomLength1, const std::string & sGeomLength2, const std::string & sGeomLength3,
        const std::string & sDistanceTotaleParcourue, const std::string & sDistanceTotaleParcourueStricte, const std::string & sTempsTotalPasse, const std::string & sTempsTotalPasseStrict,
        const std::string & sDebitSortie, const std::string &  sIntDebitSortie, const std::string &  sTransDebitSortie, const std::string & sDebitSortieStrict, const std::string & sLongueurDeplacement, const std::string & sLongueurDeplacementStricte,
        const std::string & sVitesseSpatiale, const std::string & sVitesseSpatialeStricte, const std::string & sConcentration, const std::string & sDebit);

    virtual void AddInfoCapteurMFDByTypeOfVehicle( const std::string & sIdCpt, const std::string & sIdTV, const std::string & sTTD, const std::string & sTTT);

    /*DOMNode**/ PCAPTEUR AddInfoCapteurReservoir(const std::string & sIdCpt);

    /*DOMNode**/ PCAPTEUR AddInfoCapteurBlueTooth(const std::string & sIdCpt, const BlueToothSensorData& data);

	PCAPTEUR AddIndicateursCapteurGlobal( double dbTravelledDistance, double dbTravelledTime, double dbVit, int nNbVeh);

	virtual PCAPTEUR AddInfoCapteurGlobal( const std::string & sIdVeh, double TpsEcoulement, double DistanceParcourue);

    virtual void AddVehCpt(/*DOMNode **/PCAPTEUR xmlNodeCpt, const std::string & sId, double dbInstPsg, int nVoie);

    virtual void AddTraverseeCpt(/*DOMNode **/PCAPTEUR xmlNodeCpt, const std::string & vehId, const std::string & entryTime, const std::string & exitTime, const std::string & travelledDistance,
        const std::string & bCreatedInZone, const std::string & bDetroyedInZone);

    // Sauvegarde d'un instant
	virtual void SaveLastInstant();

	// Suppression du dernier instant en mémoire DOM
	virtual void RemoveLastInstant();

	// Ajoute le contenu du document trafic de la source dans le document courant
	virtual void Add(DocTrafic *docTraficSrc);

    // Retourne le flux XML du dernier instant
    XERCES_CPP_NAMESPACE::DOMNode*	GetLastInstant(bool bFull, bool bTraceIteratif);

	XERCES_CPP_NAMESPACE::DOMNode*	GetSymTronconsNode();

    XERCES_CPP_NAMESPACE::DOMNode*	GetSymVehiclesNode() const;

    // Ajout d'une description d'une cellule de discrétisation
	virtual void AddCellule(int nID, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav,  double dbYav, double dbZav);

    // Ajout d'une description d'un tronçon (défini par l'utilisateur ou construit par Symubruit)
	virtual void AddTroncon(const std::string & strLibelle, Point* pPtAm, Point* pPtAv, int nVoie , const std::string & ssBrique, std::deque<Point*> &lstPtInterne, double dbLong, std::vector<double> dbLarg, double dbStockMax, double dbStockInitial, Tuyau * pTuyau);

    // Ajout d'une description d'une voie (défini par l'utilisateur ou construit par Symubruit)
    virtual void AddVoie(Point* pPtAm, Point* pPtAv, std::deque<Point*> &lstPtInterne, double dbLong);

    // Ajout d'une description d'un capteur (de l'utilisateur ou créé par Symubruit)
    virtual void AddDefCapteur(const std::string & sId, const std::string & sT, double dbPos, Point ptCoord );

    // Ajout d'une description d'un capteur longitudinal (de l'utilisateur)
    virtual void AddDefCapteurLongitudinal(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut, double dbPosFin, Point ptCoordFin);

    // Ajout d'une description d'un capteur Edie (de l'utilisateur)
    virtual void AddDefCapteurEdie(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut, double dbPosFin, Point ptCoordFin);

    // Ajout d'une description d'un capteur MFD (de l'utilisateur)
    virtual void AddDefCapteurMFD(const std::string & sId, bool bIncludeStrictData, const std::vector<Tuyau*>& lstTuyaux);

    // Ajout d'une description d'une entrée
    virtual void AddDefEntree(const std::string & sId, Point ptCoord );

    // Ajout d'une description d'une sortie
    virtual void AddDefSortie(const std::string & sId, Point ptCoord );

    // Ajout d'une description d'un CAF
    virtual void AddDefCAF(CarrefourAFeuxEx * pCAF);

    // Ajout d'une description d'un Giratoire
    virtual void AddDefGir(Giratoire * pGir);

    // Ajout d'une description d'un CDF
    virtual void AddDefCDF(ControleurDeFeux * pCDF);

    // Ajout d'une description d'un arrêt de bus
	virtual void AddArret(const std::string & strLibelle, int numVoie, char * strNomLigne);

    // Ajout de la description d'un véhicule
	virtual void AddVehicule(int nID, const std::string & strLibelle, const std::string & strType, const std::string & strGMLType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string &  strZoneEntree, const std::string &  strZoneSortie, const std::string & strPlaqueEntree, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif, int iLane, const std::string & sLine, const std::vector<std::string> & initialPath, const std::deque<PlageAcceleration> & plagesAcc, const std::string & motifOrigine, const std::string & motifDestination, bool bAddToCreationNode);

    // ajout la description d'un véhicule sur sa création
    virtual void AddVehiculeToCreationNode(int nID, const std::string & strLibelle, const std::string & strType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif) ;

	virtual void AddVehiculeToSortieNode(int nID, const std::string & strSortie, double dbInstSortie, double dbVit);

    // Ajout des sorties d'une brique de régulation
    virtual void AddSimuRegulation(XERCES_CPP_NAMESPACE::DOMNode * pRestitutionNode);

    // Ajout de la description des données trafic d'une entrée
	virtual void AddSimuEntree(const std::string & sEntree, int nVeh);
    
    // Ajout de la description des données trafic d'un parking
    virtual void AddSimuParking(const std::string & sParking, int nStock, int nVehAttente);

    // Ajout de la description des données trafic d'un troncon de stationnement
    virtual void AddSimuTronconStationnement(const std::string & sTroncon, double dbLongueur);

	// Mise à jour de l'instant d'entrée du véhicule (peut être différent de l'instant de création
	virtual void UpdateInstEntreeVehicule(int nID, double dbInstEntree);

    // Mise à jour de l'instant de sortie du véhicule
	virtual void UpdateInstSortieVehicule(int nID, double dbInstSortie, const std::string & sSortie, const std::string & sPlaqueSortie, double dstParcourue, const std::vector<Tuyau*> & itinerary, const std::map<std::string, std::string> & additionalAttributes);

	// Suppression des véhicules mémorisées à partir d'un certain ID
    virtual void RemoveVehicules(int nFromID);

    // Ajout de l'état des feux pour l'instant considéré
	virtual void AddSimFeux(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire);

    // Ajout de l'état des feux pour l'instant considéré pour EVE
	virtual void AddSimFeuxEVE(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire);
  
    // Ajout des données complète trafic d'une cellule de discrétisation pour l'instant considéré
	virtual void AddCellSimu(int nID, double dbConc, double dbDebit, double dbVitAm, double dbAccAm, double dbNAm, double dbVitAv, double dbAccAv, double dbNAv,
		const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav,  double dbYav, double dbZav);

    // Ajout des données de la trajectoire d'un véhicule à l'instant considéré
	virtual void AddTrajectoire(int nID, Tuyau * pTuyau, const std::string & strTuyau, const std::string & strTuyauEx, const std::string & strNextTuyauEx, int nNumVoie, double dbAbs, double dbOrd, double dbZ, double dbAbsCur, double dbVit, double dbAcc, double dbDeltaN, const std::string & sTypeVehicule, double dbVitMax, double dbLongueur, const std::string & sLib, int nIDVehLeader, int nCurrentLoad, bool bTypeChgtVoie, TypeChgtVoie eTypeChgtVoie,
        bool bVoieCible, int nVoieCible, bool bPi, double dbPi, bool bPhi, double dbPhi, bool bRand, double dbRand, bool bDriven, const std::string & strDriveState, bool bDepassement, bool bRegimeFluide, const std::map<std::string, std::string> & additionalAttributes);

    // Ajout des données de flux d'un véhicule à l'instant considéré
	virtual void AddStream(int nID, const std::string & strTuyau, const std::string & strTuyauEx);

    // Ajout des données d'un tuyau à l'instant considéré
    virtual void AddLink(const std::string &  strTuyau, double dbConcentration);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};


#endif
