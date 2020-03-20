#include "stdafx.h"
#include "Symubruit.h"
#include "SymubruitExports.h"
// Symubruit.cpp : définit le point d'entrée pour l'application DLL.
//

#include "TraceDocTrafic.h"
#include "XMLDocTrafic.h"
#include "TraceDocAcoustique.h"
#include "regulation/PythonUtils.h"
#include "Xerces/XMLUtil.h"
#include "reseau.h"
#include "SystemUtil.h"
#include "EVEDocTrafic.h"
#include "EVEDocAcoustique.h"
#include "BriqueDeConnexion.h"
#include "ConnectionPonctuel.h"
#include "SerializeUtil.h"
#include "RandManager.h"
#include "vehicule.h"
#include "tuyau.h"
#include "usage/TripNode.h"
#include "usage/Trip.h"
#include "usage/TripLeg.h"
#include "usage/AbstractFleet.h"
#include "usage/SymuViaFleet.h"
#include "usage/logistic/DeliveryFleet.h"
#include "usage/logistic/PointDeLivraison.h"
#include "Logger.h"
#include "SymuCoreManager.h"
#include "Affectation.h"
#include "Parking.h"
#include "TronconDestination.h"

#pragma warning(disable : 4003)
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#pragma warning(default : 4003)

#include <xercesc/dom/DOMXPathResult.hpp>

#ifdef USE_SYMUCORE
namespace SymuCore {
    class MultiLayersGraph;
    class VehicleType;
    class AbstractPenalty;
    class Pattern;
    class Populations;
}
#include "Plaque.h"
#include <Graph/Model/MultiLayersGraph.h>
#include <Demand/Origin.h>
#include <Demand/Destination.h>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#endif

#include <boost/math/special_functions/round.hpp>

using namespace std;

XERCES_CPP_NAMESPACE_USE

#ifndef WIN32
#define LPSTR char*
#endif

#define DEFAULT_NETWORK_ID -1

CSymubruitApp::CSymubruitApp()
{
#ifndef WIN32
    InitInstance();
#endif
}

CSymubruitApp::~CSymubruitApp()
{
#ifndef WIN32
    ExitInstance();
#endif
}

bool CSymubruitApp::InitInstance()
{    
	XMLUtil::XMLInit();

	//::MessageBox(NULL, "CSymubruitApp::InitInstance()", "Simuvia", 0);
    return TRUE;
}

int CSymubruitApp::ExitInstance()
{
//	::MessageBox(NULL, "CSymubruitApp::ExitInstance()::Début", "Simuvia", 0);

    DeleteAllNetworks();
	XMLUtil::XMLEnd();
//	::MessageBox(NULL, "CSymubruitApp::ExitInstance()::Fin", "Simuvia", 0);
    return 0;
}

CSymubruitApp theApp;

#ifdef WIN32
#ifdef _MANAGED
#pragma managed(push, off)
#endif


bool APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//InitInstance
		SystemUtil::SetInstance(hModule);
		theApp.InitInstance();
		break;
	case DLL_PROCESS_DETACH:
		//ExitInstance
		theApp.ExitInstance();
		break;
	default:
		break;
	}
    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
#endif

// AJouté pour récupération réseau pour validation des tests unitaires
SYMUBRUIT_EXPORT Reseau* SYMUBRUIT_CDECL SymGetReseau() 
{
    return theApp.GetNetwork(DEFAULT_NETWORK_ID);
}

bool _SymRunNextStep(int networkId, std::string &sXmlFluxInstant, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(networkId);

    bNEnd = true;       

    if(!pReseau)  
        return false;

    // Interdiction des simulations uniquement acoustiques
    if(!pReseau->IsSimuTrafic())
    {        
        return false;
    }

    // Initialisation du type de simulation
    //pReseau->SetIntegrationEve(false);   
	pReseau->SetSymMode(Reseau::SYM_MODE_STEP_XML);

	pReseau->SetXmlOutput(bTrace);	// Sortie dans fichier XML

    if(!pReseau->IsInitSimuTrafic())
        if(!pReseau->InitSimuTrafic())
        {
            return false;
        }

    if( pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
        if(!pReseau->IsInitSimuEmissions())
            pReseau->InitSimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());
        
    bool bEndStep;
    do
    {
        bNEnd = pReseau->SimuTrafic(bEndStep);    
    }
    while(bEndStep == false);
    
	std::string sTrafficFlow;
	std::string sAcoustiqueFlow;
    std::string sAtmoFlow;
    
    // on restitue uniquement si on a une unique plage d'extraction
    if( pReseau->m_xmlDocTrafics.size() != 1)
    {
        pReseau->PostTimeStepSimulation();
        return false;
    }
    XMLDocTrafic * XmlDocTrafic = pReseau->m_xmlDocTrafics.begin()->m_pData->GetXMLDocTrafic();
	XMLDocAcoustique * XmlDocAcoustique = (XMLDocAcoustique*)pReseau->m_XmlDocAcoustique;
    XMLDocEmissionsAtmo * XmlDocAtmo = (XMLDocEmissionsAtmo*)pReseau->m_XmlDocAtmo;

    if( pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
    {       
        // Simulation émissions
        bool bSimuTrafic = pReseau->IsSimuTrafic();

        bNEnd = pReseau->SimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());

        pReseau->SetSimuTrafic(bSimuTrafic);

        // Récupération des flux
		sTrafficFlow = pReseau->GetXMLUtil()->getOuterXml(XmlDocTrafic->GetLastInstant(false, bTrace));

        // Manipulation de chaîne pour reconstruire une chaîne XML bien formée
        SystemUtil::trim(sTrafficFlow);
		size_t nIT = sTrafficFlow.find_last_of('<');
		sTrafficFlow = sTrafficFlow.substr(0, nIT);
        sXmlFluxInstant = sTrafficFlow;

        // idem que pour le flux trafic mais pour l'acoustique
        if(pReseau->IsSimuAcoustique())
        {
            sAcoustiqueFlow = pReseau->GetXMLUtil()->getOuterXml(XmlDocAcoustique->GetLastInstant());
            XmlDocAcoustique->ReleaseLastInstant();
            SystemUtil::trim(sAcoustiqueFlow);
            size_t nIA = sAcoustiqueFlow.find_first_of('>');
            sAcoustiqueFlow = sAcoustiqueFlow.substr( nIA+1 );
            sXmlFluxInstant += sAcoustiqueFlow;
        }
    }
    else
    {		
        sXmlFluxInstant = pReseau->GetXMLUtil()->getOuterXml(XmlDocTrafic->GetLastInstant(false, bTrace));
    }

    pReseau->PostTimeStepSimulation();
	
 /*  if(bNEnd == true)
    {
        pReseau->FinSimuTrafic();
        if(pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
            pReseau->FinSimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());        

        // Déchargement du réseau
        delete theApp.m_pReseau;
        theApp.m_pReseau = NULL;
    }
    */
    return true;
}

bool _SymRunNextStep(eveShared::TrafficState * &pTrafficEVE, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;       

    if(!pReseau)  
        return false;

    // Interdiction des simulations uniquement acoustiques
    if(!pReseau->IsSimuTrafic())
    {        
        return false;
    }

    // Initialisation du type de simulation
	pReseau->SetSymMode(Reseau::SYM_MODE_STEP_EVE);

	pReseau->SetXmlOutput(bTrace);	// Sortie dans fichier XML

    if(!pReseau->IsInitSimuTrafic())
        if(!pReseau->InitSimuTrafic())
        {
            return false;
        }

    if( pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
        if(!pReseau->IsInitSimuEmissions())
            pReseau->InitSimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());
      
    bool bEndStep;
    do
    {
        bNEnd = pReseau->SimuTrafic(bEndStep);    
    }
    while(bEndStep == false) ;
    	
    if( pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
    {       
        // Simulation acoustique
        bool bSimuTrafic = pReseau->IsSimuTrafic();

        bNEnd = pReseau->SimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());

        pReseau->SetSimuTrafic(bSimuTrafic);

        // Récupération des flux
		if (pTrafficEVE == NULL)
		{
			pTrafficEVE = new eveShared::TrafficState();
		}
		else
		{
			eveShared::TrafficState::Empty(pTrafficEVE);
		}

        eveShared::TrafficState * pTrafState1 = pReseau->m_xmlDocTrafics.front().m_pData->GetTrafficState();
		eveShared::TrafficState * pTrafState2;
        TraceDocAcoustique * tDocAcoustique = dynamic_cast<TraceDocAcoustique*>(pReseau->m_XmlDocAcoustique);
        if (tDocAcoustique)
		{
			pTrafState2 = tDocAcoustique->GetTrafficState();
		}
		else
		{
			EVEDocAcoustique * EveDocAcoustique = (EVEDocAcoustique*)(pReseau->m_XmlDocAcoustique);
			pTrafState2 = EveDocAcoustique->GetTrafficState();

		}
		eveShared::TrafficState::Copy(pTrafState1, pTrafficEVE);
		eveShared::TrafficState::Copy(pTrafState2, pTrafficEVE);
    }
    else
    {
		if (pTrafficEVE == NULL)
		{
			pTrafficEVE = new eveShared::TrafficState();
		}
		else
		{
			eveShared::TrafficState::Empty(pTrafficEVE);
		}

        eveShared::TrafficState * pTrafState = pReseau->m_xmlDocTrafics.front().m_pData->GetTrafficState();
		eveShared::TrafficState::Copy(pTrafState, pTrafficEVE);
    }

    pReseau->PostTimeStepSimulation();
	
 /*   if(bNEnd == true)
    {
        pReseau->FinSimuTrafic();
        if(pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
            pReseau->FinSimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());        

        // Déchargement du réseau
        delete theApp.m_pReseau;
        theApp.m_pReseau = NULL;
    }*/

    return true;
}

bool _SymRunNextStepJSON(std::string &json, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;       

    if(!pReseau)  
        return false;

    // Interdiction des simulations uniquement acoustiques
    if(!pReseau->IsSimuTrafic())
    {        
        return false;
    }

    // Initialisation du type de simulation
	pReseau->SetSymMode(Reseau::SYM_MODE_STEP_JSON);

	pReseau->SetXmlOutput(false);	// Sortie dans fichier XML
	//pReseau->SetXmlOutput(true);	// Sortie dans fichier XML

    if(!pReseau->IsInitSimuTrafic())
        if(!pReseau->InitSimuTrafic())
        {
            return false;
        }

    bool bEndStep;
    do
    {
        bNEnd = pReseau->SimuTrafic(bEndStep);    
    }
    while(bEndStep == false);
    	
    const rapidjson::Document & jsonObject = pReseau->m_xmlDocTrafics.front().m_pData->GetTrafficStateJSON();

    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    jsonObject.Accept(writer);
    json = buffer.GetString();

    pReseau->PostTimeStepSimulation();
	
 /*   if(bNEnd == true)
    {
        pReseau->FinSimuTrafic();
        if(pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
            pReseau->FinSimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());        

        // Déchargement du réseau
        delete theApp.m_pReseau;
        theApp.m_pReseau = NULL;
    }*/

    return true;
}


bool _SymRunToStep(int nStep, eveShared::TrafficState * &pTrafficEVE, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;       

    if(!pReseau)  
        return false;

    // Interdiction des simulations uniquement acoustiques
    if(!pReseau->IsSimuTrafic())
    {        
        return false;
    }

    // Initialisation du type de simulation
	pReseau->SetSymMode(Reseau::SYM_MODE_STEP_EVE);

	pReseau->SetXmlOutput(bTrace);	// Sortie dans fichier XML

    if(!pReseau->IsInitSimuTrafic())
        if(!pReseau->InitSimuTrafic())
        {
            return false;
        }

    if( pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
        if(!pReseau->IsInitSimuEmissions())
            pReseau->InitSimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());
        
	bNEnd = false;

    bool bEndStep;
	pReseau->SetSortieTraj(false);
	while( nStep > 1 && !bNEnd )
    {
		bNEnd = pReseau->SimuTrafic(bEndStep); 
        pReseau->PostTimeStepSimulation();
        if(bEndStep)
        {   
            nStep--;
        }
    }

	pReseau->SetSortieTraj(true);
    bool bReleaseNeaded = false;
	if( !bNEnd )
    {
        bEndStep = false;
        while(bEndStep == false && !bNEnd)
        {
		    bNEnd = pReseau->SimuTrafic(bEndStep); 
        }
        bReleaseNeaded = true;
    }
    	
    if( pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
    {       
        // Simulation acoustique
        bool bSimuTrafic = pReseau->IsSimuTrafic();

        bNEnd = pReseau->SimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());

        pReseau->SetSimuTrafic(bSimuTrafic);

        // Récupération des flux
		if (pTrafficEVE == NULL)
		{
			pTrafficEVE = new eveShared::TrafficState();
		}
		else
		{
			eveShared::TrafficState::Empty(pTrafficEVE);
		}

        eveShared::TrafficState * pTrafState1 = pReseau->m_xmlDocTrafics.front().m_pData->GetTrafficState();
        eveShared::TrafficState::Copy(pTrafState1, pTrafficEVE);

        if(pReseau->IsSimuAcoustique())
        {
	    	eveShared::TrafficState * pTrafState2;
            TraceDocAcoustique * tDocAcoustique = dynamic_cast<TraceDocAcoustique*>(pReseau->m_XmlDocAcoustique);
            if(tDocAcoustique)
            {
                pTrafState2 = tDocAcoustique->GetTrafficState();
            }
            else
            {
                EVEDocAcoustique * EveDocAcoustique = (EVEDocAcoustique*)(pReseau->m_XmlDocAcoustique);
			    pTrafState2 = EveDocAcoustique->GetTrafficState();
            }
            eveShared::TrafficState::Copy(pTrafState2, pTrafficEVE);
        }
    }
    else
    {
		if (pTrafficEVE == NULL)
		{
			pTrafficEVE = new eveShared::TrafficState();
		}
		else
		{
			eveShared::TrafficState::Empty(pTrafficEVE);
		}

        eveShared::TrafficState * pTrafState = pReseau->m_xmlDocTrafics.front().m_pData->GetTrafficState();
		eveShared::TrafficState::Copy(pTrafState, pTrafficEVE);
    }

    if(bReleaseNeaded)
    {
        pReseau->PostTimeStepSimulation();
    }
	
    /*if(bNEnd == true)
    {
        pReseau->FinSimuTrafic();
        if(pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
            pReseau->FinSimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());        

        // Déchargement du réseau
        delete theApp.m_pReseau;
        theApp.m_pReseau = NULL;
    }*/

    return true;
}


bool _SymRunToStep(int nStep, std::string &sXmlFluxInstant, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;       

    if(!pReseau)  
        return false;

    // Interdiction des simulations uniquement acoustiques
    if(!pReseau->IsSimuTrafic())
    {        
        return false;
    }

    // Initialisation du type de simulation
	pReseau->SetSymMode(Reseau::SYM_MODE_STEP_XML);

	pReseau->SetXmlOutput(bTrace);	// Sortie dans fichier XML

    if(!pReseau->IsInitSimuTrafic())
        if(!pReseau->InitSimuTrafic())
        {
            return false;
        }

    if( pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
        if(!pReseau->IsInitSimuEmissions())
            pReseau->InitSimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());
        
	bNEnd = false;

    bool bEndStep;
	pReseau->SetSortieTraj(false);
	while( nStep > 1 && !bNEnd )
    {
		bNEnd = pReseau->SimuTrafic(bEndStep); 
        pReseau->PostTimeStepSimulation();
        if(bEndStep)
        {   
            nStep--;
        }
    }

	pReseau->SetSortieTraj(true);
    bool bReleaseNeaded = false;
	if( !bNEnd )
    {
		bEndStep = false;
        while(bEndStep == false && !bNEnd)
        {
		    bNEnd = pReseau->SimuTrafic(bEndStep); 
        }
        bReleaseNeaded = true;
    }

    std::string sTrafficFlow;
	std::string sAcoustiqueFlow;
    std::string sAtmoFlow;
	XMLDocTrafic * XmlDocTrafic = pReseau->m_xmlDocTrafics.front().m_pData->GetXMLDocTrafic();
	XMLDocAcoustique * XmlDocAcoustique = (XMLDocAcoustique*)pReseau->m_XmlDocAcoustique;
    XMLDocEmissionsAtmo * XmlDocAtmo = (XMLDocEmissionsAtmo*)pReseau->m_XmlDocAtmo;
    	

    if( pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
    {       
        // Simulation émissions
        bool bSimuTrafic = pReseau->IsSimuTrafic();

        bNEnd = pReseau->SimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());

        pReseau->SetSimuTrafic(bSimuTrafic);

        // Récupération des flux
        sTrafficFlow = pReseau->GetXMLUtil()->getOuterXml(XmlDocTrafic->GetLastInstant(false, bTrace));

        // Manipulation de chaîne pour reconstruire une chaîne XML bien formée
        SystemUtil::trim(sTrafficFlow);
		size_t nIT = sTrafficFlow.find_last_of('<');
		sTrafficFlow = sTrafficFlow.substr(0, nIT);
        sXmlFluxInstant = sTrafficFlow;

        // idem que pour le flux trafic mais pour l'acoustique
        if(pReseau->IsSimuAcoustique())
        {
            sAcoustiqueFlow = pReseau->GetXMLUtil()->getOuterXml(XmlDocAcoustique->GetLastInstant());
            XmlDocAcoustique->ReleaseLastInstant();
            SystemUtil::trim(sAcoustiqueFlow);
            size_t nIA = sAcoustiqueFlow.find_first_of('>');
            sAcoustiqueFlow = sAcoustiqueFlow.substr( nIA+1 );
            sXmlFluxInstant += sAcoustiqueFlow;
        }
    }
    else
    {		
        sXmlFluxInstant = pReseau->GetXMLUtil()->getOuterXml(XmlDocTrafic->GetLastInstant(false, bTrace));
    }

    if(bReleaseNeaded)
    {
        pReseau->PostTimeStepSimulation();
    }
    /*
    if(bNEnd == true)
    {
        pReseau->FinSimuTrafic();
        if(pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
            pReseau->FinSimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());        

        // Déchargement du réseau
        delete theApp.m_pReseau;
        theApp.m_pReseau = NULL;
    }*/

    return true;
}

