#pragma once
#ifndef GMLDocTraficH
#define GMLDocTraficH

#include "DocTrafic.h"
#include "TimeUtil.h"
#include "Xerces/XMLUtil.h"

#include <map>

class DOMLSSerializerSymu;

class GMLDocTrafic : public DocTrafic
{
private:
    std::string            m_strFilenameNetwork;
    std::string            m_strFilenameTrafficLights;
    std::string            m_strFilenameTrajsHeader;
    std::string            m_strFilenameTrajsTemp;
    std::string            m_strFilenameSensors;
    
	DOMLSSerializerSymu    *m_XmlWriterNetwork;
    DOMLSSerializerSymu    *m_XmlWriterTrafficLights;
    DOMLSSerializerSymu    *m_XmlWriterTrajsHeader;
    DOMLSSerializerSymu    *m_XmlWriterTrajsTemp;
    DOMLSSerializerSymu    *m_XmlWriterSensors;

    SDateTime               m_dtDeb;
    double                  m_dbCurrentInstant;
    double                  m_dbTimeStep;

    double                  m_dbSensorPeriodStart;
    double                  m_dbSensorPeriodEnd;

    std::map<int, Point>    m_mapLastVehiclePositions;

    bool                    m_bReinitialized;

public:
    // Accesseurs
    const std::string & GetTrafficLightsFileName() {return m_strFilenameTrafficLights;}
    const std::string & GetTrajHeaderFileName() {return m_strFilenameTrajsHeader;}
    const std::string & GetTrajTempFileName() {return m_strFilenameTrajsTemp;}
    const std::string & GetSensorsFileName() {return m_strFilenameSensors;}


public:

    // Constructeur par d�faut
    GMLDocTrafic();

    virtual ~GMLDocTrafic();

    virtual void Init(const std::string & strFilename, SDateTime dtDeb, SDateTime dtFin, const std::vector<Point> & enveloppe, bool bTmpDoc, const std::string & strFinalFilename, GMLDocTrafic * pOriginalDoc, bool bHasAtLeastOneCDF, bool bHasSensor);

    // Re-initialisation (apr�s une d�s�rialisation par exemple)
    virtual void ReInit(const std::string & trafficLights, const std::string & trajHeader, const std::string & trajTemp, const std::string & sensors);

	virtual void PostInit();

    // Mise en veille (prise snapshot pour reprise plus tard)
    virtual void Suspend();

    // Fin
    virtual void Terminate();

    // Suppression du fichier
    virtual void Remove();

    // Ajout d'un instant
    virtual void AddInstant(double dbInstant, double dbTimeStep, int nNbVeh);

    // Ajout d'une p�riode de surveillance des capteurs
    virtual void AddPeriodeCapteur(double dbDeb, double dbFin, const std::string & nodeName);

    virtual PCAPTEUR AddInfoCapteur(const std::string & sIdCpt, const std::string & sTypeVeh, const std::string & sVitGlob, const std::string & sNbCumGlob, const std::string & sVitVoie, const std::string & sNbCumVoie, const std::string & sFlow, bool bExtractVehicleList);

    virtual PCAPTEUR AddInfoCapteurLongitudinal(const std::string & sIdCpt, const std::string & sVoieOrigine, const std::string & sVoieDestination, const std::string & sCount);

    virtual PCAPTEUR AddInfoCapteurEdie(const std::string & sIdCpt, const std::string & sConcentrationCum, const std::string & sDebitCum, const std::string & sConcentration, const std::string & sDebit);

    virtual PCAPTEUR AddInfoCapteurMFD(const std::string & sIdCpt, bool bIncludeStrictData, const std::string & sGeomLength1, const std::string & sGeomLength2, const std::string & sGeomLength3,
        const std::string & sDistanceTotaleParcourue, const std::string & sDistanceTotaleParcourueStricte, const std::string & sTempsTotalPasse, const std::string & sTempsTotalPasseStrict,
        const std::string & sDebitSortie, const std::string &  sIntDebitSortie, const std::string &  sTransDebitSortie, const std::string & sDebitSortieStrict, const std::string & sLongueurDeplacement, const std::string & sLongueurDeplacementStricte,
        const std::string & sVitesseSpatiale, const std::string & sVitesseSpatialeStricte, const std::string & sConcentration, const std::string & sDebit);

