#include "stdafx.h"
#include "TraceDocTrafic.h"

#include "EVEDocTrafic.h"
#include "XMLDocTrafic.h"
#include "GMLDocTrafic.h"
#include "JSONDocTrafic.h"

#include "symUtil.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>
#include <boost/serialization/vector.hpp>


TraceDocTrafic::TraceDocTrafic()
{
    m_pEveDocTrafic = NULL;
    m_pXMLDocTrafic = NULL;
    m_pGMLDocTrafic = NULL;
    m_pJSONDocTrafic = NULL;
}

TraceDocTrafic::TraceDocTrafic(Reseau * pNetwork, bool bEnableXMLDoc, bool bWriteXml, bool bDebug, bool bTraceStocks, bool bSortieLight, bool bSortieRegime, XERCES_CPP_NAMESPACE::DOMDocument * pXMLDocument,
    bool bEVEOutput, bool bGMLOutput, bool JSONOutput)
{
    if(JSONOutput)
    {
	    m_pJSONDocTrafic = new JSONDocTrafic();
        m_LstDocTrafic.push_back(m_pJSONDocTrafic);
    }
    else
    {
        m_pJSONDocTrafic = NULL;
    }
    if(bGMLOutput)
    {
	    m_pGMLDocTrafic = new GMLDocTrafic();
        m_LstDocTrafic.push_back(m_pGMLDocTrafic);
    }
    else
    {
        m_pGMLDocTrafic = NULL;
    }
    if(bEVEOutput)
    {
	    m_pEveDocTrafic = new EVEDocTrafic();
        m_LstDocTrafic.push_back(m_pEveDocTrafic);
    }
    else
    {
        m_pEveDocTrafic = NULL;
    }
    if(bEnableXMLDoc)
    {
        m_pXMLDocTrafic = new XMLDocTrafic(pNetwork, pXMLDocument);
        m_LstDocTrafic.push_back(m_pXMLDocTrafic);

        m_pXMLDocTrafic->setSave(bWriteXml);
        m_pXMLDocTrafic->setDebug(bDebug);
        m_pXMLDocTrafic->setTraceStocks(bTraceStocks);
        m_pXMLDocTrafic->setSortieLight(bSortieLight);
        m_pXMLDocTrafic->setSortieRegime(bSortieRegime);
    }
    else
    {
        m_pXMLDocTrafic = NULL;
    }
}

TraceDocTrafic::~TraceDocTrafic()
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        delete m_LstDocTrafic[iDoc];
    }
}

XMLDocTrafic * TraceDocTrafic::GetXMLDocTrafic()
{
    return m_pXMLDocTrafic;
}
    
EVEDocTrafic * TraceDocTrafic::GetEVEDocTrafic()
{
    return m_pEveDocTrafic;
}

GMLDocTrafic * TraceDocTrafic::GetGMLDocTrafic()
{
    return m_pGMLDocTrafic;
}

JSONDocTrafic * TraceDocTrafic::GetJSONDocTrafic()
{
    return m_pJSONDocTrafic;
}

std::string TraceDocTrafic::GetFileName()
{
    return m_strFileName;
}

void TraceDocTrafic::Init(const std::string & strFilename, const std::string & strVer, SDateTime dtDeb, SDateTime dtFin, double dbPerAgrCpts, XERCES_CPP_NAMESPACE::DOMDocument * xmlDocReseau, unsigned int uiInit, const std::string & simulationID, const std::string & traficID, const std::string & reseauID,
    const std::vector<Point> & enveloppe, TraceDocTrafic * pOriginalDocTrafic, OGRCoordinateTransformation *pCoordTransf, bool bHasAtLeastOneCDF, bool bHasSensor)
{
    m_strFileName = strFilename;
    if(m_pXMLDocTrafic)
	    m_pXMLDocTrafic->Init(strFilename, strVer, dtDeb, dtFin, dbPerAgrCpts, xmlDocReseau, uiInit, simulationID, traficID, reseauID, pCoordTransf);
	if (m_pEveDocTrafic)
		m_pEveDocTrafic->Init();
    if (m_pGMLDocTrafic)
		m_pGMLDocTrafic->Init(strFilename, dtDeb, dtFin, enveloppe, pOriginalDocTrafic != NULL, pOriginalDocTrafic==NULL?strFilename:pOriginalDocTrafic->GetFileName(), pOriginalDocTrafic?pOriginalDocTrafic->GetGMLDocTrafic():NULL, bHasAtLeastOneCDF, bHasSensor);
    if (m_pJSONDocTrafic)
        m_pJSONDocTrafic->Init();
}