int SymLoadNetwork(std::string sXmlDataFile, bool bSymuMasterMode, bool bWithPublicTransportGraph, bool bUseDefaultNetworkId, std::string sScenarioID = "", std::string sOutdir = "", double dbInitialWalkRadius = -1, double dbIntermediateWalkRadius = -1, double dbWalkSpeed = -1)
{	
    int networkId;
    Reseau * pReseau = theApp.CreateNetwork(bUseDefaultNetworkId, networkId);

    // Chargement du réseau
    if (bSymuMasterMode)
    {
        pReseau->SetSymuMasterMode();
        if (networkId > 1)
        {
            std::ostringstream oss;
            oss << "_instance" << networkId;
            pReseau->SetSuffixOutputFiles(oss.str());
        }
    }
    if (bWithPublicTransportGraph)
    {
        pReseau->SetWithPublicTransportGraph();
        pReseau->SetMaxIntermediateWalkRadius(dbIntermediateWalkRadius);
        pReseau->SetMaxInitialWalkRadius(dbInitialWalkRadius);
        pReseau->SetWalkSpeed(dbWalkSpeed);
    }

    bool bLoadOk = pReseau->LoadReseauXML(sXmlDataFile, sScenarioID, sOutdir);
    pReseau->m_pLoadingLogger = NULL;

    if (!bLoadOk)
    {
        theApp.DeleteNetwork(networkId);
        return 0;
    }

    return networkId;
}


/// <summary>
/// Méthode exportée de chargement d'un scenario
/// </summary>
///<returns>0 si OK</returns>
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymLoadNetwork(std::string sXmlDataFile, std::string sScenarioID = "", std::string sOutdir = "")
{
    return SymLoadNetwork(sXmlDataFile, false, false, true, sScenarioID, sOutdir);
}

/// <summary>
/// Méthode exportée de chargement d'un nouveau scenario (pour parallélisation)
/// </summary>
///<returns>0 si OK</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymLoadNetworkPar(std::string sTmpXmlDataFile, std::string sScenarioID = "", std::string sOutdir = "")
{
    return SymLoadNetwork(sTmpXmlDataFile, false, false, false, sScenarioID, sOutdir);
}

/// <summary>
/// Méthode exportée de chargement d'un scenario, avec prise en compte des transports publics pour le graphe de calcul des itinéraires
/// </summary>
///<returns>0 si OK</returns>
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymLoadNetworkWithPublicTransport(std::string sXmlDataFile, std::string sScenarioID = "", std::string sOutdir = "", double dbInitialWalkRadius = -1, double dbIntermediateWalkRadius = -1, double dbWalkSpeed = -1)
{
    return SymLoadNetwork(sXmlDataFile, false, true, true, sScenarioID, sOutdir, dbInitialWalkRadius, dbIntermediateWalkRadius, dbWalkSpeed);
}

/// <summary>
/// Méthode wrapper exportée pour le chargement d'un scenario
/// </summary>
///<returns>0 si OK</returns>
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymLoadNetwork(const char* sTmpXmlDataFile, const char* sOutDir)
{
    return SymLoadNetwork(std::string(sTmpXmlDataFile), "", std::string(sOutDir));
}

