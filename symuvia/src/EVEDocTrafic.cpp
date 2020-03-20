#include "stdafx.h"
#include "EVEDocTrafic.h"

#include "symUtil.h"
#include "SystemUtil.h"

#include <xercesc/dom/DOMNode.hpp>

XERCES_CPP_NAMESPACE_USE

EVEDocTrafic::EVEDocTrafic(void) 
{

	m_pTrafState = NULL;
}

EVEDocTrafic::~EVEDocTrafic(void) 
{

	if (m_pTrafState != NULL)
	{
		delete m_pTrafState;
	}
}

// Obtention des informations du trafic
eveShared::TrafficState * EVEDocTrafic::GetTrafficState() 
{

	return m_pTrafState;
}

// Initialisation
void EVEDocTrafic::Init() 
{

	m_pTrafState = new eveShared::TrafficState();
}

void EVEDocTrafic::PostInit() 
{

}

// Fin
void EVEDocTrafic::Terminate() 
{

}

// Suppression du fichier
void EVEDocTrafic::Remove()
{

}

// Ajout d'un instant
void EVEDocTrafic::AddInstant(double dbInstant, double dbTimeStep, int nNbVeh) 
{

	if (m_pTrafState != NULL)
	{
		delete m_pTrafState;
	}
	m_pTrafState = new eveShared::TrafficState();
	m_pTrafState->inst = dbInstant;
}

// Ajout d'une période de surveillance des capteurs
void EVEDocTrafic::AddPeriodeCapteur(double dbDeb, double dbFin, const std::string & nodeName) 
{

}

EVEDocTrafic::PCAPTEUR EVEDocTrafic::AddInfoCapteur(const std::string & sIdCpt, const std::string & sTypeVeh, const std::string & sVitGlob, const std::string & sNbCumGlob, const std::string & sVitVoie, const std::string & sNbCumVoie, const std::string & sFlow, bool bExtractVehicleList) 
{

	eveShared::Sensor * pSensor = new eveShared::Sensor();
	pSensor->nbcum_g = SystemUtil::ToDouble(sNbCumGlob);
	pSensor->nbcum_v = SystemUtil::ToDouble(sNbCumVoie);
	pSensor->type = sTypeVeh;
	pSensor->vit_g = SystemUtil::ToDouble(sVitGlob);
	pSensor->vit_v = SystemUtil::ToDouble(sVitVoie);
	m_pTrafState->Sensors.push_back(pSensor);
	return NULL;
}

EVEDocTrafic::PCAPTEUR EVEDocTrafic::AddInfoCapteurLongitudinal(const std::string & sIdCpt, const std::string & sVoieOrigine, const std::string & sVoieDestination, const std::string & sCount)
{
    return NULL;
}

EVEDocTrafic::PCAPTEUR EVEDocTrafic::AddInfoCapteurEdie(const std::string & sIdCpt, const std::string & sConcentrationCum, const std::string & sDebitCum, const std::string & sConcentration, const std::string & sDebit)
{
    return NULL;
}

EVEDocTrafic::PCAPTEUR EVEDocTrafic::AddInfoCapteurMFD(const std::string & sIdCpt, bool bIncludeStrictData, const std::string & sGeomLength1, const std::string & sGeomLength2, const std::string & sGeomLength3,
        const std::string & sDistanceTotaleParcourue, const std::string & sDistanceTotaleParcourueStricte, const std::string & sTempsTotalPasse, const std::string & sTempsTotalPasseStrict,
        const std::string & sDebitSortie, const std::string &  sIntDebitSortie, const std::string &  sTransDebitSortie, const std::string & sDebitSortieStrict, const std::string & sLongueurDeplacement, const std::string & sLongueurDeplacementStricte,
        const std::string & sVitesseSpatiale, const std::string & sVitesseSpatialeStricte, const std::string & sConcentration, const std::string & sDebit)
{
    return NULL;
}

EVEDocTrafic::PCAPTEUR EVEDocTrafic::AddInfoCapteurReservoir(const std::string & sIdCpt)
{
    return NULL;
}

EVEDocTrafic::PCAPTEUR EVEDocTrafic::AddInfoCapteurBlueTooth(const std::string & sIdCpt, const BlueToothSensorData& data)
{
    return NULL;
}

EVEDocTrafic::PCAPTEUR EVEDocTrafic::AddInfoCapteurGlobal( const std::string & sIdVeh, double TpsEcoulement, double DistanceParcourue)
{
    return NULL;
}

EVEDocTrafic::PCAPTEUR EVEDocTrafic::AddIndicateursCapteurGlobal(double dbTotalTravelledDistance, double dbTotalTravelledTime, double dbVit, int nNbVeh)
{
	return NULL;
}


void EVEDocTrafic::AddVehCpt(EVEDocTrafic::PCAPTEUR xmlNodeCpt, const std::string & sId, double dbInstPsg, int nVoie) 
{
}

