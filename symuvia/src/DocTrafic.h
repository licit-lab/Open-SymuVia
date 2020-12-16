#pragma once
#ifndef _DOCTRAFIC_H
#define _DOCTRAFIC_H

#include <boost/serialization/assume_abstract.hpp>

#include <xercesc/util/XercesVersion.hpp>

#include <string>
#include <deque>
#include <vector>
#include <map>

struct Point;
struct BlueToothSensorData;
struct PlageAcceleration;
class Tuyau;
class CarrefourAFeuxEx;
class Giratoire;
class ControleurDeFeux;

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
};

namespace boost {
    namespace serialization {
        class access;
    }
}

// Types de changement de voie
typedef enum {
    eConfort = 0,
    eForce,
    eRabattement
} TypeChgtVoie;

class DocTrafic 
{
  public:
    typedef void * PCAPTEUR;

	virtual ~DocTrafic(){};

    virtual void PostInit() = 0;

    // Fin
    virtual void Terminate() = 0;

    // Suppression du fichier
    virtual void Remove() = 0;

    // Ajout d'un instant
    virtual void AddInstant(double dbInstant, double dbTimeStep, int nNbVeh) = 0;

    // Ajout d'une p�riode de surveillance des capteurs
    virtual void AddPeriodeCapteur(double dbDeb, double dbFin, const std::string & nodeName) = 0;

    virtual PCAPTEUR AddInfoCapteur(const std::string & sIdCpt, const std::string & sTypeVeh, const std::string & sVitGlob, const std::string & sNbCumGlob, const std::string & sVitVoie, const std::string & sNbCumVoie, const std::string & sFlow, bool bExtractVehicleList) = 0;

    virtual PCAPTEUR AddInfoCapteurLongitudinal(const std::string & sIdCpt, const std::string & sVoieOrigine, const std::string & sVoieDestination, const std::string & sCount) = 0;

    virtual PCAPTEUR AddInfoCapteurEdie(const std::string & sIdCpt, const std::string & sConcentrationCum, const std::string & sDebitCum, const std::string & sConcentration, const std::string & sDebit) = 0;

    virtual PCAPTEUR AddInfoCapteurMFD(const std::string & sIdCpt, bool bIncludeStrictData, const std::string &  sGeomLength1, const std::string &  sGeomLength2, const std::string &  sGeomLength3,
        const std::string &  sDistanceTotaleParcourue, const std::string &  sDistanceTotaleParcourueStricte, const std::string &  sTempsTotalPasse, const std::string &  sTempsTotalPasseStrict,
		const std::string &  sDebitSortie, const std::string &  sIntDebitSortie, const std::string &  sTransDebitSortie, const std::string &  sDebitSortieStrict, const std::string &  sLongueurDeplacement, const std::string &  sLongueurDeplacementStricte,
        const std::string &  sVitesseSpatiale, const std::string &  sVitesseSpatialeStricte, const std::string &  sConcentration, const std::string &  sDebit) = 0;

    virtual void AddInfoCapteurMFDByTypeOfVehicle( const std::string & sIdCpt, const std::string & sIdTV, const std::string & sTTD, const std::string & sTTT) = 0;                                                 // MFD sensor ID
                                                                    
    virtual PCAPTEUR AddInfoCapteurReservoir(const std::string & sIdCpt) = 0;

    virtual PCAPTEUR AddInfoCapteurBlueTooth(const std::string &  sIdCpt, const BlueToothSensorData& data) = 0;

	virtual PCAPTEUR AddInfoCapteurGlobal( const std::string &  sIdVeh, double TpsEcoulement, double DistanceParcourue) = 0;

	virtual PCAPTEUR AddIndicateursCapteurGlobal(  double dbTotalTravelledDistance, double dbTotalTravelTime, double dbVit, int nNbVeh) = 0;

    virtual void AddVehCpt(PCAPTEUR xmlNodeCpt, const std::string &  sId, double dbInstPsg, int nVoie) = 0;

    virtual void AddTraverseeCpt(PCAPTEUR xmlNodeCpt, const std::string & vehId, const std::string & entryTime, const std::string & exitTime, const std::string & travelledDistance,
        const std::string & bCreatedInZone, const std::string & bDetroyedInZone) = 0;