/// <summary>
/// Méthode wrapper exportée pour le chargement d'un scenario
/// </summary>
///<returns>0 si OK</returns>
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymLoadNetwork(const char* sTmpXmlDataFile, const char* sScenarioID, const char* sOutDir)
{
    return SymLoadNetwork(std::string(sTmpXmlDataFile), std::string(sScenarioID), std::string(sOutDir));
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymLoadNetwork(std::string sTmpXmlDataFile, eveShared::SimulationInfo * &pSInfo, eveShared::SymuviaNetwork * &pSNetwork, std::string sScenarioID = "", std::string sOutDir = "")
{
	bool bOk;

	bOk = SymLoadNetwork(sTmpXmlDataFile, sScenarioID, sOutDir);

	if (bOk)
	{
        Reseau * pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

		if (pSInfo == NULL)
		{
			pSInfo = new eveShared::SimulationInfo();
		}

		// Classe SimulationInfo
		pSInfo->begin = pReseau->GetSimuStartTime();
		pSInfo->changeLane = pReseau->IsChgtVoie();
		pSInfo->date = pReseau->GetDateSimulation().ToDate();
		pSInfo->duration = (int)pReseau->GetDureeSimu();
		pSInfo->end = pReseau->GetSimuEndTime();
		pSInfo->loipoursuite = pReseau->GetLoipoursuite();
		pSInfo->step = pReseau->GetTimeStep();
		pSInfo->title = pReseau->GetTitre();
        if(pReseau->IsSimuAcoustique() && pReseau->IsSimuTrafic()) {
		    pSInfo->typesimulation = "all";
        } else if(!pReseau->IsSimuAcoustique() && pReseau->IsSimuTrafic()) {
            pSInfo->typesimulation = "trafic";
        } else if(pReseau->IsSimuAcoustique() && !pReseau->IsSimuTrafic()) {
            pSInfo->typesimulation = "acoustique";
        }
		extern const char* VERSION_FICHIER;
		pSInfo->version = atof(VERSION_FICHIER);
        for(size_t iTimespan = 0; iTimespan < pReseau->m_LstPlagesTemporelles.size(); iTimespan++)
        {
            PlageTemporelle * pPT = pReseau->m_LstPlagesTemporelles[iTimespan];
            eveShared::Timespan ts;
            ts.begin = pSInfo->begin + STimeSpan(0, 0, 0, (int)pPT->m_Debut);
            ts.end = pSInfo->begin + STimeSpan(0, 0, 0, (int)pPT->m_Fin);
            ts.id = pPT->m_ID;
            pSInfo->timespans.push_back(ts);
        }

		// Classe SymuviaNetwork
		if (pSNetwork == NULL)
		{
			pSNetwork = new eveShared::SymuviaNetwork();
		}
        else
        {
            eveShared::SymuviaNetwork::Empty(pSNetwork);
        }
		std::deque<Tuyau*>::iterator itBeg, itEnd, itTuyau;
		itBeg = pReseau->m_LstTuyaux.begin();
		itEnd = pReseau->m_LstTuyaux.end();
		for (itTuyau = itBeg; itTuyau != itEnd; itTuyau++)
		{
			if( !(*itTuyau)->GetBriqueParente() )	// Exclusion des tronçons construits par SymuVia
			{
				std::string sID;
				Point * pPt;
				eveShared::Troncon * pTroncon = new eveShared::Troncon();
				pPt = (*itTuyau)->GetExtAmont();
				pTroncon->extremite_amont[0] = pPt->dbX;
				pTroncon->extremite_amont[1] = pPt->dbY;
				pTroncon->extremite_amont[2] = pPt->dbZ;
				pPt = (*itTuyau)->GetExtAval();
				pTroncon->extremite_aval[0] = pPt->dbX;
				pTroncon->extremite_aval[1] = pPt->dbY;
				pTroncon->extremite_aval[2] = pPt->dbZ;
				pTroncon->id = (*itTuyau)->GetLabel();
				sID = pTroncon->id;
				if ((*itTuyau)->GetBriqueAmont() != NULL)
				{
					pTroncon->id_eltamont = (*itTuyau)->GetBriqueAmont()->GetID();
				}
				else
					if ((*itTuyau)->getConnectionAmont() != NULL)
					{
						pTroncon->id_eltamont = (*itTuyau)->getConnectionAmont()->GetID();
					}
				if ((*itTuyau)->GetBriqueAval() != NULL)
				{
					pTroncon->id_eltaval = (*itTuyau)->GetBriqueAval()->GetID();
				}
				else
					if ((*itTuyau)->getConnectionAval() != NULL)
					{
						pTroncon->id_eltaval = (*itTuyau)->getConnectionAval()->GetID();
					}
                pTroncon->largeurs_voies = (*itTuyau)->getLargeursVoies();
				pTroncon->nb_cell_acoustique = (*itTuyau)->GetNbCell(); 
				pTroncon->nb_voie = (*itTuyau)->getNb_voies(); 

                for(size_t i = 0; i < (*itTuyau)->GetZLevelCrossings().size(); i++)
                {
                    eveShared::LevelCrossing * pLvlCrossing = new eveShared::LevelCrossing;
                    pLvlCrossing->start = (*itTuyau)->GetZLevelCrossings()[i].dbPosDebut;
                    pLvlCrossing->end = (*itTuyau)->GetZLevelCrossings()[i].dbPosFin;
                    pLvlCrossing->zlevel = (*itTuyau)->GetZLevelCrossings()[i].nZLevel;
                    pTroncon->LevelCrossings.push_back(pLvlCrossing);
                }

				if( (*itTuyau)->GetResolution() == 1)
					pTroncon->resolution = "M";
				else
					pTroncon->resolution = "H";

				pTroncon->revetement = (*itTuyau)->GetRevetement();
				pTroncon->vit_reg = (*itTuyau)->GetVitReg();				
				std::deque<Point*>::iterator itBeg, itEnd, itPt;
				itBeg = (*itTuyau)->GetLstPtsInternes().begin();
				itEnd = (*itTuyau)->GetLstPtsInternes().end();
				for (itPt = itBeg; itPt!=itEnd; itPt++)
				{
					double * pCoord = new double[3];
					pCoord[0] = (*itPt)->dbX;
					pCoord[1] = (*itPt)->dbY;
					pCoord[2] = (*itPt)->dbZ;
					pTroncon->point_internes.push_back(pCoord);
				}
				pSNetwork->troncons[sID]=pTroncon;
			}
		}
	}
	return bOk;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRun()
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    if(!pReseau)
    {
        return false;
    }

    if(!pReseau->Execution())
        return false;
    else
        return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunTraffic()
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    if(!pReseau)
        return false;

    pReseau->SetSimuTrafic(true);
    pReseau->SetSimuAcoustique(false);

    return pReseau->Execution();
}


SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymGenAcousticCells()
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    if(!pReseau)
    {        
        return false;
    }    

    pReseau->InitSimuEmissions(true, false, false, true);
    pReseau->FinSimuEmissions(true, false, false);     

    return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunAcousticEmissions()
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    if(!pReseau)
    {     
        return false;
    }    

    pReseau->SetSimuTrafic(false);
    pReseau->SetSimuAcoustique(true);

    if(!pReseau->Execution())
        return false;

    return true;
}
/*
SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymRunNextStepNode(bool bTrace, bool &bNEnd){
	if(bNEnd)
		return NULL;

	std:string sXmlFluxInstant;
	std::string sFlow;

	if( !_SymRunNextStep(sFlow, bTrace, bNEnd) )
		return NULL;

	SystemUtil::wstring2string(sFlow, sXmlFluxInstant);
	char* retValue = new char[sXmlFluxInstant.length()+1];
	for(int i=0; i < sXmlFluxInstant.length(); i++){
		retValue[i] = sXmlFluxInstant[i];
	}
	retValue[sXmlFluxInstant.length()] = '\0';
	return retValue;
}*/
/*
SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetNetworkNode(){
	std:string sXmlFlowNetwork;
	Reseau* pReseau = theApp.m_pReseau;    

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return NULL;
    }
	
	std::string sXml = pReseau->GetNetwork();   
	std::string sXmlStr;
	
	SystemUtil::wstring2string(sXml, sXmlFlowNetwork);

	char* retValue = new char[sXmlFlowNetwork.length()+1];
	for(int i=0; i < sXmlFlowNetwork.length(); i++){
		retValue[i] = sXmlFlowNetwork[i];
	}
	retValue[sXmlFlowNetwork.length()] = '\0';
	return retValue;
}
*/
/*
SYMUBRUIT_EXPORT void SYMUBRUIT_CDECL SymRunDeleteStr(char* str){
	if(str != NULL){
		delete(str);
	}
}

*/

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStep(LPSTR lpszXmlFluxInstant, bool bTrace, bool &bNEnd)
{
	std::string sFlow;

	if( !_SymRunNextStep(DEFAULT_NETWORK_ID, sFlow, bTrace, bNEnd) )
		return false;

    /*if( bNEnd )
    {
        theApp.m_pReseau->FinSimuTrafic();
        if(theApp.m_pReseau->IsSimuAcoustique() || theApp.m_pReseau->IsSimuAir() || theApp.m_pReseau->IsSimuSirane())
            theApp.m_pReseau->FinSimuEmissions(theApp.m_pReseau->IsSimuAcoustique(), theApp.m_pReseau->IsSimuAir(), theApp.m_pReseau->IsSimuSirane());        

        // Déchargement du réseau
        delete theApp.m_pReseau;
        theApp.m_pReseau = NULL;
    
    }*/
	if(bNEnd)
		theApp.DeleteAllNetworks();

	strcpy(lpszXmlFluxInstant, sFlow.c_str());
		             		
    return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStep(eveShared::TrafficState * &pTrafficEVE, bool bTrace, bool &bNEnd)
{
	if( !_SymRunNextStep(pTrafficEVE, bTrace, bNEnd) )
		return false;
		             		
    return true;
}
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStep(string &sXmlFluxInstant, bool bTrace, bool &bNEnd)
{ 
	std::string sFlow;

    if (!_SymRunNextStep(DEFAULT_NETWORK_ID, sFlow, bTrace, bNEnd))
		return false;

    // Commenté car rend l'utilisation avec SymuMaster impossible (ou en tout cas ca empêche de faire des boucles sur la dernière période d'affectation)
    /*if( bNEnd )
    {
        theApp.m_pReseau->FinSimuTrafic();
        if(theApp.m_pReseau->IsSimuAcoustique() || theApp.m_pReseau->IsSimuAir() || theApp.m_pReseau->IsSimuSirane())
            theApp.m_pReseau->FinSimuEmissions(theApp.m_pReseau->IsSimuAcoustique(), theApp.m_pReseau->IsSimuAir(), theApp.m_pReseau->IsSimuSirane());        

        // Déchargement du réseau
        delete theApp.m_pReseau;
        theApp.m_pReseau = NULL;
    
    }*/

	sXmlFluxInstant = sFlow;
		             		
    return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStep(int networkId, std::string &sXmlFluxInstant, bool bTrace, bool &bNEnd)
{
    std::string sFlow;

    if (!_SymRunNextStep(networkId, sFlow, bTrace, bNEnd))
        return false;

    sXmlFluxInstant = sFlow;

    return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunToStep(int nInstant, string &sXmlFluxInstant, bool bTrace, bool &bNEnd)
{ 
	std::string sFlow;

	if( !_SymRunToStep(nInstant, sFlow, bTrace, bNEnd) )
		return false;

	sXmlFluxInstant = sFlow;
		             		
    return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunToStep(int nStep, eveShared::TrafficState * &pTrafficEVE, bool bTrace, bool &bNEnd)
{ 
	return _SymRunToStep(nStep, pTrafficEVE, bTrace, bNEnd);
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTraffic(LPSTR lpszXmlFluxInstant, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;    

    if(!pReseau)
        return false;

    pReseau->SetSimuTrafic(true);
    pReseau->SetSimuAcoustique(false);
    pReseau->SetSimuAir(false);

    return SymRunNextStep(lpszXmlFluxInstant, bTrace, bNEnd);    
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTraffic(eveShared::TrafficState * &pTrafficEVE, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;    

    if(!pReseau)
        return false;

    pReseau->SetSimuTrafic(true);
    pReseau->SetSimuAcoustique(false);
    pReseau->SetSimuAir(false);

    return SymRunNextStep(pTrafficEVE, bTrace, bNEnd);    
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTraffic(string& sXmlFluxInstant, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;    

    if(!pReseau)
        return false;

    pReseau->SetSimuTrafic(true);
    pReseau->SetSimuAcoustique(false);
    pReseau->SetSimuAir(false);

    return SymRunNextStep(sXmlFluxInstant, bTrace, bNEnd);    
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTrafficAcoustic(string& sXmlFluxInstant, bool bCel, bool bSrc, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;    

    if(!pReseau)
        return false;

    pReseau->SetSimuTrafic(true);
    pReseau->SetSimuAcoustique(true);
    pReseau->SetSimuAir(false);

	pReseau->m_bAcousCell = bCel;
	pReseau->m_bAcousSrcs = bSrc;

    return SymRunNextStep(sXmlFluxInstant, bTrace, bNEnd);    
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTrafficAcoustic(eveShared::TrafficState * &pTrafficEVE, bool bCel, bool bSrc, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;    

    if(!pReseau)
        return false;

    pReseau->SetSimuTrafic(true);
    pReseau->SetSimuAcoustique(true);
    pReseau->SetSimuAir(false);

	pReseau->m_bAcousCell = bCel;
	pReseau->m_bAcousSrcs = bSrc;

    return SymRunNextStep(pTrafficEVE, bTrace, bNEnd);    
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTrafficAcoustic(LPSTR lpszXmlFluxInstant, bool bCel, bool bSrc, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;    

    if(!pReseau)
        return false;

    pReseau->SetSimuTrafic(true);
    pReseau->SetSimuAcoustique(true);
    pReseau->SetSimuAir(false);

	pReseau->m_bAcousCell = bCel;
	pReseau->m_bAcousSrcs = bSrc;

    return SymRunNextStep(lpszXmlFluxInstant, bTrace, bNEnd);    
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTrafficAtmospheric(string& sXmlFluxInstant, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;    

    if(!pReseau)
        return false;

    pReseau->SetSimuTrafic(true);
    pReseau->SetSimuAcoustique(false);
    pReseau->SetSimuAir(true);

    return SymRunNextStep(sXmlFluxInstant, bTrace, bNEnd);    
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTrafficAtmospheric(eveShared::TrafficState * &pTrafficEVE, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;    

    if(!pReseau)
        return false;

    pReseau->SetSimuTrafic(true);
    pReseau->SetSimuAcoustique(false);
    pReseau->SetSimuAir(true);

    return SymRunNextStep(pTrafficEVE, bTrace, bNEnd);    
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunNextStepTrafficAtmospheric(LPSTR lpszXmlFluxInstant, bool bTrace, bool &bNEnd)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    bNEnd = true;    

    if(!pReseau)
        return false;

    pReseau->SetSimuTrafic(true);
    pReseau->SetSimuAcoustique(false);
    pReseau->SetSimuAir(true);

    return SymRunNextStep(lpszXmlFluxInstant, bTrace, bNEnd);    
}


SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymUpdateNetwork(std::string sXmlDataFile)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return false;
    }

    // Traitement du nom du fichier de modification       	
	bool bResult = pReseau->UpdateReseau(sXmlDataFile);

	return bResult;    
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymGenEveNetwork(eveShared::EveNetwork * &pNetwork)
{    
    // Traitement du nom du fichier d'entrée   
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return false;
    }
	
    pReseau->GenReseauCirculationFile(pNetwork);   
	if (pNetwork == NULL)
		return false;

    return true;
}


SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymGetNetwork(std::string &sXmlFlowNetwork)
{    
    // Traitement du nom du fichier d'entrée   
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return false;
    }
	
	sXmlFlowNetwork = pReseau->GetNetwork();   

    return true;
}


SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymDeceleration(double dbRate)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return false;
    }

    // Taux de décélération négative <= 0 --> erreur
    if(dbRate <= 0)
    {
        return false;
    }   

	std::string sTrafFile = pReseau->GetPrefixOutputFiles() + "_traf.xml";
    DecelerationProcedure( dbRate, pReseau, sTrafFile);

    return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunAirEmissions()
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    if(!pReseau)
    {     
        return false;
    }    

    pReseau->SetSimuTrafic(false);
    pReseau->SetSimuAcoustique(false);
    pReseau->SetSimuAir(true);

    if(!pReseau->Execution())
        return false;

    return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymGenTrajectories()
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return false;
    }

	std::string sTrafFile = pReseau->GetPrefixOutputFiles();
    sTrafFile += "_traf.xml";
    TrajectoireProcedure( pReseau, sTrafFile);

    return true;
}

// Pilotage d'un contrôleur de feux
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymSendSignalPlan(std::string strCDF, std::string strSP)
{
    int nRet = -1;

    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return -1;
    }

    nRet = pReseau->SendSignalPlan( strCDF, strSP);

    return nRet;
}

/// <summary>
/// Méthode exportée de pilotage de la vitesse limite d'un tronçon
/// </summary>
///<param name="sSection">Identifiant du tronçon </param>
///<param name="sVehType">Identifiant du type de véhicule </param>
///<param name="numVoie">voie concernée (-1 : toutes) </param>
///<param name="dbDebutPortion">début de la portion à partir du début du tronçon (en m) </param>
///<param name="dbFinPortion">fin de la portion à partir du début du tronçon (en m) </param>
///<param name="dbSpeedLimit">Vitesse limite à imposer </param>
///<returns>0 si OK</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymSendSpeedLimit
(
	std::string sSection,				// identifiant du tronçon 
	std::string sVehType,				// identifiant du type de véhicule (si NULL, le pilotage concerne tous les véhicules)	
    int numVoie,                        // voie concernée (-1 : toutes)
    double dbDebutPortion,              // début de la portion à partir du début du tronçon (en m)
    double dbFinPortion,                // fin de la portion à partir du début du tronçon (en m)
	double dbSpeedLimit			        // vitesse réglementaire à imposer
)
{
	int nRet = -1;	// code retour

    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return -1;
    }

	return pReseau->SendSpeedLimit( sSection, sVehType, numVoie, dbDebutPortion, dbFinPortion, dbSpeedLimit);
}

/// <summary>
/// Méthode exportée d'interpolation des caractéristiques linéraires des véhicules
/// </summary>
///<param name="lpszXmlTraficFile">Chemin du fichier de sortie trafic</param>
///<param name="nSubdivision">Nombre de subdivision du pas de temps trafic</param>
///<returns>0 si OK</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymSendSpeedLimit
(
	std::string sXmlTraficFile,
	int nSubdivision
)
{
	if( nSubdivision <= 0 )
		return -1;				// Nombre de subdivision erroné

	return 0;
}

/// <summary>
/// Méthode exportée de reset de simulation
/// Cette méthode permet de se replacer à l'instant initial de la simulation lors d'un simu pas à pas
/// </summary>
///<returns>0 si OK</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymReset()
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return -1;
    }
	
	pReseau->m_bInitSimuTrafic = false;
	pReseau->m_bInitSimuEmissions = false;	

	return 0;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunDeleteTraffic(eveShared::TrafficState * &pTrafficEVE)
{
	if( pTrafficEVE )
		delete(pTrafficEVE);

	pTrafficEVE = nullptr;

	return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunDeleteInfo(eveShared::SimulationInfo * &pSimulationInfoEVE)
{
	if( pSimulationInfoEVE )
		delete(pSimulationInfoEVE);

	pSimulationInfoEVE = nullptr;

	return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymRunDeleteNetwork(eveShared::SymuviaNetwork * &pNetworkEVE)
{
	if( pNetworkEVE )
		delete(pNetworkEVE);

	pNetworkEVE = nullptr;

	return true;
}

CSymubruitApp::CSymubruitNetworkInstance::CSymubruitNetworkInstance(Reseau * pNetwork) :
m_pNetwork(pNetwork)
{
}

CSymubruitApp::CSymubruitNetworkInstance::~CSymubruitNetworkInstance()
{
    delete m_pNetwork;
}

Reseau * CSymubruitApp::CSymubruitNetworkInstance::GetNetwork() const
{
    return m_pNetwork;
}

Reseau * CSymubruitApp::GetNetwork(int networkId)
{
    // rmq. : ce lock ne protège pas d'une detruction du réseau entre sa récupération par cette méthode et son utilisation
    // puisque le lock est libéré au retour de cette méthode : l'appelant doit faire en sorte de ne pas détruire le réseau
    // tant qu'il l'utilise (ce qui fait sens).
    // Ce lock sert à protéger l'accès concurrent à m_mapNetworks : si un réseau est supprimé car plus utilisé par le client, 
    // sa suppression impliquant la modification de m_mapNetworks peut provoquer ds crash si elle a lieu en même temps qu'un GetNetork
    // pour un autre réseau dans un autre thread.
    boost::unique_lock<boost::shared_mutex> lock(m_NetworksAccessMutex);

    std::map<int, CSymubruitNetworkInstance*>::iterator iter = m_mapNetworks.find(networkId);

    if (iter != m_mapNetworks.end())
    {
        return iter->second->GetNetwork();
    }
    else
    {
        return NULL;
    }
}

Reseau * CSymubruitApp::CreateNetwork(bool bUseDefaultNetworkId, int & networkId)
{
    if (bUseDefaultNetworkId)
    {
        networkId = DEFAULT_NETWORK_ID;
        DeleteNetwork(networkId);
    }

    boost::unique_lock<boost::shared_mutex> lock(m_NetworksAccessMutex);

    Reseau * pNetwork = new Reseau("log");

    if (!bUseDefaultNetworkId)
    {
        networkId = m_mapNetworks.empty() ? 1 : m_mapNetworks.rbegin()->first + 1;
    }
    assert(m_mapNetworks.find(networkId) == m_mapNetworks.end());
    m_mapNetworks[networkId] = new CSymubruitNetworkInstance(pNetwork);

    return pNetwork;
}

CSymubruitApp::CSymubruitNetworkInstance * CSymubruitApp::GetNetworkInstance(int networkId)
{
    std::map<int, CSymubruitNetworkInstance*>::iterator iter = m_mapNetworks.find(networkId);

    if (iter != m_mapNetworks.end())
    {
        return iter->second;
    }
    else
    {
        return NULL;
    }
}

void CSymubruitApp::DeleteAllNetworks()
{
    boost::unique_lock<boost::shared_mutex> lock(m_NetworksAccessMutex);

    for (std::map<int, CSymubruitNetworkInstance*>::iterator iter = m_mapNetworks.begin(); iter != m_mapNetworks.end();)
    {
        CSymubruitNetworkInstance * pToDelete = iter->second;

        {
            // don't remove the network while a parallel shortest path computation is in progress on it
            boost::unique_lock<boost::shared_mutex> lock(pToDelete->m_ShortestPathsComputeMutex);
            iter = m_mapNetworks.erase(iter);
        }

        delete pToDelete;
    }
}

void CSymubruitApp::DeleteNetwork(int networkId)
{
    boost::unique_lock<boost::shared_mutex> lock(m_NetworksAccessMutex);

    std::map<int, CSymubruitNetworkInstance*>::iterator iter = m_mapNetworks.find(networkId);
    if (iter != m_mapNetworks.end())
    {
        CSymubruitNetworkInstance * pToDelete = iter->second;

        {
            // don't remove the network while a parallel shortest path computation is in progress on it
            boost::unique_lock<boost::shared_mutex> lock(pToDelete->m_ShortestPathsComputeMutex);
            iter = m_mapNetworks.erase(iter);
        }

        delete pToDelete;
    }
}

void CSymubruitApp::AddNetwork(int networkId, Reseau * pNetwork)
{
    boost::unique_lock<boost::shared_mutex> lock(m_NetworksAccessMutex);

    assert(m_mapNetworks.find(networkId) == m_mapNetworks.end());

    m_mapNetworks[networkId] = new CSymubruitNetworkInstance(pNetwork);
}


/// <summary>
/// Méthode exportée de déchargement de la simulation
/// </summary>
///<returns>0 si OK</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymQuit()
{
    theApp.DeleteAllNetworks();
 
	return 0;
}

/// Fonction de sauvegarde de l'état d'une simulation
/// Le paramettre est le chemin du fichier de sauvegarde
SYMUBRUIT_EXPORT void SYMUBRUIT_CDECL SymSaveState(char* sXmlDataFile)
{
    SerializeUtil::Save(theApp.GetNetwork(DEFAULT_NETWORK_ID), sXmlDataFile);
}

/// Fonction de chargement de l'état d'une simulation
/// Le paramettre est le chemin du fichier de sauvegarde
SYMUBRUIT_EXPORT void SYMUBRUIT_CDECL SymLoadState(char* sXmlDataFile)
{
    // suppression de l'ancien réseau et instanciation d'un nouveau
    theApp.DeleteNetwork(DEFAULT_NETWORK_ID);

    Reseau * pNetwork = new Reseau();
    theApp.AddNetwork(DEFAULT_NETWORK_ID, pNetwork);

    // désérialisation de la sauvegarde dans le nouvel objet réseau
    SerializeUtil::Load(pNetwork, sXmlDataFile);
}

/// Fonction permettant de renvoyer une chaîne de caractères contenant l'ensemble de IDs des scénarios présents dans le fichier
/// Le paramettre est le chemin du fichier de définition des scénarios
SYMUBRUIT_EXPORT LPSTR SYMUBRUIT_CDECL SymGetScenariosIDs(LPSTR sXmlDataFile)
{
    LPSTR lpResult;
    std::string result = "";

    XMLUtil xmlUtil;
    std::vector<ScenarioElement> scenarii = xmlUtil.GetScenarioElements(sXmlDataFile);

    for (size_t i = 0; i < scenarii.size(); i++)
    {
        if (i > 0)
        {
            result = result.append(" | ");
        }
        result = result.append(scenarii[i].id);
    }

    lpResult = new char[result.length() + 1];
    strcpy(lpResult, result.c_str());
    
    return lpResult;
}

/// Fonction permettant de renvoyer une chaîne de caractères contenant l'ensemble des valeurs de préfixe utilisés
/// Le paramettre est le chemin du fichier XML définissant les préfixes
SYMUBRUIT_EXPORT LPSTR SYMUBRUIT_CDECL SymGetPrefixValues(LPSTR sXmlDataFile)
{
    LPSTR lpResult;
    std::string result = "";

    XMLUtil xmlUtil;
    std::vector<ScenarioElement> scenarii = xmlUtil.GetScenarioElements(sXmlDataFile);

    for (size_t i = 0; i < scenarii.size(); i++)
    {
        if (i > 0)
        {
            result = result.append(" | ");
        }
        result = result.append(scenarii[i].dirout);
        result += DIRECTORY_SEPARATOR;
        result = result.append(scenarii[i].prefout);
    }

    lpResult = new char[result.length() + 1];
    strcpy(lpResult, result.c_str());
    
    return lpResult;
}

/// Fonction permettant de renvoyer une chaîne de caractères contenant le préfixe utilisé pour un scénario particulier
/// Le paramettre 1 est le chemin du fichier XML définissant les préfixes
/// Le paramettre 2 est l'identifiant du scenario dont on veut connaite le prefixe
SYMUBRUIT_EXPORT LPSTR SYMUBRUIT_CDECL SymGetPrefixValue(LPSTR sXmlDataFile, LPSTR sXmlScenarioID)
{
    LPSTR lpResult;
    std::string result = "";

    XMLUtil xmlUtil;
    std::vector<ScenarioElement> scenarii = xmlUtil.GetScenarioElements(sXmlDataFile);

    for (size_t i = 0; i < scenarii.size(); i++)
    {
        if (scenarii[i].id == std::string(sXmlScenarioID))
        {
            result = result.append(scenarii[i].dirout);
            result += DIRECTORY_SEPARATOR;
            result = result.append(scenarii[i].prefout);
            break;
        }
    }

    lpResult = new char[result.length() + 1];
    strcpy(lpResult, result.c_str());
    
    return lpResult;
}

///<summary>
///Méthode exportée de création d'un véhicule spécifique
///</summary>
///<param name="sType">Type du véhicule à créer</param>
///<param name="sEntree">Entrée où le véhicule doit être généré</param>
///<param name="sSortie">Sortie vers laquelle le véhicule doit se diriger</param>
///<param name="dbt">Instant exact au cours du pas de temps de création du véhicule</param>
///<returns>ID du véhicule généré si OK, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle
(
	std::string sType,
	std::string sEntree,
	std::string sSortie,
	int			nVoie,
	double		dbt
)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return -1;
    }	
	
	return pReseau->CreateVehicle(sType, sEntree, sSortie, nVoie-1, dbt, NULL, std::string());	
}

///<summary>
///Méthode exportée de suppression d'un véhicule spécifique
///</summary>
///<param name="sType">ID du véhicule à supprimer</param>
///<returns>si 1 succès, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymDeleteVehicle
(
	int	nID
)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return -1;
    }	

	return pReseau->DeleteVehicle(nID);	
}

///<summary>
///Méthode exportée de création d'un véhicule spécifique avec un itinéraire prédéfini
///</summary>
///<param name="sType">Type du véhicule à créer</param>
///<param name="sEntree">Entrée où le véhicule doit être généré</param>
///<param name="sSortie">Sortie vers laquelle le véhicule doit se diriger</param>
///<param name="dbt">Instant exact au cours du pas de temps de création du véhicule</param>
///<param name="links">Tronçons constituant l'itinéraire pour le véhicule créé</param>
///<returns>ID du véhicule généré si OK, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle
(
	char*       originId,
    char*       destinationId,
    char*       typeVeh,
	int			nVoie,
	double		dbt,
    const char*  links[]
)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return -1;
    }

	// Ajouter pour gérer la possible création de véhicules avant le calcul du premier pas de temps
	/*if (!pReseau->IsInitSimuTrafic())
		if (!pReseau->InitSimuTrafic())
		{
			return -2;
		}*/

    std::deque<string> dqLinks;
    int iLink = 0;
    while(links[iLink] != NULL)
    {
         string strlinkId = links[iLink];
         dqLinks.push_back(strlinkId);
         iLink++;
    }

	if (nVoie == -1)
	{
		char cTmp;
		SymuViaTripNode *pOrigine = pReseau->GetOrigineFromID(originId, cTmp);
		
		if (pOrigine && cTmp == 'E')
		{			
			Connexion* pCnx = pOrigine->GetOutputConnexion();

			int nbVoie = pCnx->m_LstTuyAv[0]->getNbVoiesDis();

			if (nbVoie > 1)
			{
				nVoie = 0;

				// tirage :
                double dbRand = pReseau->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
				nVoie = (int)ceil(dbRand * nbVoie);	

			}
		}
	}

	return pReseau->CreateVehicle(typeVeh, originId, destinationId, nVoie==-1?-1:nVoie-1, dbt, &dqLinks, std::string());	
}