void TraceDocTrafic::PostInit()
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->PostInit();
    }
}

void TraceDocTrafic::Terminate()
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->Terminate();
    }
}

void TraceDocTrafic::Remove()
{
    for (size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->Remove();
    }
}

void TraceDocTrafic::AddInstant(double dbInstant, double dbTimeStep, int nNbVeh)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddInstant(dbInstant, dbTimeStep, nNbVeh);
    }
}

void TraceDocTrafic::AddInstantXML(double dbInstant, double dbTimeStep, int nNbVeh)
{
    if(m_pXMLDocTrafic)
    {
        m_pXMLDocTrafic->AddInstant(dbInstant, dbTimeStep, nNbVeh);
    }
}

void TraceDocTrafic::AddPeriodeCapteur(double dbDeb, double dbFin, const std::string& nodeName)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddPeriodeCapteur(dbDeb, dbFin, nodeName);
    }
}

DocTrafic::PCAPTEUR TraceDocTrafic::AddInfoCapteurLongitudinal(const std::string & sIdCpt, const std::string & sVoieOrigine, const std::string & sVoieDestination, const std::string & sCount)
{
    TraceDocTrafic::PCAPTEUR pCapteur = NULL;
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        TraceDocTrafic::PCAPTEUR pCapteurTmp = m_LstDocTrafic[iDoc]->AddInfoCapteurLongitudinal(sIdCpt, sVoieOrigine, sVoieDestination, sCount);
        if(m_LstDocTrafic[iDoc] == m_pXMLDocTrafic)
        {
            pCapteur = pCapteurTmp;
        }
    }
    return pCapteur;
}

DocTrafic::PCAPTEUR TraceDocTrafic::AddInfoCapteurEdie(const std::string & sIdCpt, const std::string & sConcentrationCum, const std::string & sDebitCum, const std::string & sConcentration, const std::string & sDebit)
{
    TraceDocTrafic::PCAPTEUR pCapteur = NULL;
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        TraceDocTrafic::PCAPTEUR pCapteurTmp = m_LstDocTrafic[iDoc]->AddInfoCapteurEdie(sIdCpt, sConcentrationCum, sDebitCum, sConcentration, sDebit);
        if(m_LstDocTrafic[iDoc] == m_pXMLDocTrafic)
        {
            pCapteur = pCapteurTmp;
        }
    }
    return pCapteur;
}

DocTrafic::PCAPTEUR TraceDocTrafic::AddInfoCapteurMFD(const std::string & sIdCpt, bool bIncludeStrictData, const std::string & sGeomLength1, const std::string & sGeomLength2, const std::string & sGeomLength3,
        const std::string & sDistanceTotaleParcourue, const std::string & sDistanceTotaleParcourueStricte, const std::string & sTempsTotalPasse, const std::string & sTempsTotalPasseStrict,
        const std::string & sDebitSortie, const std::string &  sIntDebitSortie, const std::string &  sTransDebitSortie, const std::string & sDebitSortieStrict, const std::string & sLongueurDeplacement, const std::string & sLongueurDeplacementStricte,
        const std::string & sVitesseSpatiale, const std::string & sVitesseSpatialeStricte, const std::string & sConcentration, const std::string & sDebit)
{
    TraceDocTrafic::PCAPTEUR pCapteur = NULL;
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        TraceDocTrafic::PCAPTEUR pCapteurTmp = m_LstDocTrafic[iDoc]->AddInfoCapteurMFD(sIdCpt, bIncludeStrictData, sGeomLength1, sGeomLength2, sGeomLength3,
        sDistanceTotaleParcourue, sDistanceTotaleParcourueStricte, sTempsTotalPasse, sTempsTotalPasseStrict,
        sDebitSortie, sIntDebitSortie, sTransDebitSortie, sDebitSortieStrict, sLongueurDeplacement, sLongueurDeplacementStricte,
        sVitesseSpatiale, sVitesseSpatialeStricte, sConcentration, sDebit);
        if(m_LstDocTrafic[iDoc] == m_pXMLDocTrafic)
        {
            pCapteur = pCapteurTmp;
        }
    }
    return pCapteur;
}