    virtual void AddInfoCapteurMFDByTypeOfVehicle( const std::string & sIdCpt, const std::string & sIdTV, const std::string & sTTD, const std::string & sTTT){};

    virtual PCAPTEUR AddInfoCapteurReservoir(const std::string & sIdCpt);

    virtual PCAPTEUR AddInfoCapteurBlueTooth(const std::string & sIdCpt, const BlueToothSensorData& data);

	virtual PCAPTEUR AddInfoCapteurGlobal( const std::string & sIdVeh, double TpsEcoulement, double DistanceParcourue);

	virtual PCAPTEUR AddIndicateursCapteurGlobal(  double dbTravelledDistance, double dbTravelledTime, double dbVit, int nNbVeh);

    virtual void AddVehCpt(PCAPTEUR xmlNodeCpt, const std::string & sId, double dbInstPsg, int nVoie);

    virtual void AddTraverseeCpt(PCAPTEUR xmlNodeCpt, const std::string & vehId, const std::string & entryTime, const std::string & exitTime, const std::string & travelledDistance,
        const std::string & bCreatedInZone, const std::string & bDetroyedInZone);

    // Sauvegarde d'un instant
    virtual void SaveLastInstant();

    // Suppression du dernier instant en m�moire DOM
    virtual void RemoveLastInstant();

    // Ajoute le contenu du document trafic de la source dans le document courant
    virtual void Add(DocTrafic * docTraficSrc);

    // Ajout d'une description d'une cellule de discr�tisation
    virtual void AddCellule(int nID, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav);

    // Ajout d'une description d'un tron�on (d�fini par l'utilisateur ou construit par Symubruit)
    virtual void AddTroncon(const std::string & strLibelle, Point * pPtAm, Point * pPtAv, int nVoie, const std::string & ssBrique, std::deque<Point*> &lstPtInterne, double dbLong, std::vector<double> dbLarg, double dbStockMax, double dbStockInitial, Tuyau * pTuyau);

    // Ajout d'une description d'une voie (d�fini par l'utilisateur ou construit par Symubruit)
    virtual void AddVoie(Point * pPtAm, Point * pPtAv, std::deque<Point*> &lstPtInterne, double dbLong);

    // Ajout d'une description d'un capteur (de l'utilisateur ou cr�� par Symubruit)
    virtual void AddDefCapteur(const std::string & sId, const std::string & sT, double dbPos, Point ptCoord);

    // Ajout d'une description d'un capteur longitudinal (de l'utilisateur)
    virtual void AddDefCapteurLongitudinal(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut,
        double dbPosFin, Point ptCoordFin);

    // Ajout d'une description d'un capteur Edie (de l'utilisateur)
    virtual void AddDefCapteurEdie(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut,
        double dbPosFin, Point ptCoordFin);

    // Ajout d'une description d'un capteur MFD (de l'utilisateur)
    virtual void AddDefCapteurMFD(const std::string & sId, bool bIncludeStrictData, const std::vector<Tuyau*>& lstTuyaux);

    // Ajout d'une description d'une entr�e
    virtual void AddDefEntree(const std::string & sId, Point ptCoord);

    // Ajout d'une description d'une sortie
    virtual void AddDefSortie(const std::string & sId, Point ptCoord);

    // Ajout d'une description d'un CAF
    virtual void AddDefCAF(CarrefourAFeuxEx * pCAF);

    // Ajout d'une description d'un Giratoire
    virtual void AddDefGir(Giratoire * pGir);

    // Ajout d'une description d'un CDF
    virtual void AddDefCDF(ControleurDeFeux * pCDF);

    // Ajout d'une description d'un arr�t de bus
    virtual void AddArret(const std::string & strLibelle, int numVoie, char * strNomLigne);

    // Ajout de la description d'un v�hicule
	virtual void AddVehicule(int nID, const std::string & strLibelle, const std::string & strType, const std::string & strGMLType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string &  strZoneEntree, const std::string &  strZoneSortie, const std::string & strPlaqueEntree, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif, int iLane, const std::string & sLine, const std::vector<std::string> & initialPath, const std::deque<PlageAcceleration> & plagesAcc, const std::string & motifOrigine, const std::string & motifDestination, bool bAddToCreationNode);