///<summary>
///Méthode exportée de création d'un véhicule spécifique avec un itinéraire prédéfini
///</summary>
///<param name="sType">Type du véhicule à créer</param>
///<param name="sEntree">Entrée où le véhicule doit être généré</param>
///<param name="sSortie">Sortie vers laquelle le véhicule doit se diriger</param>
///<param name="dbt">Instant exact au cours du pas de temps de création du véhicule</param>
///<param name="links">Tronçons constituant l'itinéraire pour le véhicule créé</param>
///<returns>ID du véhicule généré si OK, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle
(
	char*				originId,
	char*				destinationId,
	char*				typeVeh,
	int					nVoie,
	double				dbt,
	std::deque<string>	dqLinks
)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

	// Pas de réseau chargé, sortie sur erreur
	if (!pReseau)
	{
		return -1;
	}

	// Ajouter pour gérer la possible création de véhicules avant le calcul du premier pas de temps
	/*if (!pReseau->IsInitSimuTrafic())
		if (!pReseau->InitSimuTrafic())
		{
			return -2;
		}*/

	if (nVoie == -1)
	{
		char cTmp;
		SymuViaTripNode *pOrigine = pReseau->GetOrigineFromID(originId, cTmp);

		if (pOrigine && cTmp == 'E')
		{
			Connexion* pCnx = pOrigine->GetOutputConnexion();

			int nbVoie = pCnx->m_LstTuyAv[0]->getNbVoiesDis();

			if (nbVoie > 1)
			{
				nVoie = 0;

				// tirage :
				double dbRand = pReseau->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
				nVoie = (int)ceil(dbRand * nbVoie);

			}
		}
	}

	return pReseau->CreateVehicle(typeVeh, originId, destinationId, nVoie == -1 ? -1 : nVoie - 1, dbt, &dqLinks, std::string());
}

///<summary>
///Méthode exportée de création d'un véhicule spécifique avec un itinéraire prédéfini, sans numéro de voie
///</summary>
///<param name="sType">Type du véhicule à créer</param>
///<param name="sEntree">Entrée où le véhicule doit être généré</param>
///<param name="sSortie">Sortie vers laquelle le véhicule doit se diriger</param>
///<param name="dbt">Instant exact au cours du pas de temps de création du véhicule</param>
///<param name="links">Tronçons constituant l'itinéraire pour le véhicule créé</param>
///<returns>ID du véhicule généré si OK, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle
(
    char*       originId,
    char*       destinationId,
    char*       typeVeh,
    double		dbt,
    const char*  links[]
)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if (!pReseau)
    {
        return -1;
    }

    std::deque<string> dqLinks;
    int iLink = 0;
    while (links[iLink] != NULL)
    {
        string strlinkId = links[iLink];
        dqLinks.push_back(strlinkId);
        iLink++;
    }

    return pReseau->CreateVehicle(typeVeh, originId, destinationId, -1, dbt, &dqLinks, std::string());
}


///<summary>
///Méthode exportée de création d'un véhicule spécifique avec un itinéraire prédéfini, sans numéro de voie
///et avec un numéro d'utilisateur externe (utilisé par SymuMaster, par exemple)
///</summary>
///<param name="networkId">Identificateur unique du réseau sur lequel créer le véhicule</param>
///<param name="sType">Type du véhicule à créer</param>
///<param name="sEntree">Entrée où le véhicule doit être généré</param>
///<param name="sSortie">Sortie vers laquelle le véhicule doit se diriger</param>
///<param name="dbt">Instant exact au cours du pas de temps de création du véhicule</param>
///<param name="links">Tronçons constituant l'itinéraire pour le véhicule créé</param>
///<param name="junctionName">Identificateur unique du point de jonction à utiliser (uniquement si links est vide)</param>
///<param name="externalUserID">Identificateur unique de l'utilisateur correspondant géré par le programme appelant</param>
///<param name="strNextRouteId">Identificateur de la route aval 'pour hybridation SymuMaster avec SimuRes</param>
///<returns>ID du véhicule généré si OK, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle
(
int         networkId,
char*       originId,
char*       destinationId,
char*       typeVeh,
double		dbt,
const char* links[],
char *      junctionName,
int         externalUserID,
const char* strNextRouteId = NULL
)
{
    Reseau* pReseau = theApp.GetNetwork(networkId);

    // Pas de réseau chargé, sortie sur erreur
    if (!pReseau)
    {
        return -1;
    }

    std::deque<string> dqLinks;
    int iLink = 0;
    while (links[iLink] != NULL)
    {
        string strlinkId = links[iLink];
        dqLinks.push_back(strlinkId);
        iLink++;
    }

    std::string strjunctionName;
    if (junctionName != NULL)
    {
        strjunctionName = junctionName;
    }

    std::string nextRouteId;
    if (strNextRouteId != NULL)
    {
        nextRouteId = strNextRouteId;
    }

    return pReseau->CreateVehicle(typeVeh, originId, destinationId, -1, dbt, &dqLinks, strjunctionName, externalUserID, nextRouteId);
}


///<summary>
///Méthode exportée de création d'un véhicule spécifique avec un itinéraire prédéfini, sans numéro de voie
///et avec un numéro d'utilisateur externe (utilisé par SymuMaster, par exemple)
///</summary>
///<param name="sType">Type du véhicule à créer</param>
///<param name="sEntree">Entrée où le véhicule doit être généré</param>
///<param name="sSortie">Sortie vers laquelle le véhicule doit se diriger</param>
///<param name="dbt">Instant exact au cours du pas de temps de création du véhicule</param>
///<param name="links">Tronçons constituant l'itinéraire pour le véhicule créé</param>
///<param name="junctionName">Identificateur unique du point de jonction à utiliser (uniquement si links est vide)</param>
///<param name="externalUserID">Identificateur unique de l'utilisateur correspondant géré par le programme appelant</param>
///<returns>ID du véhicule généré si OK, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateVehicle
(
char*       originId,
char*       destinationId,
char*       typeVeh,
double		dbt,
const char* links[],
char *      junctionName,
int         externalUserID
)
{
    return SymCreateVehicle(DEFAULT_NETWORK_ID, originId, destinationId, typeVeh, dbt, links, junctionName, externalUserID);
}

///<summary>
///Méthode exportée de lancement sur ordre d'une tournée de livraison
///</summary>
///<param name="sTournee">Identifiant de la tournée de livraison</param>
///<returns>ID du véhicule généré si OK, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreateDeliveryVehicle
(
	char*       sTournee
)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return -1;
    }

    Trip * pTrip = pReseau->GetDeliveryFleet()->GetTrip(sTournee);
	if(!pTrip)
		return -2;

	return pReseau->CreateVehicle(pTrip, pReseau->GetDeliveryFleet());	
}

///<summary>
///Méthode exportée d'ajout d'un point de livraison à une tournée
///</summary>
///<param name="sTournee">Identifiant de la tournée de livraison. NULL si on veut modifier la tournée d'un véhicule seulement, cf. paramètre vehicleId</param>
///<param name="vehiculeId">Identifiant du véhicule dont la tournée doit être modifiée. Paramètre ignoré si le paramètre sTournee n'est pas NULL</param>
///<param name="sPointDeLivraison">Identifiant du point de livraison à ajouter à la tournée</param>
///<param name="positionIndex">Indice du point de livraison dans la tournée auquel insérer le nouveau point de livraison</param>
///<param name="dechargement">Quantité à décharger au nouveau point de livraison</param>
///<param name="chargement">Quantité à décharger au nouveau point de livraison</param>
///<returns>0 si OK, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymAddDeliveryPoint
(
	char*       sTournee,
    int         vehiculeId,
    char*       sPointDeLivraison,
    int         positionIndex,
    int         dechargement,
    int         chargement
)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return -1;
    }

    Trip * pTournee = NULL;
    if(sTournee != NULL)
    {
        pTournee = pReseau->GetDeliveryFleet()->GetTrip(sTournee);
        if(pTournee == NULL)
        {
            return -2;
        }
    }

    PointDeLivraison * pPoint = dynamic_cast<PointDeLivraison*>(pReseau->GetDeliveryFleet()->GetTripNode(sPointDeLivraison));

    if(!pPoint)
    {
        return -3;
    }

	return pReseau->AddDeliveryPoint(pTournee, vehiculeId, pPoint, positionIndex, dechargement, chargement);	
}

///<summary>
///Méthode exportée de suppression d'un point de livraison d'une tournée
///</summary>
///<param name="sTournee">Identifiant de la tournée de livraison. NULL si on veut modifier la tournée d'un véhicule seulement, cf. paramètre vehicleId</param>
///<param name="vehiculeId">Identifiant du véhicule dont la tournée doit être modifiée. Paramètre ignoré si le paramètre sTournee n'est pas NULL</param>
///<param name="positionIndex">Indice du point de livraison dans la tournée à supprimer</param>
///<returns>0 si OK, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymRemoveDeliveryPoint
(
	char*       sTournee,
    int         vehiculeId,
    int         positionIndex
)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return -1;
    }

    Trip * pTournee = NULL;
    if(sTournee != NULL)
    {
        pTournee = pReseau->GetDeliveryFleet()->GetTrip(sTournee);
        if(pTournee == NULL)
        {
            return -2;
        }
    }

	return pReseau->RemoveDeliveryPoint(pTournee, vehiculeId, positionIndex);	
}

// <summary>
/// Méthode exportée de pilotage d'un véhicule spécifique
/// </summary>
///<param name="nID">ID du véhicule à p</param>
///<param name="sTroncon">Tronçon sur lequel se trouve le véhicule à la fin du pas de temps</param>
///<param name="nVoie">Voie sur lequel se trouve le véhicule à la fin du pas de temps</param>
///<param name="dbPos">Position du véhicule à la fin du pas de temps</param>
///<param name="bForce">Indique si la position du véhicule doit être adaptée afin de tenir compte
/// de son environnement (vit. max., feux, leader, ...)</param>
///<returns>ID du véhicule généré si OK, sinon code erreur négatif</returns>
SYMUBRUIT_EXPORT double SYMUBRUIT_CDECL SymDriveVehicle
(
	int				nID,	
	std::string	    sTroncon,
	int				nVoie,
	double			dbPos,
    bool            bForce
)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return -1;
    }	

	return pReseau->DriveVehicle(nID, sTroncon, nVoie-1, dbPos, bForce);	
}


SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymRunNextStepNode(bool bTrace, bool &bNEnd){
	
    if(bNEnd)
		return NULL;

     /*   if( bNEnd )
    {
        theApp.m_pReseau->FinSimuTrafic();
        if(theApp.m_pReseau->IsSimuAcoustique() || theApp.m_pReseau->IsSimuAir() || theApp.m_pReseau->IsSimuSirane())
            theApp.m_pReseau->FinSimuEmissions(theApp.m_pReseau->IsSimuAcoustique(), theApp.m_pReseau->IsSimuAir(), theApp.m_pReseau->IsSimuSirane());        

        // Déchargement du réseau
        delete theApp.m_pReseau;
        theApp.m_pReseau = NULL;
        return NULL;
    }*/

    bNEnd = false;

	std::string sXmlFluxInstant;
	std::string sFlow;

    if (!_SymRunNextStep(DEFAULT_NETWORK_ID, sFlow, bTrace, bNEnd))
		return NULL;


	sXmlFluxInstant = sFlow;

	char* retValue = new char[sXmlFluxInstant.length()+1];
#ifdef WIN32
    strcpy_s(retValue, sXmlFluxInstant.length()+1,sXmlFluxInstant.c_str());
#else
    strcpy(retValue, sXmlFluxInstant.c_str());
#endif
	return retValue;
}

SYMUBRUIT_EXPORT char * SYMUBRUIT_CDECL SymRunNextStepJSON(bool &bNEnd)
{
    if(bNEnd)
		return NULL;

    bNEnd = false;

	std::string sJsonFluxInstant;

	if (!_SymRunNextStepJSON(sJsonFluxInstant, bNEnd))
	{
		//std::cout << sJsonFluxInstant << std::endl;
		return NULL;
	}

	char* retValue = new char[sJsonFluxInstant.length()+1];
#ifdef WIN32
    strcpy_s(retValue, sJsonFluxInstant.length()+1,sJsonFluxInstant.c_str());
#else
    strcpy(retValue, sJsonFluxInstant.c_str());
#endif

	return retValue;
}

SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetNetworkNode(){
	std::string sXmlFlowNetwork;
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return NULL;
    }
	
	sXmlFlowNetwork = pReseau->GetNetwork();   

	char* retValue = new char[sXmlFlowNetwork.length()+1];
#ifdef WIN32
    strcpy_s(retValue, sXmlFlowNetwork.length()+1, sXmlFlowNetwork.c_str());
#else
    strcpy(retValue, sXmlFlowNetwork.c_str());
#endif
	return retValue;
}

SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetNetworkJSON(){
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return NULL;
    }
	
	std::string sXmlFlowNetwork = pReseau->GetNetwork(true);   
	
	char* retValue = new char[sXmlFlowNetwork.length()+1];
#ifdef WIN32
    strcpy_s(retValue, sXmlFlowNetwork.length()+1, sXmlFlowNetwork.c_str());
#else
    strcpy(retValue, sXmlFlowNetwork.c_str());
#endif
	return retValue;
}

SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetParkingInfosJSON(){
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if (!pReseau || !pReseau->IsWithPublicTransportGraph())
    {
        return NULL;
    }

    std::string sXmlFlowNetwork = pReseau->GetParkingInfos();

    char* retValue = new char[sXmlFlowNetwork.length() + 1];
#ifdef WIN32
    strcpy_s(retValue, sXmlFlowNetwork.length() + 1, sXmlFlowNetwork.c_str());
#else
    strcpy(retValue, sXmlFlowNetwork.c_str());
#endif
    return retValue;
}

SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetVehiclesNodeAndFinish(){

    std::string sXmlFlow;
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {                
        return NULL;
    }
	
	sXmlFlow = pReseau->GetNodeVehicles();   

	char* retValue = new char[sXmlFlow.length()+1];
	for(size_t i=0; i < sXmlFlow.length(); i++){
		retValue[i] = sXmlFlow[i];
	}
	retValue[sXmlFlow.length()] = '\0';

    // finish simulation
    {
        pReseau->FinSimuTrafic();
        if (pReseau->IsSimuAcoustique() || pReseau->IsSimuAir() || pReseau->IsSimuSirane())
            pReseau->FinSimuEmissions(pReseau->IsSimuAcoustique(), pReseau->IsSimuAir(), pReseau->IsSimuSirane());

        // Déchargement du réseau
        theApp.DeleteNetwork(DEFAULT_NETWORK_ID);       
    }


	return retValue;
}

SYMUBRUIT_EXPORT void SYMUBRUIT_CDECL SymRunDeleteStr(char* str){
	if(str != NULL){
		delete(str);
	}
}

SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetVehiclesPaths( char*  vehiculeId[], int iLength)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    string returnString;
    returnString = "<PATHS> ";
 
    for (int i = 0; i< iLength ; ++i )
    {
        string str = vehiculeId[i];
        boost::shared_ptr<Vehicule>  spVehicule = pReseau->GetVehiculeFromID(atoi(str.c_str()));

        // On ne peut piloter que les véhicules de la flotte classique SymuVia
        if (spVehicule && spVehicule->GetFleet() == pReseau->GetSymuViaFleet())
        {
            returnString += "<PATH vehicleId =\""+ str +"\" destination =\""+spVehicule->GetDestination()->GetInputID()+"\" >";

            std::vector<Tuyau*> * pTuyauPaths =  spVehicule->GetItineraire();
                       
            std::vector<Tuyau*>::const_iterator itPath   ;
            // par path link
            for (itPath = pTuyauPaths->begin(); itPath !=  pTuyauPaths->end(); itPath++)
            {
                returnString += "<TRONCON tronconId=\"" + (*itPath)->m_strLabel  +"\"> </TRONCON>";  
            }
             returnString += "</PATH>";
        }
       
    }

    returnString += "</PATHS> ";
   
    char* retValue = new char[returnString.length()+1];
#ifdef WIN32
    strcpy_s(retValue, returnString.length()+1, returnString.c_str());
#else
    strcpy(retValue, returnString.c_str());
#endif
	return  retValue;
}


SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetVehiclePathJSON(int vehicleId)
{
    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    char * retValue = NULL;

    rapidjson::Document response(rapidjson::kObjectType);

    rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

    boost::shared_ptr<Vehicule>  spVehicule = pReseau->GetVehiculeFromID( vehicleId ); 

    if( spVehicule && spVehicule->GetFleet() == pReseau->GetSymuViaFleet())
    {
        std::vector<Tuyau*> * pTuyauPaths =  spVehicule->GetItineraire();
        std::vector<Tuyau*>::const_iterator itPath;
        rapidjson::Value path(rapidjson::kArrayType);
        for (itPath = pTuyauPaths->begin(); itPath !=  pTuyauPaths->end(); itPath++)
        {
            rapidjson::Value strValue(rapidjson::kStringType);
            strValue.SetString((*itPath)->m_strLabel.c_str(), (int)(*itPath)->m_strLabel.length(), allocator);
            path.PushBack(strValue, allocator);
        }
        response.AddMember("path", path, allocator);
    }

    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    response.Accept(writer);
    std::string returnString = buffer.GetString();
           
    retValue = new char[returnString.length()+1];
#ifdef WIN32
    strcpy_s(retValue, returnString.length()+1, returnString.c_str());
#else
    strcpy(retValue, returnString.c_str());
#endif
    return retValue;
}

// Renvoie une structure JSON donnant pour chaque tronçon restant dans l'itinéraire du véhicule,
// la vitesse moyenne mesurée par des capteurs EDIE présents sur les tronçons correspondants
// (les capteurs doivent avoir été définis par l'utilisateur dans le XML d'entrée)
SYMUBRUIT_EXPORT char* SYMUBRUIT_CDECL SymGetRemainingPathInfoJSON(int vehicleId)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);
	if (!pReseau)
	{
		return NULL;
	}

	boost::shared_ptr<Vehicule> pVeh = pReseau->GetVehiculeFromID(vehicleId);
	if (!pVeh)
	{
		return NULL;
	}

	std::string sJSON = pReseau->GetRemainingPathInfoJSON(pVeh);

	char* retValue = new char[sJSON.length() + 1];
#ifdef WIN32
	strcpy_s(retValue, sJSON.length() + 1, sJSON.c_str());