    // Sauvegarde d'un instant
    virtual void SaveLastInstant() = 0;

    // Suppression du dernier instant en m�moire DOM
    virtual void RemoveLastInstant() = 0;

    // Ajoute le contenu du document trafic de la source dans le document courant
    virtual void Add(DocTrafic * docTraficSrc) = 0;

    //// Retourne le flux XML du dernier instant
    //virtual DOMNode*	GetLastInstant(bool bFull, bool bTraceIteratif) = 0;
    // Ajout d'une description d'une cellule de discr�tisation
    virtual void AddCellule(int nID, const std::string &  strLibelle, const std::string &  strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav) = 0;

    // Ajout d'une description d'un tron�on (d�fini par l'utilisateur ou construit par Symubruit)
    virtual void AddTroncon(const std::string &  strLibelle, Point * pPtAm, Point * pPtAv, int nVoie, const std::string &  ssBrique, std::deque<Point*> &lstPtInterne, double dbLong, std::vector<double> dbLarg, double dbStockMax, double dbStockInitial, Tuyau * pTuyau) = 0;

    // Ajout d'une description d'une voie (d�fini par l'utilisateur ou construit par Symubruit)
    virtual void AddVoie(Point * pPtAm, Point * pPtAv, std::deque<Point*> &lstPtInterne, double dbLong) = 0;

    // Ajout d'une description d'un capteur (de l'utilisateur ou cr�� par Symubruit)
    virtual void AddDefCapteur(const std::string &  sId, const std::string &  sT, double dbPos, Point ptCoord) = 0;

    // Ajout d'une description d'un capteur longitudinal (de l'utilisateur)
    virtual void AddDefCapteurLongitudinal(const std::string &  sId, const std::string &  sT, double dbPosDebut, Point ptCoordDebut,
        double dbPosFin, Point ptCoordFin) = 0;

    // Ajout d'une description d'un capteur Edie (de l'utilisateur)
    virtual void AddDefCapteurEdie(const std::string &  sId, const std::string &  sT, double dbPosDebut, Point ptCoordDebut,
        double dbPosFin, Point ptCoordFin) = 0;

    // Ajout d'une description d'un capteur MFD (de l'utilisateur)
    virtual void AddDefCapteurMFD(const std::string &  sId, bool bIncludeStrictData, const std::vector<Tuyau*>& lstTuyaux) = 0;

    // Ajout d'une description d'une entr�e
    virtual void AddDefEntree(const std::string &  sId, Point ptCoord) = 0;

    // Ajout d'une description d'une sortie
    virtual void AddDefSortie(const std::string &  sId, Point ptCoord) = 0;

    // Ajout d'une description d'un CAF
    virtual void AddDefCAF(CarrefourAFeuxEx * pCAF) = 0;

    // Ajout d'une description d'un Giratoire
    virtual void AddDefGir(Giratoire * pGir) = 0;

    // Ajout d'une description d'un CDF
    virtual void AddDefCDF(ControleurDeFeux * pCDF) = 0;

    // Ajout d'une description d'un arr�t de bus
    virtual void AddArret(const std::string &  strLibelle, int numVoie, char * strNomLigne) = 0;

    // Ajout de la description d'un v�hicule
	virtual void AddVehicule(int nID, const std::string &  strLibelle, const std::string &  strType, const std::string &  strGMLType, double dbKx, double dbVx, double dbW, const std::string &  strEntree, const std::string &  strSortie, const std::string &  strZoneEntree, const std::string &  strZoneSortie, const std::string & strPlaqueEntree, const std::string &  strRoute, double dbInstCreation, const std::string &  sVoie, bool bAgressif, int iLane, const std::string &  sLine, const std::vector<std::string> & initialPath, const std::deque<PlageAcceleration> & plagesAcc,
        const std::string & motifOrigine, const std::string & motifDestination, bool bAddToCreationNode) = 0;