void EVEDocTrafic::AddTraverseeCpt(PCAPTEUR xmlNodeCpt, const std::string & vehId, const std::string & entryTime, const std::string & exitTime, const std::string & travelledDistance,
    const std::string & bCreatedInZone, const std::string & bDetroyedInZone)
{
}

// Sauvegarde d'un instant
void EVEDocTrafic::SaveLastInstant() 
{

}

// Suppression du dernier instant en mémoire DOM
void EVEDocTrafic::RemoveLastInstant() 
{

}

// Ajoute le contenu du document trafic de la source dans le document courant
void EVEDocTrafic::Add(DocTrafic * docTraficSrc) 
{

	EVEDocTrafic * Trafic = (EVEDocTrafic*)docTraficSrc;
	eveShared::TrafficState::Copy(Trafic->GetTrafficState(), m_pTrafState);
}

// Ajout d'une description d'une cellule de discrétisation
void EVEDocTrafic::AddCellule(int nID, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav) 
{

}

// Ajout d'une description d'un tronçon (défini par l'utilisateur ou construit par Symubruit)
void EVEDocTrafic::AddTroncon(const std::string & strLibelle, Point * pPtAm, Point * pPtAv, int nVoie, const std::string & ssBrique, std::deque<Point*> & lstPtInterne, double dbLong, std::vector<double> dbLarg, double dbStockMax, double dbStockInitial, Tuyau * pTuyau) 
{

}

// Ajout d'une description d'une voie (défini par l'utilisateur ou construit par Symubruit)
void EVEDocTrafic::AddVoie(Point * pPtAm, Point * pPtAv, std::deque<Point*> & lstPtInterne, double dbLong) 
{

}

// Ajout d'une description d'un capteur (de l'utilisateur ou créé par Symubruit)
void EVEDocTrafic::AddDefCapteur(const std::string & sId, const std::string & sT, double dbPos, Point ptCoord) 
{

}

// Ajout d'une description d'un capteur longitudinal (de l'utilisateur)
void EVEDocTrafic::AddDefCapteurLongitudinal(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut, double dbPosFin, Point ptCoordFin)
{

}

// Ajout d'une description d'un capteur Edie (de l'utilisateur)
void EVEDocTrafic::AddDefCapteurEdie(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut, double dbPosFin, Point ptCoordFin)
{

}

// Ajout d'une description d'un capteur MFD (de l'utilisateur)
void EVEDocTrafic::AddDefCapteurMFD(const std::string & sId, bool bIncludeStrictData, const std::vector<Tuyau*>& lstTuyaux)
{

}

// Ajout d'une description d'une entrée
void EVEDocTrafic::AddDefEntree(const std::string & sId, Point ptCoord) 
{

}

// Ajout d'une description d'une sortie
void EVEDocTrafic::AddDefSortie(const std::string & sId, Point ptCoord) 
{

}

// Ajout d'une description d'un CAF
void EVEDocTrafic::AddDefCAF(CarrefourAFeuxEx * pCAF)
{

}

// Ajout d'une description d'un Giratoire
void EVEDocTrafic::AddDefGir(Giratoire * pGir)
{

}

// Ajout d'une description d'un CDF
void EVEDocTrafic::AddDefCDF(ControleurDeFeux * pCDF)
{

}

// Ajout d'une description d'un arrêt de bus
void EVEDocTrafic::AddArret(const std::string & strLibelle, int numVoie, char * strNomLigne) 
{

}

// Ajout de la description d'un véhicule
void EVEDocTrafic::AddVehicule(int nID, const std::string & strLibelle, const std::string & strType, const std::string & strGMLType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string &  strZoneEntree, const std::string &  strZoneSortie, const std::string & strPlaqueEntree, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif, int iLane, const std::string & sLine, const std::vector<std::string> & initialPath, const std::deque<PlageAcceleration> & plagesAcc, const std::string & motifOrigine, const std::string & motifDestination, bool bAddToCreationNode)
{

}


// ajout la description d'un véhicule sur sa création
void EVEDocTrafic::AddVehiculeToCreationNode(int nID, const std::string & strLibelle, const std::string & strType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif) 
{
}

// ajout la description d'un véhicule lors de la sortie d'un véhicule du réseau
void AddVehiculeToSortieNode(int nID, const std::string & strSortie, double dbInstSortie, double dbVit)
{
}

// Ajout des sorties d'une brique de régulation
void EVEDocTrafic::AddSimuRegulation(DOMNode * pRestitutionNode)
{

}

// Ajout de la description des données trafic d'une entrée
void EVEDocTrafic::AddSimuEntree(const std::string & sEntree, int nVeh) 
{

}

// Ajout de la description des données trafic d'un parking
void EVEDocTrafic::AddSimuParking(const std::string & sParking, int nStock, int nVehAttente)
{

}