#else
	strcpy(retValue, sJSON.c_str());
#endif
	return retValue;
}

/// <summary>
/// Alter route function (for one vehicle)
/// </summary>
///<param name="nIdVeh">Vehicle ID</param>
///<param name="dqLinks">Links of route</param>
///<returns>0 if succeed else negative </returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymAlterRoute(int nIdVeh, char*  links[] , int iLength)
{
    std::deque<string> dqLinks;
    for( int i = 0; i <iLength; ++i)
    {
         string strlinkId = links[i];
         dqLinks.push_back(strlinkId);
    }

    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);    
    // No load network, error output
    if(!pReseau)
    {                
        return -1;
    }

	return pReseau->AlterRoute(nIdVeh, dqLinks);
}

SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymSetRoutes(char* originId, char* destinationId, char* typeVeh, char** links[] , double coeffs[], int iLength)
{

    Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);
    // No load network, error output
    if(!pReseau)
    {                
        return -1;
    }

    // Mise en forme des arguments
    std::vector<std::pair<double, std::vector<string> > > routes;
    for(int iRoute = 0; iRoute < iLength; iRoute++)
    {
        std::pair<double, std::vector<string> > route;
        route.first = coeffs[iRoute];
        int iTuyau = 0;
        while(links[iRoute][iTuyau] != NULL)
        {
            string strlinkId = links[iRoute][iTuyau];
            route.second.push_back(strlinkId);
            iTuyau++;
        }
        routes.push_back(route);
    }

	return pReseau->SetRoutes(originId, destinationId, typeVeh, routes);
}

/// <summary>
/// Fonction de calcul des itinéraires possibles entre deux noeuds
/// </summary>
///<param name="originId">Origin ID</param>
///<param name="destinationId">Destination ID</param>
///<param name="typeVeh">Vehicle type ID</param>
///<param name="dbTime">Simulation time</param>
///<param name="nbShortestPaths">Desired number of routes to compute</param>
///<param name="pathLinks">Computed path links</param>
///<param name="pathCosts">Computed path costs</param>
///<returns>0 if no error, else negative error code</returns>
SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymComputeRoutes(char* originId, char* destinationId, char* typeVeh, double dbTime, int nbShortestPaths,
    char ***pathLinks[], double* pathCosts[])
{
    theApp.m_NetworksAccessMutex.lock();

    CSymubruitApp::CSymubruitNetworkInstance * pNetworkInstance = theApp.GetNetworkInstance(DEFAULT_NETWORK_ID);

    if (!pNetworkInstance)
    {
        theApp.m_NetworksAccessMutex.unlock();
        return -1;
    }

    Reseau * pReseau = pNetworkInstance->GetNetwork();

    boost::shared_lock<boost::shared_mutex> lock(pNetworkInstance->m_ShortestPathsComputeMutex);

    theApp.m_NetworksAccessMutex.unlock();

    std::vector<std::pair<double, std::vector<std::string> > > paths;
    std::vector<Tuyau*> placeHolder;
    double refPathCost;
    int returnCode = pReseau->ComputeRoutes(originId, destinationId, typeVeh, dbTime, nbShortestPaths, 0, placeHolder, placeHolder, paths, refPathCost);

    *pathLinks = new char**[paths.size()+1];
    *pathCosts = new double[paths.size()];
    for(size_t iPath = 0; iPath < paths.size(); iPath++)
    {
        (*pathCosts)[iPath] = paths[iPath].first;

        const std::vector<std::string> & links = paths[iPath].second;
        (*pathLinks)[iPath] = new char*[links.size()+1];
        for(size_t iLink = 0; iLink < links.size(); iLink++)
        {
            std::string linkId = links[iLink];
            (*pathLinks)[iPath][iLink] = new char[linkId.length()+1];
#ifdef WIN32
            strcpy_s((*pathLinks)[iPath][iLink], linkId.length()+1, linkId.c_str());
#else
            strcpy((*pathLinks)[iPath][iLink], linkId.c_str());
#endif
        }
        (*pathLinks)[iPath][links.size()] = NULL;
    }
    (*pathLinks)[paths.size()] = NULL;
    

    return returnCode;
}

bool isSameLink(Tuyau * pTuyau, void * pArg)
{
    Tuyau * pTuyauRef = (Tuyau*)pArg;
    return pTuyauRef == pTuyau;
}

SYMUBRUIT_EXPORT double SYMUBRUIT_CDECL SymGetCurrentPathCost(int idVeh, char * linkId)
{
    theApp.m_NetworksAccessMutex.lock();

    CSymubruitApp::CSymubruitNetworkInstance * pNetworkInstance = theApp.GetNetworkInstance(DEFAULT_NETWORK_ID);

    if (!pNetworkInstance)
    {
        theApp.m_NetworksAccessMutex.unlock();
        return -1;
    }

    Reseau * pReseau = pNetworkInstance->GetNetwork();

    boost::shared_lock<boost::shared_mutex> lock(pNetworkInstance->m_ShortestPathsComputeMutex);

    theApp.m_NetworksAccessMutex.unlock();

    Tuyau *pLink = pReseau->GetLinkFromLabel(linkId);
    if (!pLink)
    {
        return -2;
    }

    Connexion *pUpNode = pLink->GetCnxAssAm();
    if (!pUpNode)
    {
        return -3;
    }

    // rmq. en cas d'appel asynchrone, il faudrait en toute rigueur synchroniser également les accès à la liste des véhicules
    // (mais a priori le cas où on fait une requête d'alternative pour un véhicule qui serait détruit entre temps est "impossible" avec le SG).
    // Par contre, on synchronise les accès à GetItineraireIndex car les variables utilisées sont potentiellement modifiées en parallèle par le SymRunNextStep
    boost::shared_ptr<Vehicule> pVehicule = pReseau->GetVehiculeFromID(idVeh);
    if (!pVehicule)
    {
        return -4;
    }

    std::vector<Tuyau*> remainingPath;
    std::vector<Tuyau*> itineraire;
    int itiIndex = pVehicule->GetItineraireIndexThreadSafe(isSameLink, pLink, itineraire);

    for (size_t i = itiIndex; i < itineraire.size(); i++)
    {
        remainingPath.push_back(itineraire.at(i));
    }

    double refPathCost = pReseau->GetSymuScript()->ComputeCost(pVehicule->GetType(), remainingPath, false);
    
    return refPathCost;
}

SYMUBRUIT_EXPORT double SYMUBRUIT_CDECL SymGetPathCost(char* path[], char* typeVeh, double dbTime)
{
    theApp.m_NetworksAccessMutex.lock();

    CSymubruitApp::CSymubruitNetworkInstance * pNetworkInstance = theApp.GetNetworkInstance(DEFAULT_NETWORK_ID);

    if (!pNetworkInstance)
    {
        theApp.m_NetworksAccessMutex.unlock();
        return -1;
    }

    Reseau * pReseau = pNetworkInstance->GetNetwork();

    boost::shared_lock<boost::shared_mutex> lock(pNetworkInstance->m_ShortestPathsComputeMutex);

    theApp.m_NetworksAccessMutex.unlock();

    TypeVehicule *pTV = pReseau->GetVehicleTypeFromID(typeVeh);
    if (!pTV)
    {
        return -2;
    }

    std::vector<Tuyau*> pathLinks;
    int linkIdx = 0;

    if (path)
    {
        while (path[linkIdx] != NULL)
        {
            Tuyau * pLink = pReseau->GetLinkFromLabel(path[linkIdx]);

            if (!pLink)
                return -3;

            pathLinks.push_back(pLink);
            linkIdx++;
        }
    }

    double refPathCost = pReseau->GetSymuScript()->ComputeCost(pTV, pathLinks, false);

    return refPathCost;
}