DocTrafic::PCAPTEUR TraceDocTrafic::AddInfoCapteurReservoir(const std::string & sIdCpt)
{
    TraceDocTrafic::PCAPTEUR pCapteur = NULL;
    for (size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        TraceDocTrafic::PCAPTEUR pCapteurTmp = m_LstDocTrafic[iDoc]->AddInfoCapteurReservoir(sIdCpt);
        if (m_LstDocTrafic[iDoc] == m_pXMLDocTrafic)
        {
            pCapteur = pCapteurTmp;
        }
    }
    return pCapteur;
}

DocTrafic::PCAPTEUR TraceDocTrafic::AddIndicateursCapteurGlobal(double dbTotalTravelledDistance, double dbTotalTravelledTime, double dbVit, int nNbVeh)
{
    TraceDocTrafic::PCAPTEUR pCapteur = NULL;
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        TraceDocTrafic::PCAPTEUR pCapteurTmp = m_LstDocTrafic[iDoc]->AddIndicateursCapteurGlobal(dbTotalTravelledDistance, dbTotalTravelledTime, dbVit, nNbVeh);
        if(m_LstDocTrafic[iDoc] == m_pXMLDocTrafic)
        {
            pCapteur = pCapteurTmp;
        }
    }
    return pCapteur;
}

DocTrafic::PCAPTEUR TraceDocTrafic::AddInfoCapteurBlueTooth(const std::string & sIdCpt, const BlueToothSensorData& data)
{
    TraceDocTrafic::PCAPTEUR pCapteur = NULL;
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        TraceDocTrafic::PCAPTEUR pCapteurTmp = m_LstDocTrafic[iDoc]->AddInfoCapteurBlueTooth(sIdCpt, data);
        if(m_LstDocTrafic[iDoc] == m_pXMLDocTrafic)
        {
            pCapteur = pCapteurTmp;
        }
    }
    return pCapteur;
}

TraceDocTrafic::PCAPTEUR TraceDocTrafic::AddInfoCapteur(const std::string & sIdCpt, const std::string & sTypeVeh, const std::string & sVitGlob, const std::string & sNbCumGlob, const std::string & sVitVoie, const std::string & sNbCumVoie, const std::string & sFlow, bool bExtractVehicleList)
{
	TraceDocTrafic::PCAPTEUR pCapteur = NULL;
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        TraceDocTrafic::PCAPTEUR pCapteurTmp = m_LstDocTrafic[iDoc]->AddInfoCapteur(sIdCpt, sTypeVeh, sVitGlob, sNbCumGlob, sVitVoie, sNbCumVoie, sFlow, bExtractVehicleList);
        if(m_LstDocTrafic[iDoc] == m_pXMLDocTrafic)
        {
            pCapteur = pCapteurTmp;
        }
    }
	return pCapteur;
}

TraceDocTrafic::PCAPTEUR TraceDocTrafic::AddInfoCapteurGlobal( const std::string & sIdVeh, double TpsEcoulement, double DistanceParcourue)
{
	TraceDocTrafic::PCAPTEUR pCapteur = NULL;
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        TraceDocTrafic::PCAPTEUR pCapteurTmp = m_LstDocTrafic[iDoc]->AddInfoCapteurGlobal(sIdVeh,  TpsEcoulement,  DistanceParcourue);
        if(m_LstDocTrafic[iDoc] == m_pXMLDocTrafic)
        {
            pCapteur = pCapteurTmp;
        }
    }
	return pCapteur;
}