// Ajout de la description des données trafic d'un troncon de stationnement
void EVEDocTrafic::AddSimuTronconStationnement(const std::string & sTroncon, double dbLongueur)
{

}

void EVEDocTrafic::UpdateInstEntreeVehicule(int nID, double dbInstEntree)
{

}

// Mise à jour de l'instant de sortie du véhicule
void EVEDocTrafic::UpdateInstSortieVehicule(int nID, double dbInstSortie, const std::string & sSortie, const std::string & sPlaqueSortie, double dstParcourue, const std::vector<Tuyau*> & itinerary, const std::map<std::string, std::string> & additionalAttributes)
{

}

// Suppression des véhicules mémorisées à partir d'un certain ID
void EVEDocTrafic::RemoveVehicules(int nFromID) 
{

}

// Ajout de l'état des feux pour l'instant considéré
void EVEDocTrafic::AddSimFeux(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire) 
{

}

// Ajout de l'état des feux pour l'instant considéré pour EVE
void EVEDocTrafic::AddSimFeuxEVE(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire) 
{
	eveShared::TrafficLight * pTL = new eveShared::TrafficLight();

	pTL->ctrl_feux = sCtrlFeux;
	pTL->etat = (bEtatFeu != 0);
	pTL->troncon_entree = sTE;
	pTL->troncon_sortie = sTS;

	m_pTrafState->TrafficLights.push_back(pTL);
}

// Ajout des données complète trafic d'une cellule de discrétisation pour l'instant considéré
void EVEDocTrafic::AddCellSimu(int nID, double dbConc, double dbDebit, double dbVitAm, double dbAccAm, double dbNAm, double dbVitAv, double dbAccAv, double dbNAv, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav) 
{

	eveShared::Segment * pSeg = new eveShared::Segment();

	pSeg->accAm = dbAccAm;
	pSeg->accAv = dbAccAv;
	pSeg->conc = dbConc;
	pSeg->deb = dbDebit;
	pSeg->id = nID;
	pSeg->lib.empty();
	pSeg->Nam = (int)dbNAm;
	pSeg->Nav = (int)dbNAv;
	pSeg->tron.empty();
	pSeg->vitAm = dbVitAm;
	pSeg->vitAv = dbVitAv;
	pSeg->Xam = dbXam;
	pSeg->Xav = dbXav;
	pSeg->Yam = dbYam;
	pSeg->Yav = dbYam;
	pSeg->Zam = dbZam;
	pSeg->Zav = dbZav;
	m_pTrafState->Segments.push_back(pSeg);
}

// Ajout des données de la trajectoire d'un véhicule à l'instant considéré
void EVEDocTrafic::AddTrajectoire(int nID, Tuyau * pTuyau, const std::string & strTuyau, const std::string & strTuyauEx, const std::string & strNextTuyauEx, int nNumVoie, double dbAbs, double dbOrd, double dbZ, double dbAbsCur, double dbVit, double dbAcc, double dbDeltaN, const std::string & sTypeVehicule, double dbVitMax, double dbLongueur, const std::string & sLib, int nIDVehLeader, int nCurrentLoad, bool bTypeChgtVoie, TypeChgtVoie eTypeChgtVoie,
    bool bVoieCible, int nVoieCible, bool bPi, double dbPi, bool bPhi, double dbPhi, bool bRand, double dbRand, bool bDriven, const std::string & strDriveState, bool bDepassement, bool bRegimeFluide, const std::map<std::string, std::string> & additionalAttributes) 
{

	eveShared::Trajectory * pTraj = new eveShared::Trajectory();
	pTraj->abs = dbAbs;
	pTraj->acc = dbAcc;
	pTraj->dst = dbAbsCur;
	pTraj->id = nID;
	pTraj->ord = dbOrd;
	pTraj->tron = strTuyauEx;
    pTraj->nextTron = strNextTuyauEx;
	pTraj->type = sTypeVehicule;
    pTraj->vitMax = dbVitMax;
    pTraj->lon = dbLongueur;
	pTraj->vit = dbVit;
	pTraj->z = dbZ;
	//m_pTrafState->Trajectories[nNumVoie] = pTraj;
	m_pTrafState->Trajectories[nID] = pTraj;
}

// Ajout des données du flux d'un véhicule à l'instant considéré
void EVEDocTrafic::AddStream(int nID, const std::string & strTuyau, const std::string & strTuyauEx )
{	
    eveShared::Stream * pStream = new eveShared::Stream();
	pStream->id = nID;
	pStream->tron = strTuyauEx;
	//m_pTrafState->Trajectories[nNumVoie] = pTraj;
	m_pTrafState->Streams[nID] = pStream;

}

// Ajout des données d'un tuyau à l'instant considéré
void EVEDocTrafic::AddLink(const std::string &  strTuyau, double dbConcentration)
{

}