    // ajout la description d'un v�hicule sur sa cr�ation
    virtual void AddVehiculeToCreationNode(int nID, const std::string & strLibelle, const std::string & strType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif);

	// ajout la description d'un v�hicule lors de la sortie d'un v�hicule du r�seau
	virtual void AddVehiculeToSortieNode(int nID, const std::string & strSortie, double dbInstSortie, double dbVit) {};

    // Ajout des sorties d'une brique de r�gulation
    virtual void AddSimuRegulation(XERCES_CPP_NAMESPACE::DOMNode * pRestitutionNode);

    // Ajout de la description des donn�es trafic d'une entr�e
    virtual void AddSimuEntree(const std::string & sEntree, int nVeh);

    // Ajout de la description des donn�es trafic d'un parking
    virtual void AddSimuParking(const std::string & sParking, int nStock, int nVehAttente);

    // Ajout de la description des donn�es trafic d'un troncon de stationnement
    virtual void AddSimuTronconStationnement(const std::string & sTroncon, double dbLongueur);

	// Mise � jour de l'instant d'entr�e du v�hicule (peut �tre diff�rent de l'instant de cr�ation
	virtual void UpdateInstEntreeVehicule(int nID, double dbInstEntree);

    // Mise � jour de l'instant de sortie du v�hicule
	virtual void UpdateInstSortieVehicule(int nID, double dbInstSortie, const std::string & sSortie, const std::string & sPlaqueSortie, double dstParcourue, const std::vector<Tuyau*> & itinerary, const std::map<std::string, std::string> & additionalAttributes);

    // Suppression des v�hicules m�moris�es � partir d'un certain ID
    virtual void RemoveVehicules(int nFromID);
	
    // Ajout de l'�tat des feux pour l'instant consid�r�
    virtual void AddSimFeux(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire);

    // Ajout de l'�tat des feux pour l'instant consid�r� pour EVE
    virtual void AddSimFeuxEVE(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire);

    // Ajout des donn�es compl�te trafic d'une cellule de discr�tisation pour l'instant consid�r�
    virtual void AddCellSimu(int nID, double dbConc, double dbDebit, double dbVitAm, double dbAccAm, double bNAm, double dbVitAv, double dbAccAv, double dbNAv, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav);
    
    // Ajout des donn�es de la trajectoire d'un v�hicule � l'instant consid�r�
    virtual void AddTrajectoire(int nID, Tuyau * pTuyau, const std::string & strTuyau, const std::string & strTuyauEx, const std::string & strNextTuyauEx, int nNumVoie, double dbAbs, double dbOrd, double dbZ, double dbAbsCur, double dbVit, double dbAcc, double dbDeltaN, const std::string & sTypeVehicule, double dbVitMax, double dbLongueur, const std::string & sLib, int nIDVehLeader, int nCurrentLoad,
        bool bTypeChgtVoie, TypeChgtVoie eTypeChgtVoie, bool bVoieCible, int nVoieCible, bool bPi, double dbPi, bool bPhi, double dbPhi, bool bRand, double dbRand, bool bDriven, const std::string & strDriveState, bool bDepassement, bool bRegimeFluide, const std::map<std::string, std::string> & additionalAttributes);

    // Ajout des donn�es du flux d'un v�hicule � l'instant consid�r�
    virtual void AddStream(int nID, const std::string & strTuyau, const std::string & strTuyauEx);

    // Ajout des donn�es d'un tuyau � l'instant consid�r�
    virtual void AddLink(const std::string &  strTuyau, double dbConcentration);

private:

    // Construit une cha�ne de caract�re d�crivant la g�om�trie d'une surface � partir des deux polylignes englobantes
    std::string BuildSurfaceString(const std::deque<Point> & line1, const std::deque<Point> & line2, int & nbPoints);

    // Construit l'ID d'un feu tricolore en fonction du CDF et du mouvement correspondant
    std::string BuildTrafficLightName(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS);

    // �crit le noeud om:phenomenonTime pour une mesure capteur
    void writeCommonObservationNodes(const std::string & sCptId);

    // �crit un noeud <sml:output> pour une d�finition de capteur
    void writeSensorOutput(const XMLCh* sLabel, const XMLCh* sUnit);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);

};

#endif // GMLDocTraficH