void TraceDocTrafic::AddVehCpt(DocTrafic::PCAPTEUR xmlNodeCpt, const std::string & sId, double dbInstPsg, int nVoie)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddVehCpt(xmlNodeCpt, sId, dbInstPsg, nVoie);
    }
}

void TraceDocTrafic::AddTraverseeCpt(PCAPTEUR xmlNodeCpt, const std::string & vehId, const std::string & entryTime, const std::string & exitTime,
    const std::string & travelledDistance, const std::string & bCreatedInZone, const std::string & bDetroyedInZone)
{
    for (size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddTraverseeCpt(xmlNodeCpt, vehId, entryTime, exitTime, travelledDistance, bCreatedInZone, bDetroyedInZone);
    }
}

void TraceDocTrafic::SaveLastInstant()
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->SaveLastInstant();
    }
}

void TraceDocTrafic::RemoveLastInstant()
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->RemoveLastInstant();
    }
}

void TraceDocTrafic::Add(DocTrafic * docTraficSrc)
{
    TraceDocTrafic * pTraceDoc = (TraceDocTrafic*) docTraficSrc;
    for(size_t iDoc = 0; iDoc < pTraceDoc->m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->Add(pTraceDoc->m_LstDocTrafic[iDoc]);
    }
}

void TraceDocTrafic::AddCellule(int nID, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddCellule(nID, strLibelle, strTuyau, dbXam, dbYam, dbZam, dbXav, dbYav, dbZav);
    }
}

void TraceDocTrafic::AddTroncon(const std::string & strLibelle, Point * pPtAm, Point * pPtAv, int nVoie, const std::string & ssBrique, std::deque<Point*> &lstPtInterne, double dbLong, std::vector<double> dbLarg, double dbStockMax, double dbStockInitial, Tuyau * pTuyau)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddTroncon(strLibelle, pPtAm, pPtAv, nVoie, ssBrique, lstPtInterne, dbLong, dbLarg, dbStockMax, dbStockInitial, pTuyau);
    }
}

void TraceDocTrafic::AddVoie(Point * pPtAm, Point * pPtAv, std::deque<Point*> &lstPtInterne, double dbLong)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddVoie(pPtAm, pPtAv, lstPtInterne, dbLong);
    }
}

void TraceDocTrafic::AddDefCapteur(const std::string & sId, const std::string & sT, double dbPos, Point ptCoord)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddDefCapteur(sId, sT, dbPos, ptCoord);
    }
}

void TraceDocTrafic::AddDefCapteurLongitudinal(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut, double dbPosFin, Point ptCoordFin)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddDefCapteurLongitudinal(sId, sT, dbPosDebut, ptCoordDebut, dbPosFin, ptCoordFin);
    }
}

void TraceDocTrafic::AddDefCapteurEdie(const std::string & sId, const std::string & sT, double dbPosDebut, Point ptCoordDebut, double dbPosFin, Point ptCoordFin)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddDefCapteurEdie(sId, sT, dbPosDebut, ptCoordDebut, dbPosFin, ptCoordFin);
    }
}

void TraceDocTrafic::AddDefCapteurMFD(const std::string & sId, bool bIncludeStrictData, const std::vector<Tuyau*>& lstTuyaux)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddDefCapteurMFD(sId, bIncludeStrictData, lstTuyaux);
    }
}

void TraceDocTrafic::AddDefEntree(const std::string & sId, Point ptCoord)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddDefEntree(sId, ptCoord);
    }
}

void TraceDocTrafic::AddDefSortie(const std::string & sId, Point ptCoord)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddDefSortie(sId, ptCoord);
    }
}

void TraceDocTrafic::AddDefCAF(CarrefourAFeuxEx * pCAF)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddDefCAF(pCAF);
    }
}

void TraceDocTrafic::AddDefGir(Giratoire * pGir)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddDefGir(pGir);
    }
}