SYMUBRUIT_EXPORT char * SYMUBRUIT_CDECL SymGetPathInfoJSON(const std::vector<std::string> & links, char* typeVeh, double dbTime)
{
    theApp.m_NetworksAccessMutex.lock();

    CSymubruitApp::CSymubruitNetworkInstance * pNetworkInstance = theApp.GetNetworkInstance(DEFAULT_NETWORK_ID);

    if (!pNetworkInstance)
    {
        theApp.m_NetworksAccessMutex.unlock();
        return NULL;
    }

    Reseau * pReseau = pNetworkInstance->GetNetwork();

    boost::shared_lock<boost::shared_mutex> lock(pNetworkInstance->m_ShortestPathsComputeMutex);

    theApp.m_NetworksAccessMutex.unlock();

    TypeVehicule *pTV = pReseau->GetVehicleTypeFromID(typeVeh);
    if (!pTV)
    {
        return NULL;
    }

    std::vector<Tuyau*> pathLinks;
    for (size_t iLink = 0; iLink < links.size(); iLink++)
    {
        Tuyau * pLink = pReseau->GetLinkFromLabel(links[iLink]);

        if (!pLink)
            return NULL;

        pathLinks.push_back(pLink);
    }

    // Distance euclidienne ente le noeud point de choix et la destination
    double dbDistance = sqrt((pathLinks.back()->GetAbsAval() - pathLinks.front()->GetAbsAval())*(pathLinks.back()->GetAbsAval() - pathLinks.front()->GetAbsAval())
        + (pathLinks.back()->GetOrdAval() - pathLinks.front()->GetOrdAval())*(pathLinks.back()->GetOrdAval() - pathLinks.front()->GetOrdAval()));

    // Calcul de la longueur
    const std::vector<Tuyau*> fullPath = pReseau->ComputeItineraireComplet(pathLinks);
    double dbLength = 0;
    double dbFunc1Length = 0;
    double dbFunc2Length = 0;
    for (size_t iLink = 0; iLink < fullPath.size(); iLink++)
    {
        dbLength += fullPath[iLink]->GetLength();

        // we assume internal links are not highways.
        if (!fullPath[iLink]->GetBriqueParente())
        {
            if (fullPath[iLink]->GetFunctionalClass() == 1)
            {
                dbFunc1Length += fullPath[iLink]->GetLength();
            }
            else if(fullPath[iLink]->GetFunctionalClass() == 2)
            {
                dbFunc2Length += fullPath[iLink]->GetLength();
            }
        }
    }

    // Calcul du cout
    double pathCost = pReseau->GetSymuScript()->ComputeCost(pTV, pathLinks, false);

    // Pourcentage d'autoroute
    double dbFunc1Ratio = dbFunc1Length / dbLength;
    double dbFunc2Ratio = dbFunc2Length / dbLength;

    // nombre de turnings
    int nbTurnings = 0;
    for (int iLink = 0; iLink < (int)pathLinks.size()-1; iLink++)
    {
        Tuyau * pUpLink = pathLinks.at(iLink);
        Connexion * pNode = pathLinks[iLink]->GetCnxAssAv();

        bool bHasAlignedNextLink = false;
        bool bIsOutside90DegreesCone = false;
        for (size_t iDownLink = 0; iDownLink < pNode->m_LstTuyAssAv.size(); iDownLink++)
        {
            double dbAngle;

            Tuyau * pDownLink = pNode->m_LstTuyAssAv.at(iDownLink);

            if (pNode->IsMouvementAutorise(pUpLink, pDownLink, pTV, NULL))
            {
                double dbPrevX = pUpLink->GetLstPtsInternes().empty() ? pUpLink->GetAbsAmont() : pUpLink->GetLstPtsInternes().back()->dbX;
                double dbPrevY = pUpLink->GetLstPtsInternes().empty() ? pUpLink->GetOrdAmont() : pUpLink->GetLstPtsInternes().back()->dbY;

                double dbNextX = pDownLink->GetLstPtsInternes().empty() ? pDownLink->GetAbsAval() : pDownLink->GetLstPtsInternes().front()->dbX;
                double dbNextY = pDownLink->GetLstPtsInternes().empty() ? pDownLink->GetOrdAval() : pDownLink->GetLstPtsInternes().front()->dbY;

                double a1 = pUpLink->GetAbsAval() - dbPrevX;
                double a2 = pUpLink->GetOrdAval() - dbPrevY;

                double b1 = dbNextX - pDownLink->GetAbsAmont();
                double b2 = dbNextY - pDownLink->GetOrdAmont();

                double a = (a1*a1) + (a2*a2);
                double b = (b1*b1) + (b2*b2);

                if ((a == 0.0) || (b == 0.0))
                {
                    dbAngle = 0.0;
                }
                else
                {
                    // atan2 renvoie un angle dans le sens trigo compris entre -pi et +pi.
                    dbAngle = atan2(a1*b2 - a2*b1, a1*b1 + a2*b2);
                }

                // we test +/-45° so we have a 90° cone
                if (pDownLink == pathLinks.at(iLink + 1))
                {
                    // RMQ. we don't use m_pReseau->GetAngleToutDroit() here because it is worth 30° and not 45° by default, and don't want to change the value
                    // to not change the shortest paths travel times displayed to the players with the default value.
                    if (fabs(dbAngle) >= M_PI_4)
                    {
                        bIsOutside90DegreesCone = true;
                    }
                }
                else
                {
                    if (fabs(dbAngle) < M_PI_4)
                    {
                        bHasAlignedNextLink = true;
                    }
                }
            }
        }

        if (bHasAlignedNextLink && bIsOutside90DegreesCone)
        {
            nbTurnings++;
        }
    }   

    // produce JSON result struct
    rapidjson::Document response(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

    //response.AddMember("cost", pathCost, allocator);
    response.AddMember("length", dbLength, allocator);
    response.AddMember("euclDistance", dbDistance, allocator);
    response.AddMember("turnings", nbTurnings, allocator);
    response.AddMember("func1", dbFunc1Ratio, allocator);
    response.AddMember("func2", dbFunc2Ratio, allocator);

    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    response.Accept(writer);
    std::string returnString = buffer.GetString();

    char * retValue = new char[returnString.length() + 1];

#ifdef WIN32
    strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
    strcpy(retValue, returnString.c_str());
#endif

    return retValue;
}

/// <summary>
/// Fonction de calcul des itinéraires alternatifs entre deux noeuds avec sortie au format JSON
/// La fonction retourne un chemin possible passant par un lien aval du noeud d'origine différent du lien considéré
/// </summary>
///<param name="linkId">Lien aval de l'origine étudié - ne doit pas être proposé comme premier lien des itinéraires alternatifs calculés </param>
///<param name="prevLinkId">Lien précédent l'origine étudié - afin de prendre en compte les mouvements interdits</param>
///<param name="destinationId">Destination ID</param>
///<param name="typeVeh">Vehicle type ID</param>
///<param name="dbTime">Simulation time</param>
///<param name="method">Calculation method</param>
///<param name="idVeh">ID of the vehicle to compute the reference path cost(-1 if none)</param>
///<returns>JSON structure with desired result</returns>
SYMUBRUIT_EXPORT char * SYMUBRUIT_CDECL SymGetAlternatePathsFromNodeJSON(char* linkId, char* prevLinkId, char* destinationId, char* typeVeh, double dbTime, int nMethod, int idVeh)
{
	char* retValue = NULL;
	rapidjson::Document response(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

    response.AddMember("alternate paths", rapidjson::Value(rapidjson::kArrayType), allocator);

    std::string returnString;

    {
        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        response.Accept(writer);
        returnString = buffer.GetString();
    }
           
	retValue = new char[returnString.length()+1];

    theApp.m_NetworksAccessMutex.lock();

    CSymubruitApp::CSymubruitNetworkInstance * pNetworkInstance = theApp.GetNetworkInstance(DEFAULT_NETWORK_ID);

    if (!pNetworkInstance)
    {
        theApp.m_NetworksAccessMutex.unlock();
#ifdef WIN32
        strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
        strcpy(retValue, returnString.c_str());
#endif
        return retValue;
    }

    Reseau * pReseau = pNetworkInstance->GetNetwork();

    boost::shared_lock<boost::shared_mutex> lock(pNetworkInstance->m_ShortestPathsComputeMutex);

    theApp.m_NetworksAccessMutex.unlock();

	// If origin id is destination id, error output
	if (strcmp(linkId, destinationId) == 0)
	{
#ifdef WIN32
		strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
		strcpy(retValue, returnString.c_str());
#endif
		return retValue;
	}

	// If origin id is destination id, error output
	if (strcmp(prevLinkId, destinationId) == 0)
	{
#ifdef WIN32
		strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
		strcpy(retValue, returnString.c_str());
#endif
		return retValue;
	}

	Tuyau *pLink = pReseau->GetLinkFromLabel(linkId);
	if(!pLink)
		{    
		#ifdef WIN32
			strcpy_s(retValue, returnString.length()+1, returnString.c_str());
		#else
			strcpy(retValue, returnString.c_str());
		#endif
        return retValue;
		//	return "-1";
    }

	Tuyau *pPrevLink = pReseau->GetLinkFromLabel(prevLinkId);
	if(!pPrevLink)
		{    
		#ifdef WIN32
			strcpy_s(retValue, returnString.length()+1, returnString.c_str());
		#else
			strcpy(retValue, returnString.c_str());
		#endif
        return retValue;
		//	return "-2";
    }

	Connexion *pUpNode = pLink->GetCnxAssAm();
	if(!pUpNode)
		{    
		#ifdef WIN32
			strcpy_s(retValue, returnString.length()+1, returnString.c_str());
		#else
			strcpy(retValue, returnString.c_str());
		#endif
        return retValue;
		//	return "-3";
    }

	if(pPrevLink->GetCnxAssAv()!=pUpNode)
		{    
		#ifdef WIN32
			strcpy_s(retValue, returnString.length()+1, returnString.c_str());
		#else
			strcpy(retValue, returnString.c_str());
		#endif
        return retValue;
		//	return "-4";
    }

	if(pUpNode->GetNbElAssAval() == 1)	// No alernate path
		{    
		#ifdef WIN32
			strcpy_s(retValue, returnString.length()+1, returnString.c_str());
		#else
			strcpy(retValue, returnString.c_str());
		#endif
        return retValue;		
    }

	char cType;
	SymuViaTripNode *pDestinationNode = pReseau->GetDestinationFromID(destinationId, cType );
    Connexion *pDestinationCon = NULL;
    Tuyau *pTD = NULL;
	Tuyau *pLinkDst = NULL;

    if (!pDestinationNode)
	{ 
        pDestinationCon = pReseau->GetConnectionFromID(destinationId, cType);
        if (!pDestinationCon)
        {
            pDestinationCon = pReseau->GetBrickFromID(destinationId);
        }
		else
		
        if (!pDestinationCon)
        {
            pLinkDst = pReseau->GetLinkFromLabel(destinationId);
            if (pLinkDst)
            {
                pTD = pLinkDst;
            }
            else
            {
#ifdef WIN32
                strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
                strcpy(retValue, returnString.c_str());
#endif
                //return "-6";
                return retValue;
            }
        }
		else
		{
			// No alternate path if destination node is the downstream of the link
			if (pDestinationCon == pLink->GetCnxAssAv())
			{
#ifdef WIN32
				strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
				strcpy(retValue, returnString.c_str());
#endif
				return retValue;
			}

			// No alternate path if destination node is the downstream of previous link
			if (pDestinationCon == pPrevLink->GetCnxAssAv())
			{
#ifdef WIN32
				strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
				strcpy(retValue, returnString.c_str());
#endif
				return retValue;
			}
		}
	}
	else
	{
		{
			if (pDestinationNode->GetInputConnexion())
			{
				// No alternate path if destination node is the downstream of the link
				if (pDestinationNode->GetInputConnexion() == pLink->GetCnxAssAv())
				{
#ifdef WIN32
					strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
					strcpy(retValue, returnString.c_str());
#endif
					return retValue;
				}

				// No alternate path if destination node is the downstream of previous link
				if (pDestinationNode->GetInputConnexion() == pPrevLink->GetCnxAssAv())
				{
#ifdef WIN32
					strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
					strcpy(retValue, returnString.c_str());
#endif
					return retValue;
				}
			}
		}
	}
	
	TypeVehicule *pTV = pReseau->GetVehicleTypeFromID(typeVeh);
	if(!pTV)
	{
		#ifdef WIN32
		strcpy_s(retValue, returnString.length()+1, returnString.c_str());
#else
		strcpy(retValue, returnString.c_str());
#endif
		return retValue;
		//return "-7";
	}

    std::vector<Tuyau*> refPath;
    double refPathCost = 0;

    // Si un véhicule a été précisé, on calcule l'itinéraire restant depuis pLink :
    // il s'agit de l'itiéraire de référence.
    if (idVeh != -1)
    {
        // rmq. en cas d'appel asynchrone, il faut synchroniser l'accès à la liste des véhicules ici avec les endroits
        // où on la modifie (un simple push_back sur la liste ailleurs peut faire planter un GetVehiculeFromID non synchronisé)
        // on synchronise également les accès à GetItineraireIndex plus bas car les variables utilisées sont potentiellement modifiées en parallèle par le SymRunNextStep
        boost::shared_ptr<Vehicule> pVehicule = pReseau->GetVehiculeFromIDThreadSafe(idVeh);
        if (!pVehicule)
        {
            #ifdef WIN32
            strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
            #else
            strcpy(retValue, returnString.c_str());
            #endif

            return retValue;
			//return "-8";
        }

        std::vector<Tuyau*> itineraire;
        int itiIndex = pVehicule->GetItineraireIndexThreadSafe(isSameLink, pLink, itineraire);
        
        for (size_t i = itiIndex; i < itineraire.size(); i++)
        {
            refPath.push_back(itineraire.at(i));
        }
        // rmq. : pour cette fonction, on calcule le cout de référence ici pour ne pas être gené 
        // par la pénalisation arbitraire de pLink
        refPathCost = pReseau->GetSymuScript()->ComputeCost(pVehicule->GetType(), refPath, false);
		//std::cout << "refPathCost " << refPathCost << std::endl;
    }

    std::vector<Tuyau*> linksToAvoid;
    linksToAvoid.push_back(pLink);
    linksToAvoid.push_back(pPrevLink);

	// Search of potential downstream links
    rapidjson::Value alternatePaths(rapidjson::kArrayType);
	for(int i=0; i<pUpNode->GetNbElAssAval(); i++)
	{
		Tuyau* pDownLink = pUpNode->m_LstTuyAv[i];
		if(pDownLink!=pLink)
		{
            if( pUpNode->IsMouvementAutorise(pPrevLink, pDownLink, pTV, NULL) && !pDownLink->IsInterdit(pTV, dbTime))
			{
				std::vector<std::pair<double, std::vector<std::string> > > paths;

                // on récupère d'abord les éventuels chemins prédéfinis :
                if (!pReseau->GetPredefinedAlternativeRoutes(pTV, pDownLink, pDestinationNode, pTD, pDestinationCon, paths))
				{
					double refPathCostTmp;
					//std::cout << "ComputeRoutes 1 " << destinationId << std::endl;
					if (pReseau->ComputeRoutes(pDownLink->GetLabel(), destinationId, typeVeh, dbTime, 1, refPath.empty() ? 0 : nMethod, refPath, linksToAvoid, paths, refPathCostTmp) != 0)
					{
						paths.clear();
					}
				}
				if( paths.size() > 0)
				{
                    rapidjson::Value linksJSON(rapidjson::kArrayType);
					const std::vector<std::string> & links = paths[0].second;
					for(size_t iLink = 0; iLink < links.size(); iLink++)
					{
                        const std::string & linkStr = links[iLink];
                        rapidjson::Value strValue(rapidjson::kStringType);
                        strValue.SetString(linkStr.c_str(), (int)linkStr.length(), allocator);
                        linksJSON.PushBack(strValue, allocator);
					}
                    rapidjson::Value alternatePath(rapidjson::kObjectType);
                    rapidjson::Value path(rapidjson::kObjectType);
                    path.AddMember("links", linksJSON, allocator);
                    // SymuCore utilise à présent un cout INF pour un chemin imposible au lieu de DBL_MAX comme SymuScript.
                    // Hors, rapidjson ne gère pas par défaut les INF (il faudrait passer le flag kWriteNanAndInfFlag au Writer).
                    // Mais pour ne pas risquer d'avoir des plantages car le SG ne gérerait pas les INF, on les remplace par des DBL_MAX pour
                    // se ramener au cas précédent (même si ce n'est pas sensé arriver sur un itinéraire trouvé ici...)
                    path.AddMember("cost", std::min<double>(paths[0].first, DBL_MAX) , allocator);
                    alternatePath.AddMember("path", path, allocator);
                    alternatePaths.PushBack(alternatePath, allocator);
				}
			}
		}
	}

    response.RemoveAllMembers();
    response.AddMember("alternate paths", alternatePaths, allocator);
    if (refPathCost != 0)
    {
        // SymuCore utilise à présent un cout INF pour un chemin imposible au lieu de DBL_MAX comme SymuScript.
        // Hors, rapidjson ne gère pas par défaut les INF (il faudrait passer le flag kWriteNanAndInfFlag au Writer).
        // Mais pour ne pas risquer d'avoir des plantages car le SG ne gérerait pas les INF, on les remplace par des DBL_MAX pour
        // se ramener au cas précédent
        response.AddMember("reference cost", std::min<double>(refPathCost, DBL_MAX), allocator);
    }

    rapidjson::StringBuffer buffer;
    buffer.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    response.Accept(writer);
    returnString = buffer.GetString();
           
    retValue = new char[returnString.length()+1];

	
#ifdef WIN32
		strcpy_s(retValue, returnString.length()+1, returnString.c_str());
#else
		strcpy(retValue, returnString.c_str());
#endif

	return retValue;
}

/// <summary>
/// Fonction de calcul des itinéraires possibles entre deux noeuds avec sortie au format JSON
/// </summary>
///<param name="originId">Origin ID</param>
///<param name="destinationId">Destination ID</param>
///<param name="typeVeh">Vehicle type ID</param>
///<param name="dbTime">Simulation time</param>
///<param name="nbShortestPaths">Desired number of routes to compute</param>
///<returns>JSON structure with desired result</returns>
SYMUBRUIT_EXPORT char * SYMUBRUIT_CDECL SymGetAlternatePathsJSON(char* originId, char* destinationId, char* typeVeh, double dbTime, int nbShortestPaths)
{
    char * retValue = NULL;

    theApp.m_NetworksAccessMutex.lock();

    CSymubruitApp::CSymubruitNetworkInstance * pNetworkInstance = theApp.GetNetworkInstance(DEFAULT_NETWORK_ID);

    if (!pNetworkInstance)
    {
        theApp.m_NetworksAccessMutex.unlock();
        return retValue;
    }

    Reseau * pReseau = pNetworkInstance->GetNetwork();

    boost::shared_lock<boost::shared_mutex> lock(pNetworkInstance->m_ShortestPathsComputeMutex);

    theApp.m_NetworksAccessMutex.unlock();

    std::vector<std::pair<double, std::vector<std::string> > > paths;
    std::vector<Tuyau*> placeHolder;
    double refPathCost;
    int returnCode = pReseau->ComputeRoutes(originId, destinationId, typeVeh, dbTime, nbShortestPaths, 0, placeHolder, placeHolder, paths, refPathCost);

    if(returnCode == 0)
    {
        rapidjson::Document response(rapidjson::kObjectType);
        rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

        rapidjson::Value alternatePaths(rapidjson::kArrayType);

        for(size_t iPath = 0; iPath < paths.size(); iPath++)
        {
            rapidjson::Value linksJSON(rapidjson::kArrayType);
            const std::vector<std::string> & links = paths[iPath].second;
            for(size_t iLink = 0; iLink < links.size(); iLink++)
            {
                rapidjson::Value strValue(rapidjson::kStringType);
                const std::string & linkStr = links[iLink];
                strValue.SetString(linkStr.c_str(), (int)linkStr.length(), allocator);
                linksJSON.PushBack(strValue, allocator);
            }

            rapidjson::Value path(rapidjson::kObjectType);
            // SymuCore utilise à présent un cout INF pour un chemin imposible au lieu de DBL_MAX comme SymuScript.
            // Hors, rapidjson ne gère pas par défaut les INF (il faudrait passer le flag kWriteNanAndInfFlag au Writer).
            // Mais pour ne pas risquer d'avoir des plantages car le SG ne gérerait pas les INF, on les remplace par des DBL_MAX pour
            // se ramener au cas précédent
            path.AddMember("cost", std::min<double>(paths[iPath].first, DBL_MAX), allocator);
            path.AddMember("links", linksJSON, allocator);
            rapidjson::Value alternatePath(rapidjson::kObjectType);
            alternatePath.AddMember("path", path, allocator);
            alternatePaths.PushBack(alternatePath, allocator);
        }

        response.AddMember("alternate paths", alternatePaths, allocator);

        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        response.Accept(writer);
        std::string returnString = buffer.GetString();
           
        retValue = new char[returnString.length()+1];
#ifdef WIN32
        strcpy_s(retValue, returnString.length()+1, returnString.c_str());
#else
        strcpy(retValue, returnString.c_str());
#endif
    }

    return retValue;
}


/// <summary>
/// Fonction de calcul des itinéraires possibles entre deux noeuds avec sortie au format JSON, avec différentes méthodes 
/// Faisant intervenir le calcul de Commonality Factors.
/// </summary>
///<param name="originId">Origin ID</param>
///<param name="destinationId">Destination ID</param>
///<param name="typeVeh">Vehicle type ID</param>
///<param name="dbTime">Simulation time</param>
///<param name="nbShortestPaths">Desired number of routes to compute</param>
///<param name="refPath">Itinéraire de référence - peut-être null si pas d'itinéraire de référence - </param>
///<param name="method">Méthode de calcul à utiliser. 1 : les résultats incluent l'itinéraire de référence. 2 : les résultats n'incluent pas l'itinéraire de référence
///                     mais le commonality factor est elevé (les résultats peuvent être proches de l'itinéraire de référence). 3 : comme 2, mais avec un commonality
///                     factor maximum faible (les résultats doivent être éloignés de l'itinéraire de référence.
///</param>
///<returns>JSON structure with desired result</returns>
SYMUBRUIT_EXPORT char * SYMUBRUIT_CDECL SymGetAlternatePathsExJSON(char* originId, char* destinationId, char* typeVeh, double dbTime, int nbShortestPaths,
    char* refPath[], int method)
{
    char * retValue = NULL;

    theApp.m_NetworksAccessMutex.lock();

    CSymubruitApp::CSymubruitNetworkInstance * pNetworkInstance = theApp.GetNetworkInstance(DEFAULT_NETWORK_ID);

    if (!pNetworkInstance)
    {
        theApp.m_NetworksAccessMutex.unlock();
        return retValue;
    }

    Reseau * pReseau = pNetworkInstance->GetNetwork();

    boost::shared_lock<boost::shared_mutex> lock(pNetworkInstance->m_ShortestPathsComputeMutex);

    theApp.m_NetworksAccessMutex.unlock();

    std::vector<Tuyau*> refPathLinks;
    int linkIdx = 0;

	if(refPath)
	{
		while(refPath[linkIdx] != NULL)
		{
			Tuyau * pLink = pReseau->GetLinkFromLabel(refPath[linkIdx]);

			if (!pLink)
			{
				std::cerr << "SymGetAlternatePathsExJSON - Unknown link" << refPath[linkIdx] << std::endl;
				return NULL;
			}

			refPathLinks.push_back(pLink);
			linkIdx++;
		}
	}

    std::vector<Tuyau*> linksToAvoid;
    std::vector<std::pair<double, std::vector<std::string> > > paths;
    double refPathCost;
    int returnCode = pReseau->ComputeRoutes(originId, destinationId, typeVeh, dbTime, nbShortestPaths, method, refPathLinks, linksToAvoid, paths, refPathCost);

    if (returnCode == 0)
    {
        rapidjson::Document response(rapidjson::kObjectType);
        rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

        rapidjson::Value alternatePaths(rapidjson::kArrayType);

        for (size_t iPath = 0; iPath < paths.size(); iPath++)
        {
            rapidjson::Value linksJSON(rapidjson::kArrayType);
            const std::vector<std::string> & links = paths[iPath].second;
            for (size_t iLink = 0; iLink < links.size(); iLink++)
            {
                rapidjson::Value strValue(rapidjson::kStringType);
                const std::string & linkStr = links[iLink];
                strValue.SetString(linkStr.c_str(), (int)linkStr.length(), allocator);
                linksJSON.PushBack(strValue, allocator);
            }
            rapidjson::Value path(rapidjson::kObjectType);
            // SymuCore utilise à présent un cout INF pour un chemin imposible au lieu de DBL_MAX comme SymuScript.
            // Hors, rapidjson ne gère pas par défaut les INF (il faudrait passer le flag kWriteNanAndInfFlag au Writer).
            // Mais pour ne pas risquer d'avoir des plantages car le SG ne gérerait pas les INF, on les remplace par des DBL_MAX pour
            // se ramener au cas précédent
            path.AddMember("cost", std::min<double>(paths[iPath].first, DBL_MAX), allocator);
            path.AddMember("links", linksJSON, allocator);
            rapidjson::Value alternatePath(rapidjson::kObjectType);
            alternatePath.AddMember("path", path, allocator);
            alternatePaths.PushBack(alternatePath, allocator);
        }

        response.AddMember("alternate paths", alternatePaths, allocator);
        if (refPath)
        {
            // SymuCore utilise à présent un cout INF pour un chemin imposible au lieu de DBL_MAX comme SymuScript.
            // Hors, rapidjson ne gère pas par défaut les INF (il faudrait passer le flag kWriteNanAndInfFlag au Writer).
            // Mais pour ne pas risquer d'avoir des plantages car le SG ne gérerait pas les INF, on les remplace par des DBL_MAX pour
            // se ramener au cas précédent
            response.AddMember("reference cost", std::min<double>(refPathCost, DBL_MAX), allocator);
        }

        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        response.Accept(writer);
        std::string returnString = buffer.GetString();
           
        retValue = new char[returnString.length()+1];
#ifdef WIN32
        strcpy_s(retValue, returnString.length()+1, returnString.c_str());
#else
        strcpy(retValue, returnString.c_str());
#endif
    }
	else
		std::cerr << "SymGetAlternatePathsExJSON - Error code " << returnCode << " " << originId << " " << destinationId << std::endl;

    return retValue;
}

/// <summary>
/// Fonction de libération de la mémoire allouée par la fonction SymComputeRoutes
/// </summary>
///<param name="pathLinks">Computed path links</param>
///<param name="pathCosts">Computed path costs</param>
SYMUBRUIT_EXPORT void SYMUBRUIT_CDECL SymRunDeleteRoutes(char ***pathLinks[], double* pathCosts[])
{
	size_t iPath = 0;
    while((*pathLinks)[iPath] != NULL)
    {
        size_t iLink = 0;
        while((*pathLinks)[iPath][iLink] != NULL)
        {
            delete [] (*pathLinks)[iPath][iLink];
            iLink++;
        }
        delete [] (*pathLinks)[iPath];
        iPath++;
    }
    delete [] *pathLinks;
    delete [] *pathCosts;
}


/// <summary>
/// Fonction de calcul des itinéraires possibles entre deux noeuds avec sortie au format JSON en transports en commun uniquement.
/// </summary>
///<param name="originId">Origin ID</param>
///<param name="destinationId">Destination ID</param>
///<param name="dbTime">Simulation time</param>
///<param name="nbShortestPaths">Desired number of routes to compute</param>
///</param>
///<returns>JSON structure with desired result</returns>
SYMUBRUIT_EXPORT char * SYMUBRUIT_CDECL SymGetAlternatePathsPublicTransportJSON(char* originId, char* destinationId, double dbTime, int nbShortestPaths)
{
    char * retValue = NULL;

    theApp.m_NetworksAccessMutex.lock();

    CSymubruitApp::CSymubruitNetworkInstance * pNetworkInstance = theApp.GetNetworkInstance(DEFAULT_NETWORK_ID);

    if (!pNetworkInstance)
    {
        theApp.m_NetworksAccessMutex.unlock();
        return retValue;
    }

    Reseau * pReseau = pNetworkInstance->GetNetwork();

    boost::shared_lock<boost::shared_mutex> lock(pNetworkInstance->m_ShortestPathsComputeMutex);

    theApp.m_NetworksAccessMutex.unlock();


    std::vector<std::pair<double, std::vector<std::string> > > paths;
    int returnCode = pReseau->ComputeRoutesPublicTransport(originId, std::string(), std::string(), destinationId, dbTime, nbShortestPaths, paths);

    if (returnCode == 0)
    {
        rapidjson::Document response(rapidjson::kArrayType);
        rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

        for (size_t iPath = 0; iPath < paths.size(); iPath++)
        {
            rapidjson::Value pathJSON(rapidjson::kObjectType);
            pathJSON.AddMember("type_veh", "BUS", allocator);
            rapidjson::Value strValueOrigin(rapidjson::kStringType);
            strValueOrigin.SetString(originId, (rapidjson::SizeType)strlen(originId), allocator);
            pathJSON.AddMember("origine", strValueOrigin, allocator);
            rapidjson::Value strValueDestination(rapidjson::kStringType);
            strValueDestination.SetString(destinationId, (rapidjson::SizeType)strlen(destinationId), allocator);
            pathJSON.AddMember("destination", strValueDestination, allocator);

            const std::vector<std::string> & publicTransportPath = paths[iPath].second;
            std::string concatenatedPath;
            for (size_t iStep = 0; iStep < publicTransportPath.size(); iStep++)
            {
                if (iStep != 0)
                {
                    concatenatedPath.append(" ");
                }
                concatenatedPath.append(publicTransportPath.at(iStep));
            }
            rapidjson::Value strValue(rapidjson::kStringType);
            strValue.SetString(concatenatedPath.c_str(), (int)concatenatedPath.length(), allocator);
            pathJSON.AddMember("path", strValue, allocator);
            
            response.PushBack(pathJSON, allocator);
        }

        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        response.Accept(writer);
        std::string returnString = buffer.GetString();

        retValue = new char[returnString.length() + 1];
#ifdef WIN32
        strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
        strcpy(retValue, returnString.c_str());
#endif
    }

    return retValue;
}


/// <summary>
/// Fonction de calcul d'un itinéraire alternatif possible depuis un arrêt de bus mais sans emprunter la ligne courante.
/// </summary>
///<param name="originStopId">Nom de l'arrêt origine</param>
///<param name="currentLineId">Nom de la ligne sur laquelle le passager est en approche</param>
///<param name="nextLineId">Nom de la ligne sur laquelle le passager est pour l'instant sensé repartir de l'arrêt original</param>
///<param name="destinationId">Destination ID</param>
///<param name="dbTime">Simulation time</param>
///</param>
///<returns>JSON structure with desired result</returns>
SYMUBRUIT_EXPORT char * SYMUBRUIT_CDECL SymGetAlternatePathsPublicTransportFromStopJSON(char* originStopId, char* currentLineId, char* nextLineId, char* destinationId, double dbTime)
{
    char * retValue = NULL;

    theApp.m_NetworksAccessMutex.lock();

    CSymubruitApp::CSymubruitNetworkInstance * pNetworkInstance = theApp.GetNetworkInstance(DEFAULT_NETWORK_ID);

    if (!pNetworkInstance)
    {
        theApp.m_NetworksAccessMutex.unlock();
        return retValue;
    }

    Reseau * pReseau = pNetworkInstance->GetNetwork();

    boost::shared_lock<boost::shared_mutex> lock(pNetworkInstance->m_ShortestPathsComputeMutex);

    theApp.m_NetworksAccessMutex.unlock();

    // No public transport graph, no result
    if (!pReseau->IsWithPublicTransportGraph())
    {
        return retValue;
    }

    std::vector<std::pair<double, std::vector<std::string> > > paths;
    int returnCode = pReseau->ComputeRoutesPublicTransport(originStopId, currentLineId, nextLineId, destinationId, dbTime, 1, paths);

    if (returnCode == 0)
    {
        rapidjson::Document response(rapidjson::kArrayType);
        rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

        for (size_t iPath = 0; iPath < paths.size(); iPath++)
        {
            rapidjson::Value pathJSON(rapidjson::kObjectType);
            pathJSON.AddMember("type_veh", "BUS", allocator);
            rapidjson::Value strValueOrigin(rapidjson::kStringType);
            strValueOrigin.SetString(originStopId, (rapidjson::SizeType)strlen(originStopId), allocator);
            pathJSON.AddMember("origine", strValueOrigin, allocator);
            rapidjson::Value strValueDestination(rapidjson::kStringType);
            strValueDestination.SetString(destinationId, (rapidjson::SizeType)strlen(destinationId), allocator);
            pathJSON.AddMember("destination", strValueDestination, allocator);

            const std::vector<std::string> & publicTransportPath = paths[iPath].second;
            std::string concatenatedPath;
            for (size_t iStep = 0; iStep < publicTransportPath.size(); iStep++)
            {
                if (iStep != 0)
                {
                    concatenatedPath.append(" ");
                }
                concatenatedPath.append(publicTransportPath.at(iStep));
            }
            rapidjson::Value strValue(rapidjson::kStringType);
            strValue.SetString(concatenatedPath.c_str(), (int)concatenatedPath.length(), allocator);
            pathJSON.AddMember("path", strValue, allocator);

            response.PushBack(pathJSON, allocator);
        }

        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        response.Accept(writer);
        std::string returnString = buffer.GetString();

        retValue = new char[returnString.length() + 1];
#ifdef WIN32
        strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
        strcpy(retValue, returnString.c_str());
#endif
    }

    return retValue;
}

/// <summary>
/// Fonction de calcul des itinéraires possibles en mode mixte (avec partie VL jusqu'à un parking
/// puis une partie transports en commun) entre deux noeuds avec sortie au format JSON.
/// </summary>
///<param name="originId">Origin ID</param>
///<param name="destinationId">Destination ID</param>
///<param name="typeVeh">Vehicle type ID</param>
///<param name="dbTime">Simulation time</param>
///<param name="nbShortestPaths">Desired number of routes to compute</param>
///<returns>JSON structure with desired result</returns>
SYMUBRUIT_EXPORT char * SYMUBRUIT_CDECL SymGetAlternatePathsMixedJSON(char* originId, char* destinationId, char* typeVeh, double dbTime, int nbShortestPaths)
{
    char * retValue = NULL;

    theApp.m_NetworksAccessMutex.lock();

    CSymubruitApp::CSymubruitNetworkInstance * pNetworkInstance = theApp.GetNetworkInstance(DEFAULT_NETWORK_ID);

    if (!pNetworkInstance)
    {
        theApp.m_NetworksAccessMutex.unlock();
        return retValue;
    }

    Reseau * pReseau = pNetworkInstance->GetNetwork();

    boost::shared_lock<boost::shared_mutex> lock(pNetworkInstance->m_ShortestPathsComputeMutex);

    theApp.m_NetworksAccessMutex.unlock();

    std::map<Parking*, std::vector<std::vector<std::string> > > paths;
    int returnCode = pReseau->ComputeMixedRoutes(originId, destinationId, typeVeh, dbTime, nbShortestPaths, paths);

    if (returnCode == 0)
    {
        rapidjson::Document response(rapidjson::kArrayType);
        rapidjson::Document::AllocatorType& allocator = response.GetAllocator();

        std::map<Parking*, std::vector<std::vector<std::string> > >::const_iterator iterParking;
        for (iterParking = paths.begin(); iterParking != paths.end(); ++iterParking)
        {
            rapidjson::Value mixedJSON(rapidjson::kObjectType);

            rapidjson::Value parkingJSON(rapidjson::kObjectType);

            rapidjson::Value strValueId(rapidjson::kStringType);
            strValueId.SetString(iterParking->first->GetID().c_str(), (rapidjson::SizeType)strlen(iterParking->first->GetID().c_str()), allocator);
            parkingJSON.AddMember("id", strValueId, allocator);

            rapidjson::Value position(rapidjson::kObjectType);
            rapidjson::Value strValueRef(rapidjson::kStringType);
            strValueRef.SetString(iterParking->first->GetID().c_str(), (rapidjson::SizeType)strlen(iterParking->first->GetID().c_str()), allocator);
            position.AddMember("ref", strValueRef, allocator);
            position.AddMember("coordX", iterParking->first->getCoordonnees().dbX, allocator);
            position.AddMember("coordY", iterParking->first->getCoordonnees().dbY, allocator);
            parkingJSON.AddMember("position", position, allocator);

            mixedJSON.AddMember("parking", parkingJSON, allocator);

            rapidjson::Value publicTransportsJSON(rapidjson::kArrayType);

            for (size_t iPath = 0; iPath < iterParking->second.size(); iPath++)
            {
                rapidjson::Value publicTransportJSON(rapidjson::kObjectType);
                publicTransportJSON.AddMember("type_veh", "BUS", allocator);
                rapidjson::Value strValueOriginId(rapidjson::kStringType);
                strValueOriginId.SetString(iterParking->first->GetID().c_str(), (rapidjson::SizeType)strlen(iterParking->first->GetID().c_str()), allocator);
                publicTransportJSON.AddMember("origine", strValueOriginId, allocator);
                rapidjson::Value strValueDestination(rapidjson::kStringType);
                strValueDestination.SetString(destinationId, (rapidjson::SizeType)strlen(destinationId), allocator);
                publicTransportJSON.AddMember("destination", strValueDestination, allocator);

                const std::vector<std::string> & publicTransportPath = iterParking->second.at(iPath);
                std::string concatenatedPath;
                for (size_t iStep = 0; iStep < publicTransportPath.size(); iStep++)
                {
                    if (iStep != 0)
                    {
                        concatenatedPath.append(" ");
                    }
                    concatenatedPath.append(publicTransportPath.at(iStep));
                }
                rapidjson::Value strValue(rapidjson::kStringType);
                strValue.SetString(concatenatedPath.c_str(), (int)concatenatedPath.length(), allocator);
                publicTransportJSON.AddMember("path", strValue, allocator);

                publicTransportsJSON.PushBack(publicTransportJSON, allocator);
            }

            mixedJSON.AddMember("publicTransports", publicTransportsJSON, allocator);

            response.PushBack(mixedJSON, allocator);
        }

        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        response.Accept(writer);
        std::string returnString = buffer.GetString();

        retValue = new char[returnString.length() + 1];
#ifdef WIN32
        strcpy_s(retValue, returnString.length() + 1, returnString.c_str());
#else
        strcpy(retValue, returnString.c_str());
#endif
    }

    return retValue;
}


SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymWriteTempusNetwork()
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

    // Pas de réseau chargé, sortie sur erreur
    if(!pReseau)
    {           
        std::cerr << "No loaded network !" << std::endl;
        return false;
    }
	
	return pReseau->WriteTempusFiles();   
}

#ifdef USE_SYMUCORE

SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymLoadNetwork(SymuCore::IUserHandler * pUserHandler, std::string sTmpXmlDataFile, int marginalsDeltaN, bool bUseMarginalsMeanMethod,
	bool bUseMarginalsMedianMethod, int nbVehiclesForTravelTimes, int nbPeriodsForTravelTimes, bool bUseSpatialTTMethod, bool bUseClassicSpatialTTMethod, 
	bool bEstimateTrafficLightsWaitTime, double dbMinSpeedForTravelTimes, double dbMaxMarginalsValue, bool bUseTravelTimesAsMarginalsInAreas,
	double dbConcentrationRatioForFreeFlowCriterion, int nNbClassRobustTTMethod, int nMaxNbPointsPerClassRobustTTMethod, int nMinNbPointsStatisticalFilterRobustTTMethod,
	bool bUseLastBusIfAnyForTravelTimes, bool bUseEmptyTravelTimesForBusLines, double dbMeanBusExitTime,
	int nbStopsToConnectOutsideOfSubAreas, double dbMaxInitialWalkRadius, double dbMaxIntermediateWalkRadius, bool bComputeAllCosts, double dbMinLengthForMarginals,
	std::string sRobustTravelTimesFile, std::string sRobustTravelSpeedsFile, bool bPollutantEmissionComputation,
	boost::posix_time::ptime & startTime, boost::posix_time::ptime & endTime, boost::posix_time::time_duration & timeStep, bool & bHasTrajectoriesOutput)
{
	int networkId = SymLoadNetwork(sTmpXmlDataFile, true, false, false);
	bool bOk = networkId != 0;

	if (bOk)
	{
		Reseau * pNetwork = theApp.GetNetwork(networkId);

		pNetwork->SetSymuMasterUserHandler(pUserHandler);
		pNetwork->SetMarginalsDeltaN(marginalsDeltaN);
		pNetwork->SetUseMarginalsMeanMethod(bUseMarginalsMeanMethod);
		pNetwork->SetUseMarginalsMedianMethod(bUseMarginalsMedianMethod);
		pNetwork->SetNbVehiclesForTravelTimes(nbVehiclesForTravelTimes);
		pNetwork->SetNbPeriodsForTravelTimes(nbPeriodsForTravelTimes);
		pNetwork->SetUseSpatialTTMethod(bUseSpatialTTMethod);
		pNetwork->SetUseClassicSpatialTTMethod(bUseClassicSpatialTTMethod);
		pNetwork->SetEstimateTrafficLightsWaitTime(bEstimateTrafficLightsWaitTime);
		pNetwork->SetMinSpeedForTravelTime(dbMinSpeedForTravelTimes);
		pNetwork->SetMaxMarginalsValue(dbMaxMarginalsValue);
		pNetwork->SetUseTravelTimesAsMarginalsInAreas(bUseTravelTimesAsMarginalsInAreas);
		pNetwork->SetConcentrationRatioForFreeFlowCriterion(dbConcentrationRatioForFreeFlowCriterion);
        pNetwork->SetUseLastBusIfAnyForTravelTimes(bUseLastBusIfAnyForTravelTimes);
        pNetwork->SetUseEmptyTravelTimesForBusLines(bUseEmptyTravelTimesForBusLines);
        pNetwork->SetMeanBusExitTime(dbMeanBusExitTime);
        pNetwork->SetNbStopsToConnectOutsideOfSubAreas(nbStopsToConnectOutsideOfSubAreas);
        pNetwork->SetMaxInitialWalkRadius(dbMaxInitialWalkRadius);
        pNetwork->SetMaxIntermediateWalkRadius(dbMaxIntermediateWalkRadius);
        pNetwork->SetComputeAllCosts(bComputeAllCosts);  
        pNetwork->SetMinLengthForMarginals(dbMinLengthForMarginals);
		pNetwork->SetNbClassRobustTTMethod(nNbClassRobustTTMethod);
		pNetwork->SetMaxNbPointsPerClassRobustTTMethod(nMaxNbPointsPerClassRobustTTMethod);
		pNetwork->SetMinNbPointsStatisticalFilterRobustTTMethod(nMinNbPointsStatisticalFilterRobustTTMethod);
		pNetwork->SetRobustTravelTimesFile(sRobustTravelTimesFile);
		pNetwork->SetRobustTravelSpeedsFile(sRobustTravelSpeedsFile);
		pNetwork->SetPollutantEmissionComputation(bPollutantEmissionComputation);

        startTime = pNetwork->GetSimulationStartTime();
        endTime = startTime + boost::posix_time::microseconds((int64_t)boost::math::round(pNetwork->GetDureeSimu()*1000000.0));
        timeStep = boost::posix_time::microseconds((int64_t)boost::math::round(pNetwork->GetTimeStep()*1000000.0));

        bHasTrajectoriesOutput = pNetwork->IsSortieTraj();

        bOk = pNetwork->InitSimuTrafic();
        if (!bOk)
        {
            networkId = 0;
        }
    }

    return networkId;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymDisableTrajectoriesOutput(int networkId, bool bDisableTrajectoriesOutput)
{
    Reseau* pReseau = theApp.GetNetwork(networkId);

    assert(pReseau); // Pas de réseau chargé

    pReseau->SetDisableTrajectoriesOutput(bDisableTrajectoriesOutput);

    return true;
}

SYMUBRUIT_EXPORT SymuCore::Origin * SYMUBRUIT_CDECL SymGetParentOrigin(int networkId, SymuCore::Origin * pChildOrigin, SymuCore::MultiLayersGraph * pGraph)
{
    Reseau* pReseau = theApp.GetNetwork(networkId);

    assert(pReseau); // Pas de réseau chargé

    const std::string & subOriginName = pChildOrigin->getStrNodeName();
    for (size_t iZone = 0; iZone < pReseau->Liste_zones.size(); iZone++)
    {
        const std::vector<CPlaque*> & plaques = pReseau->Liste_zones[iZone]->GetLstPlaques();
        for (size_t iPlaque = 0; iPlaque < plaques.size(); iPlaque++)
        {
            if (plaques[iPlaque]->GetID() == subOriginName)
            {
                // Récupération on création de l'origine parente dans le graphe : c'est notre résultat
                return pGraph->GetOrCreateOrigin(pReseau->Liste_zones[iZone]->GetID());
            }
        }
    }

    return NULL;
}

SYMUBRUIT_EXPORT SymuCore::Destination * SYMUBRUIT_CDECL SymGetParentDestination(int networkId, SymuCore::Destination * pChildDestination, SymuCore::MultiLayersGraph * pGraph)
{
    Reseau* pReseau = theApp.GetNetwork(networkId);

    assert(pReseau); // Pas de réseau chargé

    const std::string & subDestinationName = pChildDestination->getStrNodeName();
    for (size_t iZone = 0; iZone < pReseau->Liste_zones.size(); iZone++)
    {
        const std::vector<CPlaque*> & plaques = pReseau->Liste_zones[iZone]->GetLstPlaques();
        for (size_t iPlaque = 0; iPlaque < plaques.size(); iPlaque++)
        {
            if (plaques[iPlaque]->GetID() == subDestinationName)
            {
                // Récupération on création de la destination parente dans le graphe : c'est notre résultat
                return pGraph->GetOrCreateDestination(pReseau->Liste_zones[iZone]->GetID());
            }
        }
    }

    return NULL;
}


SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymBuildGraph(int networkId, SymuCore::MultiLayersGraph * pGraph, bool bIsPrimaryGraph)
{
    Reseau* pReseau = theApp.GetNetwork(networkId);

    assert(pReseau); // Pas de réseau chargé

    return pReseau->BuildGraph(pGraph, bIsPrimaryGraph);
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymFillPopulations(int networkId, SymuCore::MultiLayersGraph * pGraph, SymuCore::Populations & populations,
    const boost::posix_time::ptime & startSimulationTime, const boost::posix_time::ptime & endSimulationTime, bool bIgnoreSubAreas,
    std::vector<SymuCore::Trip> & lstTrips)
{
    Reseau* pReseau = theApp.GetNetwork(networkId);

    assert(pReseau); // Pas de réseau chargé

    return pReseau->FillPopulations(pGraph, populations, startSimulationTime, endSimulationTime, bIgnoreSubAreas, lstTrips);
}

// Met à jour les mesures des temps de parcours des liens et noeuds
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymComputeCosts(int networkId, const boost::posix_time::ptime & startTime, const boost::posix_time::ptime & endTime,
    const std::vector<SymuCore::MacroType*> & macroTypes, const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions)
{
    Reseau * pNetwork = theApp.GetNetwork(networkId);

    assert(pNetwork); // Pas de réseau chargé

    return pNetwork->ComputeCosts(startTime, endTime, macroTypes, mapCostFunctions);
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymTakeSimulationSnapshot(int networkId)
{
    Reseau * pNetwork = theApp.GetNetwork(networkId);

    assert(pNetwork != NULL && pNetwork->GetAffectationModule() != NULL);

    // Initialisation de la simu de trafic si pas déjà fait (sinon, on crée une sauvegarde de l'état d'une 
    // simulation non initialisée qui ne le sera plus après un éventuel restore)
    if (!pNetwork->IsInitSimuTrafic())
    {
        if (!pNetwork->InitSimuTrafic())
        {
            return false;
        }
    }
    pNetwork->GetAffectationModule()->TakeSimulationSnapshot(pNetwork);
    return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymSimulationRollback(int networkId, size_t iSnapshotIdx)
{
    Reseau * pNetwork = theApp.GetNetwork(networkId);

    assert(pNetwork != NULL && pNetwork->GetAffectationModule() != NULL);

    pNetwork->GetAffectationModule()->SimulationRollback(pNetwork, iSnapshotIdx);
    return true;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymSimulationCommit(int networkId)
{
    Reseau * pNetwork = theApp.GetNetwork(networkId);

    assert(pNetwork != NULL && pNetwork->GetAffectationModule() != NULL);

    pNetwork->GetAffectationModule()->SimulationCommit(pNetwork);
    return true;
}

SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymCreatePublicTransportUser(
    int networkId,
    const std::string & startStop,
    const std::string & endStop,
    const std::string & lineName,
    double dbt,
    int externalUserID)
{
    Reseau * pNetwork = theApp.GetNetwork(networkId);

    if (!pNetwork)
    {
        return -1;
    }

    if (!pNetwork->IsSymuMasterMode())
    {
        return -2;
    }

    return pNetwork->CreatePublicTransportUser(startStop, endStop, lineName, dbt, externalUserID);
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymFillShortestPathParameters(bool bFillKParameters, bool bUseCommonalityFilter, double& dbAssignementAlpha, double& dbAssignementBeta, double& dbAssignementGamma,
                                                                    std::vector<double>& dbCommonalityFactorParameters, double& dbWardropTolerance,
                                                                    std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, SymuCore::ListTimeFrame<double> > > >& KByOD)
{
    Reseau* pReseau = theApp.GetNetwork(1);

    assert(pReseau); // Pas de réseau chargé 

    return pReseau->FillShortestPathParameters(bFillKParameters, bUseCommonalityFilter, dbAssignementAlpha, dbAssignementBeta, dbAssignementGamma, dbCommonalityFactorParameters, dbWardropTolerance, KByOD);
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymFillLogitParameters(std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, SymuCore::ListTimeFrame<double> > > >& LogitByOD)
{
    Reseau* pReseau = theApp.GetNetwork(1);

    assert(pReseau); // Pas de réseau chargé

    return pReseau->FillLogitParameters(LogitByOD);
}

// récupère les informations d'offre aux entrées demandées (pour gestion des interfaces d'hybridation avec d'autres simulateurs)
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymGetOffers(int networkID, const std::vector<std::string> & inputNames, std::vector<double> & offerValuesVect)
{
    Reseau* pReseau = theApp.GetNetwork(networkID);

    assert(pReseau); // Pas de réseau chargé

    return pReseau->GetOffers(inputNames, offerValuesVect);
}

// met à jour la restriction de capacité d'une sortie pour un identifiant de route aval donné (hybridation avec SimuRes)
SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymSetCapacities(int networkID, const std::string & exitName, const std::string & downstreamRouteName, double dbCapacityValue)
{
    Reseau* pReseau = theApp.GetNetwork(networkID);

    assert(pReseau); // Pas de réseau chargé

    return pReseau->SetCapacities(exitName, downstreamRouteName, dbCapacityValue);
}

#endif // USE_SYMUCORE

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymForbidLink(char* linkName)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

	if (!pReseau)
		return false;

	Tuyau * pLink = pReseau->GetLinkFromLabel(linkName);

	if (!pLink)
		return false;

	std::vector<Tuyau*> lstLinks(1, pLink);
	return pReseau->ForbidLinks(lstLinks);
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymForbidLinks(char* linkNamesJSON)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

	if (!pReseau)
		return false;

	rapidjson::Document document;
	document.Parse(linkNamesJSON);

	if (!document.IsArray())
		return false;

	std::vector<Tuyau*> lstLinks;
	for (rapidjson::SizeType i = 0; i < document.Size(); i++)
	{
		std::string linkName = document[i].GetString();
		Tuyau * pLink = pReseau->GetLinkFromLabel(linkName);
		if (!pLink)
			return false;
		lstLinks.push_back(pLink);
	}

	return pReseau->ForbidLinks(lstLinks);
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymForbidLinks(int networkId, const std::vector<std::string> & linkNames)
{
	Reseau* pReseau = theApp.GetNetwork(networkId);

	if (!pReseau)
	{
		std::cout << networkId << " Unknown network" << std::endl;
		return false;
	}
		

	std::vector<Tuyau*> lstLinks;
	for (size_t iLinkName = 0; iLinkName < linkNames.size(); iLinkName++)
	{
		Tuyau * pLink = pReseau->GetLinkFromLabel(linkNames[iLinkName]);
		if (pLink)
		{
			lstLinks.push_back(pLink);
		}
	}

	return pReseau->ForbidLinks(lstLinks);
}

SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymAddControlZone(int networkId, double dbAcceptanceRate, double dbDistanceLimit, const std::vector<std::string> & linkNames)
{
	Reseau* pNetwork = theApp.GetNetwork(networkId);

	if (!pNetwork)
	{
		std::cout << networkId << " Unknown network" << std::endl;
		return false;
	}

	std::vector<Tuyau*> lstLinks;
	for (size_t iLinkName = 0; iLinkName < linkNames.size(); iLinkName++)
	{
		Tuyau * pLink = pNetwork->GetLinkFromLabel(linkNames[iLinkName]);
		if (pLink)
		{
			lstLinks.push_back(pLink);
		}
	}

	return pNetwork->AddControlZone(dbAcceptanceRate, dbDistanceLimit, lstLinks);
}

SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymRemoveControlZone(int networkId, int nControlZoneID)
{
	Reseau* pNetwork = theApp.GetNetwork(networkId);

	if(pNetwork)
		return pNetwork->RemoveControlZone(nControlZoneID);

	return -1;
}

SYMUBRUIT_EXPORT int SYMUBRUIT_CDECL SymModifyControlZone(int networkId, int nControlZoneID, double dbAcceptanceRate)
{
	Reseau* pNetwork = theApp.GetNetwork(networkId);

	if (pNetwork)
		return pNetwork->ModifyControlZone(nControlZoneID, dbAcceptanceRate);

	return -1;
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymForbidZone(char* areaJSON)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

	if (!pReseau)
		return false;

	std::vector<Tuyau*> intersectedLinks;
	bool bOk = pReseau->GetLinksFromGeoJSONPolygon(areaJSON, intersectedLinks);

	if (!bOk)
		return false;

	return pReseau->ForbidLinks(intersectedLinks);
}


SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymAllowLink(char* linkName)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

	if (!pReseau)
		return false;

	Tuyau * pLink = pReseau->GetLinkFromLabel(linkName);

	if (!pLink)
		return false;

	std::vector<Tuyau*> lstLinks(1, pLink);
	return pReseau->AllowLinks(lstLinks);
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymAllowLinks(char* linkNamesJSON)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

	if (!pReseau)
		return false;

	rapidjson::Document document;
	document.Parse(linkNamesJSON);

	if (!document.IsArray())
		return false;

	std::vector<Tuyau*> lstLinks;
	for (rapidjson::SizeType i = 0; i < document.Size(); i++)
	{
		std::string linkName = document[i].GetString();
		Tuyau * pLink = pReseau->GetLinkFromLabel(linkName);
		if (!pLink)
			return false;
		lstLinks.push_back(pLink);
	}

	return pReseau->AllowLinks(lstLinks);
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymAllowZone(char* areaJSON)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

	if (!pReseau)
		return false;

	std::vector<Tuyau*> intersectedLinks;
	bool bOk = pReseau->GetLinksFromGeoJSONPolygon(areaJSON, intersectedLinks);

	if (!bOk)
		return false;

	return pReseau->AllowLinks(intersectedLinks);
}

SYMUBRUIT_EXPORT bool SYMUBRUIT_CDECL SymSetODPaths(char* pathsDefJSON)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

	if (!pReseau)
		return false;

	return pReseau->SetODPaths(pathsDefJSON);
}

// Return last computed value of total travel time of a MFD sensor
SYMUBRUIT_EXPORT double SYMUBRUIT_CDECL SymGetTotalTravelTime(std::string sMFDSensorID)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

	if (!pReseau)
		return false;

	return pReseau->GetTotalTravelTime(sMFDSensorID);
}

// Return last computed value of total travel distance of a MFD sensor
SYMUBRUIT_EXPORT double SYMUBRUIT_CDECL SymGetTotalTravelDistance(std::string sMFDSensorID)
{
	Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

	if (!pReseau)
		return false;

	return pReseau->GetTotalTravelDistance(sMFDSensorID);
}

//-------------------------------------------------------------------------------------
// Fonction exportée avec déclaration en C (chargement et exécution de la simulation)
//-------------------------------------------------------------------------------------

#define DLL_EXPORT
#include "SymExpC.h"

extern "C"
{
	DECLDIR int SymRunEx(char* sFile)
	{
        XMLUtil xmlUtil;
		std::vector<ScenarioElement> scenarii = xmlUtil.GetScenarioElements(sFile);

		// Pour tous les scénarios définis dans le fichier d'entrée, chargement + exécution
		for (size_t iScenario = 0; iScenario < scenarii.size(); iScenario++)
		{
			bool bOk = SymLoadNetwork(std::string(sFile), scenarii[iScenario].id);
			if (bOk)
			{
				SymRun();
			}

			SymQuit();

			if (!bOk)
			{
				return 1;
			}
		}

		return 0;
	}

	DECLDIR int SymLoadNetworkEx(char* sFile)
	{
		return SymLoadNetwork(std::string(sFile));
	}

	DECLDIR bool SymRunNextStepEx(char *sXmlFluxInstant, bool bTrace, bool *bNEnd)
	{
		return SymRunNextStep(sXmlFluxInstant, bTrace, *bNEnd);
	}

	DECLDIR char * SymRunNextStepJSONEx(bool *bNEnd)
	{
		std::string sJsonFluxInstant;

		if (!_SymRunNextStepJSON(sJsonFluxInstant, *bNEnd))
		{
			return NULL;
		}

		char* retValue = new char[sJsonFluxInstant.length() + 1];
#ifdef WIN32
		strcpy_s(retValue, sJsonFluxInstant.length() + 1, sJsonFluxInstant.c_str());
#else
		strcpy(retValue, sJsonFluxInstant.c_str());
#endif

		return retValue;
	}

	DECLDIR double SymDriveVehicleFromAbsolutePositionEx
	(
		int				nID,
		double			dbX,
		double			dbY,
		double			dbZ
	)
	{
		Reseau* pReseau = theApp.GetNetwork(DEFAULT_NETWORK_ID);

		// Pas de réseau chargé, sortie sur erreur
		if (!pReseau)
		{
			return -1;
		}

		Tuyau *pT=NULL;
		int nLane;
		double dbRelativePosition;

		pT = AbsoluteToRelativePosition(pReseau, dbX, dbY, dbZ, nLane, dbRelativePosition);

		if (pT)
			return pReseau->DriveVehicle(nID, pT->GetLabel(), nLane, dbRelativePosition, true);
		else
			return -1.00;
	}

	DECLDIR double SymDriveVehicleEx
	(
		int				nID,
		char			*sTroncon,
		int				nVoie,
		double			dbPos,
		bool			bForce
	)
	{
		return SymDriveVehicle(nID, sTroncon, nVoie, dbPos, bForce);
	}

	DECLDIR int SymCreateVehicleEx
	(
		char* sType,
		char* sEntree,
		char* sSortie,
		int			nVoie,
		double		dbt
	)
	{
		return SymCreateVehicle(sType, sEntree, sSortie, nVoie, dbt);
	}

	DECLDIR int SymCreateVehicleWithRouteEx
	(
		char*       originId,
		char*       destinationId,
		char*       typeVeh,
		int			nVoie,
		double		dbt,
		char*		links
	)
	{
		std::string slinks = std::string(links);
		std::deque<string> vctlinkNames;

		vctlinkNames = SystemUtil::split(links, ' ');


		return SymCreateVehicle(originId, destinationId, typeVeh, nVoie, dbt, vctlinkNames);
	}

	DECLDIR char* SymGetRemainingPathInfoJSONEx(int vehicleId)
	{
		return SymGetRemainingPathInfoJSON(vehicleId);
	}

	DECLDIR int SymComputeRoutesEx
	(
		char* originId,
		char* destinationId,
		char* typeVeh,
		double dbTime,
		int nbShortestPaths,
		char ***pathLinks[],
		double* pathCosts[]
	)
	{
		return SymComputeRoutes(originId, destinationId, typeVeh, dbTime, nbShortestPaths, pathLinks, pathCosts);
	}

	DECLDIR bool SymForbidLinksEx
	(
		int networkId, 
		char *linkNames
	)
	{
		std::string links = std::string(linkNames);
		std::vector<string> vctlinkNames;

		// Transform json data to vector of string
		// Remove all double-quote characters
		links.erase(
			remove(links.begin(), links.end(), '\"'),
			links.end()
		);

		// Remove all [ characters
		links.erase(
			remove(links.begin(), links.end(), '['),
			links.end()
		);

		// Remove all ] characters
		links.erase(
			remove(links.begin(), links.end(), ']'),
			links.end()
		);
		
		vctlinkNames = SystemUtil::split2vct(links, ',');

		for(int i=0;i<vctlinkNames.size();i++)
			std::cout << vctlinkNames[i] << std::endl;

		int ok=SymForbidLinks(networkId, vctlinkNames);
		
		return ok;
	}

	DECLDIR int SymAddControlZoneEx(int networkId, double dbAcceptanceRate, double dbDistanceLimit, char* links)
	{
		std::vector<std::string> linkNames;
		std::string slinks = std::string(links);
		
		linkNames = SystemUtil::split2vct(slinks, ' ');

		return SymAddControlZone(networkId, dbAcceptanceRate, dbDistanceLimit, linkNames);
	}

	DECLDIR int SymRemoveControlZoneEx(int networkId, int nControlZoneID)
	{
		return SymRemoveControlZone(networkId, nControlZoneID);
	}

	DECLDIR int SymModifyControlZoneEx(int networkId, int nControlZoneID, double dbAcceptanceRate)
	{
		return SymModifyControlZone(networkId, nControlZoneID, dbAcceptanceRate);
	}

	DECLDIR double SymGetTotalTravelTimeEx(char* MFDSensorID)
	{
		std::string sMFDSensorID = std::string(MFDSensorID);
		return SymGetTotalTravelTime(sMFDSensorID);
	}

	DECLDIR double SymGetTotalTravelDistanceEx(char* MFDSensorID)
	{
		std::string sMFDSensorID = std::string(MFDSensorID);
		return SymGetTotalTravelDistance(sMFDSensorID);
	}
}
