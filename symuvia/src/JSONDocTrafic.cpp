#include "stdafx.h"
#include "JSONDocTrafic.h"

#include "symUtil.h"
#include "SystemUtil.h"
#include "vehicule.h"

#include <xercesc/dom/DOMNode.hpp>

XERCES_CPP_NAMESPACE_USE

JSONDocTrafic::JSONDocTrafic(void) :
m_TrafState(rapidjson::kObjectType)
{
}

JSONDocTrafic::~JSONDocTrafic(void) 
{
}

// Obtention des informations du trafic
rapidjson::Document & JSONDocTrafic::GetTrafficState()
{
	return m_TrafState;
}

// Initialisation
void JSONDocTrafic::Init() 
{
}

void JSONDocTrafic::PostInit() 
{

}

// Fin
void JSONDocTrafic::Terminate() 
{
}

// Suppression des fichiers
void JSONDocTrafic::Remove()
{
}

// Ajout d'un instant
void JSONDocTrafic::AddInstant(double dbInstant, double dbTimeStep, int nNbVeh) 
{
    m_TrafState = rapidjson::Document(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& allocator = m_TrafState.GetAllocator();
    rapidjson::Value stepValue(rapidjson::kObjectType);
    stepValue.AddMember("time", dbInstant, allocator);
    stepValue.AddMember("vehicule number", nNbVeh, allocator);
    m_TrafState.AddMember("step", stepValue, allocator);
}

// Ajout d'une période de surveillance des capteurs
void JSONDocTrafic::AddPeriodeCapteur(double dbDeb, double dbFin, const std::string & nodeName) 
{
}

JSONDocTrafic::PCAPTEUR JSONDocTrafic::AddInfoCapteur(const std::string & sIdCpt, const std::string & sTypeVeh, const std::string & sVitGlob, const std::string & sNbCumGlob, const std::string & sVitVoie, const std::string & sNbCumVoie, const std::string & sFlow, bool bExtractVehicleList) 
{
    if(sTypeVeh.empty())
    {
        rapidjson::Document::AllocatorType& allocator = m_TrafState.GetAllocator();
        rapidjson::Value sensorJson(rapidjson::kObjectType);
        rapidjson::Value strValue(rapidjson::kStringType);
        strValue.SetString(sIdCpt.c_str(), (int)sIdCpt.length(), allocator);
        sensorJson.AddMember("id", strValue, allocator);
        sensorJson.AddMember("speed", SystemUtil::ToDouble(sVitGlob), allocator);
        sensorJson.AddMember("flow", SystemUtil::ToDouble(sFlow), allocator);

        assert(m_TrafState.HasMember("step"));

        rapidjson::Value & step = m_TrafState["step"];
        if (!step.HasMember("sensors"))
        {
            rapidjson::Value sensors(rapidjson::kArrayType);
            sensors.PushBack(sensorJson, allocator);
            step.AddMember("sensors", sensors, allocator);
        }
        else
        {
            step["sensors"].PushBack(sensorJson, allocator);
        }
    }
	return NULL;
}

JSONDocTrafic::PCAPTEUR JSONDocTrafic::AddInfoCapteurLongitudinal(const std::string & sIdCpt, const std::string & sVoieOrigine, const std::string & sVoieDestination, const std::string & sCount)
{
    return NULL;
}

JSONDocTrafic::PCAPTEUR JSONDocTrafic::AddInfoCapteurEdie(const std::string & sIdCpt, const std::string & sConcentrationCum, const std::string & sDebitCum, const std::string & sConcentration, const std::string & sDebit)
{
    return NULL;
}

JSONDocTrafic::PCAPTEUR JSONDocTrafic::AddInfoCapteurMFD(const std::string & sIdCpt, bool bIncludeStrictData, const std::string & sGeomLength1, const std::string & sGeomLength2, const std::string & sGeomLength3,
        const std::string & sDistanceTotaleParcourue, const std::string & sDistanceTotaleParcourueStricte, const std::string & sTempsTotalPasse, const std::string & sTempsTotalPasseStrict,
        const std::string & sDebitSortie, const std::string &  sIntDebitSortie, const std::string &  sTransDebitSortie, const std::string & sDebitSortieStrict, const std::string & sLongueurDeplacement, const std::string & sLongueurDeplacementStricte,
        const std::string & sVitesseSpatiale, const std::string & sVitesseSpatialeStricte, const std::string & sConcentration, const std::string & sDebit)
{
    return NULL;
}

JSONDocTrafic::PCAPTEUR JSONDocTrafic::AddInfoCapteurReservoir(const std::string & sIdCpt)
{
    return NULL;
}

JSONDocTrafic::PCAPTEUR JSONDocTrafic::AddInfoCapteurBlueTooth(const std::string & sIdCpt, const BlueToothSensorData& data)
{
    return NULL;
}

JSONDocTrafic::PCAPTEUR JSONDocTrafic::AddInfoCapteurGlobal( const std::string & sIdVeh, double TpsEcoulement, double DistanceParcourue)
{
    return NULL;
}

JSONDocTrafic::PCAPTEUR JSONDocTrafic::AddIndicateursCapteurGlobal(double dbTotalTravelledDistance, double dbTotalTravelledTime, double dbVit, int nNbVeh)
{
	return NULL;
}

void JSONDocTrafic::AddVehCpt(JSONDocTrafic::PCAPTEUR xmlNodeCpt, const std::string & sId, double dbInstPsg, int nVoie) 
{
}

void JSONDocTrafic::AddTraverseeCpt(PCAPTEUR xmlNodeCpt, const std::string & vehId, const std::string & entryTime, const std::string & exitTime, const std::string & travelledDistance,
    const std::string & bCreatedInZone, const std::string & bDetroyedInZone)
{
}

// Sauvegarde d'un instant
void JSONDocTrafic::SaveLastInstant() 
{

}

// Suppression du dernier instant en mémoire DOM
void JSONDocTrafic::RemoveLastInstant() 
{

}

// Ajoute le contenu du document trafic de la source dans le document courant
void JSONDocTrafic::Add(DocTrafic * docTraficSrc) 
{
}

// Ajout d'une description d'une cellule de discrétisation
void JSONDocTrafic::AddCellule(int nID, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav) 
{

}

// Ajout d'une description d'un tronçon (défini par l'utilisateur ou construit par Symubruit)
void JSONDocTrafic::AddTroncon(const std::string & strLibelle, Point * pPtAm, Point * pPtAv, int nVoie, const std::string & ssBrique, std::deque<Point*> & lstPtInterne, double dbLong, std::vector<double> dbLarg, double dbStockMax, double dbStockInitial, Tuyau * pTuyau) 
{

}

// Ajout d'une description d'une voie (défini par l'utilisateur ou construit par Symubruit)
void JSONDocTrafic::AddVoie(Point * pPtAm, Point * pPtAv, std::deque<Point*> & lstPtInterne, double dbLong) 
{

}

// Ajout d'une description d'un capteur (de l'utilisateur ou créé par Symubruit)
void JSONDocTrafic::AddDefCapteur(const std::string & sId, const std::string & sT, double dbPos, Point ptCoord) 
{

}

// Ajout d'une description d'un capteur longitudinal (de l'utilisateur)
void JSONDocTrafic::AddDefCapteurLongitudinal(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut, double dbPosFin, Point ptCoordFin)
{

}

// Ajout d'une description d'un capteur Edie (de l'utilisateur)
void JSONDocTrafic::AddDefCapteurEdie(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut, double dbPosFin, Point ptCoordFin)
{

}

// Ajout d'une description d'un capteur MFD (de l'utilisateur)
void JSONDocTrafic::AddDefCapteurMFD(const std::string & sId, bool bIncludeStrictData, const std::vector<Tuyau*>& lstTuyaux)
{

}

// Ajout d'une description d'une entrée
void JSONDocTrafic::AddDefEntree(const std::string & sId, Point ptCoord) 
{

}

// Ajout d'une description d'une sortie
void JSONDocTrafic::AddDefSortie(const std::string & sId, Point ptCoord) 
{

}

// Ajout d'une description d'un CAF
void JSONDocTrafic::AddDefCAF(CarrefourAFeuxEx * pCAF)
{

}

// Ajout d'une description d'un Giratoire
void JSONDocTrafic::AddDefGir(Giratoire * pGir)
{

}

// Ajout d'une description d'un CDF
void JSONDocTrafic::AddDefCDF(ControleurDeFeux * pCDF)
{

}

// Ajout d'une description d'un arrêt de bus
void JSONDocTrafic::AddArret(const std::string & strLibelle, int numVoie, char * strNomLigne) 
{

}

// Ajout de la description d'un véhicule
void JSONDocTrafic::AddVehicule(int nID, const std::string & strLibelle, const std::string & strType, const std::string & strGMLType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string & strZoneEntree, const std::string & strZoneSortie, const std::string & strPlaqueEntree, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif, int iLane, const std::string & sLine, const std::vector<std::string> & initialPath, const std::deque<PlageAcceleration> & plagesAcc, const std::string & motifOrigine, const std::string & motifDestination, bool bAddToCreationNode)
{
    rapidjson::Document::AllocatorType& allocator = m_TrafState.GetAllocator();
    rapidjson::Value creationJson(rapidjson::kObjectType);
    creationJson.AddMember("id", nID, allocator);
    rapidjson::Value strTypeValue(rapidjson::kStringType);
    strTypeValue.SetString(strType.c_str(), (int)strType.length(), allocator);
    creationJson.AddMember("type", strTypeValue, allocator);
    creationJson.AddMember("speed max", dbVx, allocator);
    creationJson.AddMember("w", dbW, allocator);
    creationJson.AddMember("kx", dbKx, allocator);

    rapidjson::Value strTypeValueEntree(rapidjson::kStringType);
    strTypeValueEntree.SetString(strEntree.c_str(), (int)strEntree.length(), allocator);
    creationJson.AddMember("origin", strTypeValueEntree, allocator);
    rapidjson::Value strTypeValueDest(rapidjson::kStringType);
    strTypeValueDest.SetString(strSortie.c_str(), (int)strSortie.length(), allocator);
    creationJson.AddMember("destination", strTypeValueDest, allocator);
    creationJson.AddMember("load", 1, allocator);
    creationJson.AddMember("creation time", dbInstCreation, allocator);
    creationJson.AddMember("lane", iLane + 1, allocator);

    if(!sLine.empty())
    {
        rapidjson::Value strValueLine(rapidjson::kStringType);
        strValueLine.SetString(sLine.c_str(), (int)sLine.length(), allocator);
        creationJson.AddMember("line", strValueLine, allocator);
    }
    else
    {
        creationJson.AddMember("line", rapidjson::Value(rapidjson::kNullType), allocator);
    }

    rapidjson::Value initialPathJSON(rapidjson::kArrayType);
    for(size_t iTuy = 0; iTuy < initialPath.size(); iTuy++)
    {
        const std::string & tuyStr = initialPath[iTuy];
        rapidjson::Value tuyValue(rapidjson::kStringType);
        tuyValue.SetString(tuyStr.c_str(), (int)tuyStr.length(), allocator);
        initialPathJSON.PushBack(tuyValue, allocator);
    }
    creationJson.AddMember("initial path", initialPathJSON, allocator);

    rapidjson::Value accelerationRangesJSON(rapidjson::kArrayType);
    for(size_t iRange = 0; iRange < plagesAcc.size(); iRange++)
    {
        rapidjson::Value accRange(rapidjson::kObjectType);
        accRange.AddMember("ax", plagesAcc[iRange].dbAcc, allocator);
        if(iRange == plagesAcc.size()-1)
        {
            accRange.AddMember("upper speed", rapidjson::Value(rapidjson::kNullType), allocator);
        }
        else
        {
            accRange.AddMember("upper speed", plagesAcc[iRange].dbVitSup, allocator);
        }
        accelerationRangesJSON.PushBack(accRange, allocator);
    }
    creationJson.AddMember("acceleration ranges", accelerationRangesJSON, allocator);

    assert(m_TrafState.HasMember("step"));

    rapidjson::Value & step = m_TrafState["step"];
    if (!step.HasMember("creations"))
    {
        rapidjson::Value creations(rapidjson::kArrayType);
        creations.PushBack(creationJson, allocator);
        step.AddMember("creations", creations, allocator);
    }
    else
    {
        step["creations"].PushBack(creationJson, allocator);
    }
}


// ajout la description d'un véhicule sur sa création
void JSONDocTrafic::AddVehiculeToCreationNode(int nID, const std::string & strLibelle, const std::string & strType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif) 
{
    
}

// Ajout des sorties d'une brique de régulation
void JSONDocTrafic::AddSimuRegulation(DOMNode * pRestitutionNode)
{

}

// Ajout de la description des données trafic d'une entrée
void JSONDocTrafic::AddSimuEntree(const std::string & sEntree, int nVeh) 
{

}

// Ajout de la description des données trafic d'un parking
void JSONDocTrafic::AddSimuParking(const std::string & sParking, int nStock, int nVehAttente)
{

}

// Ajout de la description des données trafic d'un troncon de stationnement
void JSONDocTrafic::AddSimuTronconStationnement(const std::string & sTroncon, double dbLongueur)
{

}

void JSONDocTrafic::UpdateInstEntreeVehicule(int nID, double dbInstEntree)
{

}

// Mise à jour de l'instant de sortie du véhicule
void JSONDocTrafic::UpdateInstSortieVehicule(int nID, double dbInstSortie, const std::string & sSortie, const std::string & sPlaqueSortie, double dstParcourue, const std::vector<Tuyau*> & itinerary, const std::map<std::string, std::string> & additionalAttributes)
{
    rapidjson::Document::AllocatorType& allocator = m_TrafState.GetAllocator();

    rapidjson::Value deleteJson(rapidjson::kObjectType);
    deleteJson.AddMember("id", nID, allocator);
    deleteJson.AddMember("deletion time", dbInstSortie, allocator);

    rapidjson::Value exitValue(rapidjson::kStringType);
    exitValue.SetString(sSortie.c_str(), (int)sSortie.length(), allocator);
    deleteJson.AddMember("exit", exitValue, allocator);
	deleteJson.AddMember("length trip", dstParcourue, allocator);

    assert(m_TrafState.HasMember("step"));

    rapidjson::Value & step = m_TrafState["step"];
    if (!step.HasMember("deletions"))
    {
        rapidjson::Value deletions(rapidjson::kArrayType);
        deletions.PushBack(deleteJson, allocator);
        step.AddMember("deletions", deletions, allocator);
    }
    else
    {
        step["deletions"].PushBack(deleteJson, allocator);
    }
}

// Suppression des véhicules mémorisées à partir d'un certain ID
void JSONDocTrafic::RemoveVehicules(int nFromID) 
{

}

// Ajout de l'état des feux pour l'instant considéré
void JSONDocTrafic::AddSimFeux(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire) 
{
	// Sortie commentée car inutilisée pour l'instant et très verbeux
    /*rapidjson::Document::AllocatorType& allocator = m_TrafState.GetAllocator();

    rapidjson::Value lightJson(rapidjson::kObjectType);
    rapidjson::Value strValue(rapidjson::kStringType);
    strValue.SetString(sCtrlFeux.c_str(), (int)sCtrlFeux.length(), allocator);
    lightJson.AddMember("id", strValue, allocator);
    rapidjson::Value strSTE(rapidjson::kStringType);
    strSTE.SetString(sTE.c_str(), (int)sTE.length(), allocator);
    lightJson.AddMember("upstream link", strSTE, allocator);
    rapidjson::Value strSTS(rapidjson::kStringType);
    strSTS.SetString(sTS.c_str(), (int)sTS.length(), allocator);
    lightJson.AddMember("downstream link", strSTS, allocator);
    lightJson.AddMember("state", bEtatFeu, allocator);

    assert(m_TrafState.HasMember("step"));

    rapidjson::Value & step = m_TrafState["step"];
    if (!step.HasMember("traffic lights"))
    {
        rapidjson::Value trafficlights(rapidjson::kArrayType);
        trafficlights.PushBack(lightJson, allocator);
        step.AddMember("traffic lights", trafficlights, allocator);
    }
    else
    {
        step["traffic lights"].PushBack(lightJson, allocator);
    }*/
}

// Ajout de l'état des feux pour l'instant considéré pour EVE
void JSONDocTrafic::AddSimFeuxEVE(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire) 
{

}

// Ajout des données complète trafic d'une cellule de discrétisation pour l'instant considéré
void JSONDocTrafic::AddCellSimu(int nID, double dbConc, double dbDebit, double dbVitAm, double dbAccAm, double dbNAm, double dbVitAv, double dbAccAv, double dbNAv, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav) 
{
}

// Ajout des données de la trajectoire d'un véhicule à l'instant considéré
void JSONDocTrafic::AddTrajectoire(int nID, Tuyau * pTuyau, const std::string & strTuyau, const std::string & strTuyauEx, const std::string & strNextTuyauEx, int nNumVoie, double dbAbs, double dbOrd, double dbZ, double dbAbsCur, double dbVit, double dbAcc, double dbDeltaN, const std::string & sTypeVehicule, double dbVitMax, double dbLongueur, const std::string & sLib, int nIDVehLeader, int nCurrentLoad, bool bTypeChgtVoie, TypeChgtVoie eTypeChgtVoie,
    bool bVoieCible, int nVoieCible, bool bPi, double dbPi, bool bPhi, double dbPhi, bool bRand, double dbRand, bool bDriven, const std::string & strDriveState, bool bDepassement, bool bRegimeFluide, const std::map<std::string, std::string> & additionalAttributes) 
{
    rapidjson::Document::AllocatorType& allocator = m_TrafState.GetAllocator();

    rapidjson::Value trajectoryJson(rapidjson::kObjectType);
    trajectoryJson.AddMember("load", 1, allocator);
    trajectoryJson.AddMember("id", nID, allocator);
    rapidjson::Value tuyauValue(rapidjson::kStringType);
    tuyauValue.SetString(strTuyau.c_str(), (int)strTuyau.length(), allocator);
    trajectoryJson.AddMember("link", tuyauValue, allocator);
    trajectoryJson.AddMember("lane", nNumVoie, allocator);
    trajectoryJson.AddMember("relative position", dbAbsCur, allocator);
    rapidjson::Value positionJson(rapidjson::kObjectType);
    positionJson.AddMember("type", "Point", allocator);
    rapidjson::Value coordinatesJson(rapidjson::kArrayType);
    coordinatesJson.PushBack(dbAbs, allocator);
    coordinatesJson.PushBack(dbOrd, allocator);
    positionJson.AddMember("coordinates", coordinatesJson, allocator);
    trajectoryJson.AddMember("position", positionJson, allocator);
	trajectoryJson.AddMember("speed", dbVit, allocator);

    assert(m_TrafState.HasMember("step"));

    rapidjson::Value & step = m_TrafState["step"];
    if (!step.HasMember("trajectories"))
    {
        rapidjson::Value trajectories(rapidjson::kArrayType);
        trajectories.PushBack(trajectoryJson, allocator);
        step.AddMember("trajectories", trajectories, allocator);
    }
    else
    {
        step["trajectories"].PushBack(trajectoryJson, allocator);
    }
}

// Ajout des données du flux d'un véhicule à l'instant considéré
void JSONDocTrafic::AddStream(int nID, const std::string & strTuyau, const std::string & strTuyauEx )
{	
}

// Ajout des données d'un tuyau à l'instant considéré
void JSONDocTrafic::AddLink(const std::string &  strTuyau, double dbConcentration)
{
}