void TraceDocTrafic::AddDefCDF(ControleurDeFeux * pCDF)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddDefCDF(pCDF);
    }
}

void TraceDocTrafic::AddArret(const std::string & strLibelle, int numVoie, char * strNomLigne)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddArret(strLibelle, numVoie, strNomLigne);
    }
}

void TraceDocTrafic::AddVehicule(int nID, const std::string & strLibelle, const std::string & strType, const std::string & strGMLType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string & strZoneEntree, const std::string & strZoneSortie, const std::string & strPlaqueEntree, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif, int iLane, const std::string & sLine, const std::vector<std::string> & initialPath, const std::deque<PlageAcceleration> & plagesAcc, const std::string & motifOrigine, const std::string & motifDestination, bool bAddToCreationNode)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddVehicule(nID, strLibelle, strType, strGMLType, dbKx, dbVx, dbW, strEntree, strSortie, strZoneEntree, strZoneSortie, strPlaqueEntree, strRoute, dbInstCreation, sVoie, bAgressif, iLane, sLine, initialPath, plagesAcc, motifOrigine, motifDestination, bAddToCreationNode);
    }
}

void TraceDocTrafic::AddVehiculeToCreationNode(int nID, const std::string & strLibelle, const std::string & strType, double dbKx, double dbVx, double dbW, const std::string & strEntree, const std::string & strSortie, const std::string & strRoute, double dbInstCreation, const std::string & sVoie, bool bAgressif)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddVehiculeToCreationNode(nID, strLibelle, strType, dbKx, dbVx, dbW, strEntree, strSortie, strRoute, dbInstCreation, sVoie, bAgressif);
    }
}

void TraceDocTrafic::AddSimuRegulation(XERCES_CPP_NAMESPACE::DOMNode * pRestitutionNode)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddSimuRegulation(pRestitutionNode);
    }
}

void TraceDocTrafic::AddSimuEntree(const std::string & sEntree, int nVeh)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddSimuEntree(sEntree, nVeh);
    }
}

void TraceDocTrafic::AddSimuParking(const std::string & sParking, int nStock, int nVehAttente)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddSimuParking(sParking, nStock, nVehAttente);
    }
}

void TraceDocTrafic::AddSimuTronconStationnement(const std::string & sTroncon, double dbLongueur)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddSimuTronconStationnement(sTroncon, dbLongueur);
    }
}

void TraceDocTrafic::UpdateInstEntreeVehicule(int nID, double dbInstEntree)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->UpdateInstEntreeVehicule(nID, dbInstEntree);
    }
}

void TraceDocTrafic::UpdateInstSortieVehicule(int nID, double dbInstSortie, const std::string & sSortie, const std::string & sPlaqueSortie, double dstParcourue, const std::vector<Tuyau*> & itinerary, const std::map<std::string, std::string> & additionalAttributes)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->UpdateInstSortieVehicule(nID, dbInstSortie, sSortie, sPlaqueSortie, dstParcourue, itinerary, additionalAttributes);
    }
}

void TraceDocTrafic::RemoveVehicules(int nFromID)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->RemoveVehicules(nFromID);
    }
}
	
void TraceDocTrafic::AddSimFeux(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire)
{
    if(m_pXMLDocTrafic)
	    m_pXMLDocTrafic->AddSimFeux(sCtrlFeux, sTE, sTS, bEtatFeu, bPremierInstCycle, bPrioritaire);
    if(m_pGMLDocTrafic)
	    m_pGMLDocTrafic->AddSimFeux(sCtrlFeux, sTE, sTS, bEtatFeu, bPremierInstCycle, bPrioritaire);
    if(m_pJSONDocTrafic)
	    m_pJSONDocTrafic->AddSimFeux(sCtrlFeux, sTE, sTS, bEtatFeu, bPremierInstCycle, bPrioritaire);
}