    // ajout la description d'un v�hicule sur sa cr�ation
    virtual void AddVehiculeToCreationNode(int nID, const std::string &  strLibelle, const std::string &  strType, double dbKx, double dbVx, double dbW, const std::string &  strEntree, const std::string &  strSortie, const std::string &  strRoute, double dbInstCreation, const std::string &  sVoie, bool bAgressif) = 0;

	// ajout la description d'un v�hicule lors de la sortie d'un v�hicule du r�seau
	virtual void AddVehiculeToSortieNode(int nID, const std::string & strSortie, double dbInstSortie, double dbVit) = 0;

    // Ajout des sorties d'une brique de r�gulation
    virtual void AddSimuRegulation(XERCES_CPP_NAMESPACE::DOMNode * pRestitutionNode) = 0;

    // Ajout de la description des donn�es trafic d'une entr�e
    virtual void AddSimuEntree(const std::string &  sEntree, int nVeh) = 0;

    // Ajout de la description des donn�es trafic d'un parking
    virtual void AddSimuParking(const std::string &  sParking, int nStock, int nVehAttente) = 0;

    // Ajout de la description des donn�es trafic d'un troncon de stationnement
    virtual void AddSimuTronconStationnement(const std::string &  sTroncon, double dbLongueur) = 0;

	// Mise � jour de l'instant d'entr�e du v�hicule (peut �tre diff�rent de l'instant de cr�ation
	virtual void UpdateInstEntreeVehicule(int nID, double dbInstEntree) = 0;

    // Mise � jour de l'instant de sortie du v�hicule
	virtual void UpdateInstSortieVehicule(int nID, double dbInstSortie, const std::string &  sSortie, const std::string & sPlaqueSortie, double dstParcourue, const std::vector<Tuyau*> & itinerary, const std::map<std::string, std::string> & additionalAttributes) = 0;

    // Suppression des v�hicules m�moris�es � partir d'un certain ID
    virtual void RemoveVehicules(int nFromID) = 0;
	
    // Ajout de l'�tat des feux pour l'instant consid�r�
    virtual void AddSimFeux(const std::string &  sCtrlFeux, const std::string &  sTE, const std::string &  sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire) = 0;

    // Ajout de l'�tat des feux pour l'instant consid�r� pour EVE
    virtual void AddSimFeuxEVE(const std::string &  sCtrlFeux, const std::string &  sTE, const std::string &  sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire) = 0;

    // Ajout des donn�es compl�te trafic d'une cellule de discr�tisation pour l'instant consid�r�
    virtual void AddCellSimu(int nID, double dbConc, double dbDebit, double dbVitAm, double dbAccAm, double bNAm, double dbVitAv, double dbAccAv, double dbNAv, const std::string &  strLibelle, const std::string &  strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav) = 0;
    
    // Ajout des donn�es de la trajectoire d'un v�hicule � l'instant consid�r�
    virtual void AddTrajectoire(int nID, Tuyau * pTuyau, const std::string &  strTuyau, const std::string &  strTuyauEx, const std::string &  strNextTuyauEx, int nNumVoie, double dbAbs, double dbOrd, double dbZ, double dbAbsCur, double dbVit, double dbAcc, double dbDeltaN, const std::string &  sTypeVehicule, double dbVitMax, double dbLongueur, const std::string &  sLib, int nIDVehLeader, int nCurrentLoad,
        bool bTypeChgtVoie, TypeChgtVoie eTypeChgtVoie, bool bVoieCible, int nVoieCible, bool bPi, double dbPi, bool bPhi, double dbPhi, bool bRand, double dbRand, bool bDriven, const std::string &  strDriveState, bool bDepassement, bool bRegimeFluide, const std::map<std::string, std::string> & additionalAttributes) = 0;

    // Ajout des donn�es du flux d'un v�hicule � l'instant consid�r�
    virtual void AddStream(int nID, const std::string &  strTuyau, const std::string &  strTuyauEx) = 0;

    // Ajout des donn�es d'un tuyau � l'instant consid�r�
    virtual void AddLink(const std::string &  strTuyau, double dbConcentration) = 0;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};
BOOST_SERIALIZATION_ASSUME_ABSTRACT(DocTrafic)
#endif