void TraceDocTrafic::AddSimFeuxEVE(const std::string & sCtrlFeux, const std::string & sTE, const std::string & sTS, int bEtatFeu, int bPremierInstCycle, int bPrioritaire)
{
	if (m_pEveDocTrafic)
		m_pEveDocTrafic->AddSimFeuxEVE(sCtrlFeux, sTE, sTS, bEtatFeu, bPremierInstCycle, bPrioritaire);
}

void TraceDocTrafic::AddCellSimu(int nID, double dbConc, double dbDebit, double dbVitAm, double dbAccAm, double bNAm, double dbVitAv, double dbAccAv, double dbNAv, const std::string & strLibelle, const std::string & strTuyau, double dbXam, double dbYam, double dbZam, double dbXav, double dbYav, double dbZav)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddCellSimu(nID, dbConc, dbDebit, dbVitAm, dbAccAm, bNAm, dbVitAv, dbAccAv, dbNAv, strLibelle, strTuyau, dbXam, dbYam, dbZam, dbXav, dbYav, dbZav);
    }
}

void TraceDocTrafic::AddTrajectoire(int nID, Tuyau * pTuyau, const std::string & strTuyau, const std::string & strTuyauEx, const std::string & strNextTuyauEx, int nNumVoie, double dbAbs, double dbOrd, double dbZ, double dbAbsCur, double dbVit, double dbAcc, double dbDeltaN, const std::string & sTypeVehicule, double dbVitMax, double dbLongueur, const std::string & sLib, int nIDVehLeader, int nCurrentLoad, bool bTypeChgtVoie, TypeChgtVoie eTypeChgtVoie,
    bool bVoieCible, int nVoieCible, bool bPi, double dbPi, bool bPhi, double dbPhi, bool bRand, double dbRand, bool bDriven, const std::string & strDriveState, bool bDepassement, bool bRegimeFluide, const std::map<std::string, std::string> & additionalAttributes)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddTrajectoire(nID, pTuyau, strTuyau, strTuyauEx, strNextTuyauEx, nNumVoie, dbAbs, dbOrd, dbZ, dbAbsCur, dbVit, dbAcc, dbDeltaN, sTypeVehicule, dbVitMax, dbLongueur, sLib, nIDVehLeader, nCurrentLoad, bTypeChgtVoie, eTypeChgtVoie, bVoieCible, nVoieCible, bPi, dbPi, bPhi, dbPhi, bRand, dbRand, bDriven, strDriveState, bDepassement, bRegimeFluide, additionalAttributes);
    }
}

void TraceDocTrafic::AddStream(int nID, const std::string & strTuyau, const std::string & strTuyauEx )
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddStream(nID, strTuyau, strTuyauEx);
    }
}

void TraceDocTrafic::AddLink(const std::string &  strTuyau, double dbConcentration)
{
    for(size_t iDoc = 0; iDoc < m_LstDocTrafic.size(); iDoc++)
    {
        m_LstDocTrafic[iDoc]->AddLink(strTuyau, dbConcentration);
    }
}

eveShared::TrafficState * TraceDocTrafic::GetTrafficState()
{
	if (m_pEveDocTrafic == NULL)
	{
		return NULL;
	}
	else
	{
		return m_pEveDocTrafic->GetTrafficState();
	}
}

const rapidjson::Document TraceDocTrafic::s_EMPTY_JSON_DOC(rapidjson::kNullType);
const rapidjson::Document & TraceDocTrafic::GetTrafficStateJSON()
{
	if (m_pJSONDocTrafic == NULL)
	{
        return s_EMPTY_JSON_DOC;
	}
	else
	{
		return m_pJSONDocTrafic->GetTrafficState();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void TraceDocTrafic::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TraceDocTrafic::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TraceDocTrafic::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(DocTrafic);

    ar & BOOST_SERIALIZATION_NVP(m_pEveDocTrafic);
    ar & BOOST_SERIALIZATION_NVP(m_pXMLDocTrafic);
    ar & BOOST_SERIALIZATION_NVP(m_pGMLDocTrafic);
    ar & BOOST_SERIALIZATION_NVP(m_LstDocTrafic);
    ar & BOOST_SERIALIZATION_NVP(m_strFileName);
}

