#include "stdafx.h"
#include "SimulationSnapshot.h"

#include "ControleurDeFeux.h"
#include "reseau.h"
#include "RandManager.h"
#include "tuyau.h"
#include "TuyauMeso.h"
#include "voie.h"
#include "convergent.h"
#include "CarrefourAFeuxEx.h"
#include "Plaque.h"
#include "Logger.h"
#include "TravelTimesOutputManager.h"
#include "TraceDocTrafic.h"
#include "carFollowing/AbstractCarFollowing.h"
#include "carFollowing/CarFollowingContext.h"
#include "usage/AbstractFleet.h"
#include "sensors/EdieSensor.h"
#include "sensors/MFDSensor.h"
#include "sensors/SensorsManager.h"
#include "regulation/RegulationModule.h"
#include "regulation/RegulationBrique.h"
#include "regulation/RegulationCapteur.h"
#include "regulation/RegulationCondition.h"
#include "regulation/RegulationAction.h"
#include "Xerces/XMLUtil.h"
#include "Xerces/DOMLSSerializerSymu.hpp"
#include "CSVOutputWriter.h"
#include "SystemUtil.h"


#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/list.hpp>

#ifdef USE_SYMUCOM
#include "ITS/Stations/DerivedClass/Simulator.h"
#endif // USE_SYMUCOM

using namespace std;

SimulationSnapshot::SimulationSnapshot()
{
    m_nInstSvg = 0;

    m_nNbVehCumSvg = 0;
    m_nRandCountSvg = 0;
    m_nLastIdVehSvg = 0;

    m_pTravelTimesOutputSnapshot = NULL;

#ifdef USE_SYMUCOM
    m_pITSSnapshot = NULL;
#endif // USE_SYMUCOM

    m_XmlDocTraficTmp = NULL;

    m_pTrajsOFSTMP = NULL;
    m_pLinkChangesOFSTMP = NULL;
    m_pSensorsOFSTMP = NULL;

    m_TravelTimesTmp = NULL;

#ifdef USE_SYMUCOM
    m_pSymuComWriter = NULL;
#endif // USE_SYMUCOM
}

SimulationSnapshot::~SimulationSnapshot()
{
    for (std::map<AbstractSensor*, AbstractSensorSnapshot*>::iterator iter = m_mapCapteur.begin(); iter != m_mapCapteur.end(); ++iter)
    {
        delete iter->second;
    }

    for (std::map<AbstractFleet*, AbstractFleetSnapshot*>::iterator iter = m_mapFleet.begin(); iter != m_mapFleet.end(); ++iter)
    {
        delete iter->second;
    }

    if (m_pTravelTimesOutputSnapshot)
    {
        delete m_pTravelTimesOutputSnapshot;
    }

#ifdef USE_SYMUCOM
    if (m_pITSSnapshot)
    {
        delete m_pITSSnapshot;
    }
#endif // USE_SYMUCOM

    DiscardTempFiles();
}

void SimulationSnapshot::SwitchToTempFiles(Reseau * pReseau, size_t snapshotIdx)
{
    bool bEnableXML = (pReseau->GetSymMode() == Reseau::SYM_MODE_FULL) || (pReseau->GetSymMode() == Reseau::SYM_MODE_STEP_XML) || pReseau->IsXmlOutput();
    std::deque< TimeVariation<TraceDocTrafic> >::iterator it;
    // OTK - rmq. vu ce code, l'affectation dynamique convergente ne doit pas marcher si on a plusieurs plages temporelles... on aura au mieux une fuite mémoire. A reprendre ?
    for (it = pReseau->m_xmlDocTrafics.begin(); it != pReseau->m_xmlDocTrafics.end(); it++)
    {
        std::string ssFile = pReseau->GetPrefixOutputFiles();
        if (snapshotIdx == 0)
        {
            ssFile += "_trafTMP" + pReseau->GetSuffixOutputFiles() + ".xml";
        }
        else
        {
            ssFile += "_trafTMP" + pReseau->GetSuffixOutputFiles() + "_" + SystemUtil::ToString(snapshotIdx) + ".xml";
        }
        m_XmlDocTraficTmp = new TraceDocTrafic(pReseau, bEnableXML, pReseau->IsXmlOutput(), pReseau->IsDebug(), pReseau->IsTraceStocks(), pReseau->IsSortieLight(), pReseau->IsSortieRegime(), bEnableXML ? pReseau->GetXMLUtil()->CreateXMLDocument(XS("OUT")) : NULL,
            pReseau->GetSymMode() == Reseau::SYM_MODE_STEP_EVE, pReseau->IsGMLOutput(), pReseau->GetSymMode() == Reseau::SYM_MODE_STEP_JSON);

        SDateTime dtDeb;
        if (pReseau->GetDateSimulation().GetYear() != 0)
        {
            dtDeb = pReseau->GetDateSimulation();
        }
        else
        {
            dtDeb = SDateTime::Now();
        }
        SDateTime dtFin = dtDeb;
        dtDeb.m_hour = pReseau->GetSimuStartTime().m_hour;
        dtDeb.m_minute = pReseau->GetSimuStartTime().m_minute;
        dtDeb.m_second = pReseau->GetSimuStartTime().m_second;
        dtFin.m_hour = pReseau->GetSimuEndTime().m_hour;
        dtFin.m_minute = pReseau->GetSimuEndTime().m_minute;
        dtFin.m_second = pReseau->GetSimuEndTime().m_second;
        m_XmlDocTraficTmp->Init(ssFile, "", dtDeb, dtFin, 0, pReseau->m_XMLDocData, 0, pReseau->m_SimulationID, pReseau->m_TraficID, pReseau->m_ReseauID, pReseau->GetBoundingRect(), (*it).m_pData.get(), NULL, !pReseau->Liste_ControleursDeFeux.empty(), pReseau->GetGestionsCapteur());
    }
    m_XmlDocTraficTmp->PostInit();
    if (pReseau->DoSortieTraj())
    {
        //rmq. il ne faut le faire que dans le CP du doc trafic XML standard
        m_XmlDocTraficTmp->AddInstantXML(pReseau->m_dbInstSimu, pReseau->GetTimeStep(), (int)pReseau->m_LstVehicles.size());
    }

    // on passe en mode "écriture dans fichiers temporaires" pour les sorties CSV
    if (pReseau->IsCSVOutput())
    {
        pReseau->m_CSVOutputWriter->doUseTempFiles(this, snapshotIdx);
    }

    // Idem pour les temps de parcours JSON
    if (pReseau->GetTravelTimesOutputManager())
    {
        pReseau->GetTravelTimesOutputManager()->doUseTempFiles(this);
    }

    // Idem pour le XML SymuCom
#ifdef USE_SYMUCOM
    if (pReseau->m_pSymucomSimulator)
    {
        pReseau->m_pSymucomSimulator->doUseTempFiles(this, snapshotIdx);
    }
#endif // USE_SYMUCOM
}

void SimulationSnapshot::ValidateTempFiles(Reseau * pReseau)
{
    m_XmlDocTraficTmp->Terminate();

    //Recopie des instants de la période d'affectation dans le fichier trafic
    std::deque< TimeVariation<TraceDocTrafic> >::iterator it;
    for (it = pReseau->m_xmlDocTrafics.begin(); it != pReseau->m_xmlDocTrafics.end(); it++)
    {
        (it)->m_pData->Add(this->m_XmlDocTraficTmp);
    }

    if (pReseau->IsCSVOutput())
    {
        pReseau->m_CSVOutputWriter->converge(this);
    }

    if (pReseau->GetTravelTimesOutputManager())
    {
        pReseau->GetTravelTimesOutputManager()->converge(this);
    }

    // Idem pour le XML SymuCom
#ifdef USE_SYMUCOM
    if (pReseau->m_pSymucomSimulator)
    {
        pReseau->m_pSymucomSimulator->converge(this);
    }
#endif // USE_SYMUCOM
}

void SimulationSnapshot::DiscardTempFiles()
{
    std::cout << "Discard m_XmlDocTraficTmp" << std::endl;
    if (m_XmlDocTraficTmp)
    {
        m_XmlDocTraficTmp->Remove();
        delete(m_XmlDocTraficTmp);
        m_XmlDocTraficTmp = NULL;
    }

    std::cout << "Discard m_pTrajsOFSTMP" << std::endl;
    if (m_pTrajsOFSTMP)
    {
        if (m_pTrajsOFSTMP->is_open())
        {
            m_pTrajsOFSTMP->close();
        }
        remove(m_strTrajsOFSTMP.c_str());
        delete m_pTrajsOFSTMP;
        m_pTrajsOFSTMP = NULL;
    }

std::cout << "Discard m_pLinkChangesOFSTMP" << std::endl;
    if (m_pLinkChangesOFSTMP)
    {
        if (m_pLinkChangesOFSTMP->is_open())
        {
            m_pLinkChangesOFSTMP->close();
        }
        remove(m_strLinkChangesOFSTMP.c_str());
        delete m_pLinkChangesOFSTMP;
        m_pLinkChangesOFSTMP = NULL;
    }

std::cout << "Discard m_pSensorsOFSTMP" << std::endl;
    if (m_pSensorsOFSTMP)
    {
        if (m_pSensorsOFSTMP->is_open())
        {
            m_pSensorsOFSTMP->close();
        }
        remove(m_strSensorsOFSTMP.c_str());
        delete m_pSensorsOFSTMP;
        m_pSensorsOFSTMP = NULL;
    }

std::cout << "Discard m_TravelTimesTmp" << std::endl;
    if (m_TravelTimesTmp)
    {
        delete m_TravelTimesTmp;
        m_TravelTimesTmp = NULL;
    }

    // Idem pour le XML SymuCom
std::cout << "Discard m_pSymuComWriter" << std::endl;
#ifdef USE_SYMUCOM
    std::cout << "Discard m_pSymuComWriter 2" << std::endl;
    if (m_pSymuComWriter)
    {
        std::string symuComFile = m_pSymuComWriter->getTargetFileName();
        m_pSymuComWriter->close();
        delete m_pSymuComWriter;
        m_pSymuComWriter = NULL;
        remove(symuComFile.c_str());
    }
#endif // USE_SYMUCOM
}

void SimulationSnapshot::Backup(Reseau *pReseau)
{
	std::vector<boost::shared_ptr<Vehicule>>::iterator itVeh;	
    std::deque<boost::shared_ptr<Vehicule>>::const_iterator itVehDeq;	

	// Sauvegarde de l'instant simulé
	m_dbInstSvg = pReseau->m_dbInstSimu;
	m_nInstSvg = pReseau->m_nInstSim;
    m_nRandCountSvg = pReseau->GetRandManager()->getCount();
    m_nNbVehCumSvg = pReseau->m_nNbVehCum;

	m_nLastIdVehSvg = pReseau->m_nLastVehicleID;
	
	// Les véhicules...
	for( itVeh = pReseau->m_LstVehicles.begin(); itVeh != pReseau->m_LstVehicles.end(); ++itVeh)
	{
		boost::shared_ptr<Vehicule> pV(new Vehicule());
        (*itVeh)->CopyTo(pV);
		m_lstVehSvg.push_back(pV);
	}

	// Les flottes de véhicules
    for(size_t iFleet = 0; iFleet < pReseau->m_LstFleets.size(); iFleet++)
    {
        AbstractFleet * pFleet = pReseau->m_LstFleets[iFleet];
        AbstractFleetSnapshot * pFleetBackup = pFleet->TakeSnapshot();
        this->m_mapFleet[pFleet] = pFleetBackup;
    }

    // Les contrôleurs de feux (pour les LGP)
    std::deque<ControleurDeFeux*>::iterator itCDF;
    for(itCDF = pReseau->Liste_ControleursDeFeux.begin(); itCDF != pReseau->Liste_ControleursDeFeux.end(); ++itCDF)
	{
        stVarSimCDF sVarSim;

        sVarSim.pPDFActif = (*itCDF)->GetPDFActif();
        sVarSim.cModeFct = (*itCDF)->GetModeFct();
        for(size_t iVGP = 0; iVGP < (*itCDF)->GetLstVGPs().size(); iVGP++)
        {
            sVarSim.dqBus.push_back(std::make_pair((*itCDF)->GetLstVGPs()[iVGP].first->GetID(), (*itCDF)->GetLstVGPs()[iVGP].second));
        }
        if((*itCDF)->GetPDFEx())
        {
            sVarSim.pPDFEx.reset(new PlanDeFeux);
            (*itCDF)->GetPDFEx()->CopyTo(sVarSim.pPDFEx.get());
        }
        sVarSim.dbDebPDFEx = (*itCDF)->GetDebutPDFEx();
        sVarSim.dbFinPDFEx = (*itCDF)->GetFinPDFEx();
        sVarSim.pSeqPrePriorite = (*itCDF)->GetSeqPrePriorite();
        sVarSim.pSeqRetour1 = (*itCDF)->GetSeqRetour1();
        sVarSim.pSeqRetour2 = (*itCDF)->GetSeqRetour2();
        sVarSim.nTypeGestionPrePriorite = (*itCDF)->GetTypeGestionPrePriorite();

        m_mapCDF.insert( make_pair( (*itCDF), sVarSim ) );
    }

	// Les tuyaux microscopiques et les voies
	std::deque<Tuyau*>::iterator itT;
	for( itT = pReseau->m_LstTuyaux.begin(); itT != pReseau->m_LstTuyaux.end(); itT++)
	{
        TuyauMicro * pTuyauMicro = dynamic_cast<TuyauMicro*>(*itT);
        if(pTuyauMicro)
        {
            stVarSimTuy & sVarSimT = m_mapTuy[pTuyauMicro];

            sVarSimT.nResolution = pTuyauMicro->GetResolution();

		    sVarSimT.mapDernierVehSortant = pTuyauMicro->m_mapDernierVehSortant;
		    sVarSimT.mapNbVehEntre = pTuyauMicro->m_mapNbVehEntre;
		    sVarSimT.mapNbVehEntrePrev = pTuyauMicro->m_mapNbVehEntrePrev;
		    sVarSimT.mapNbVehSorti = pTuyauMicro->m_mapNbVehSorti;
		    sVarSimT.mapNbVehSortiPrev = pTuyauMicro->m_mapNbVehSortiPrev;
		    sVarSimT.mapVeh = pTuyauMicro->m_mapVeh;
			sVarSimT.bIsForbidden = pTuyauMicro->GetIsForbidden();
#ifdef USE_SYMUCORE
            sVarSimT.mapMarginalByMacroType = pTuyauMicro->m_mapMarginalByMacroType;
            sVarSimT.mapTTByMacroType = pTuyauMicro->m_mapTTByMacroType;
            sVarSimT.dbTTForAllMacroTypes = pTuyauMicro->m_dbTTForAllMacroTypes;
			sVarSimT.dbEmissionForAllMacroTypes = pTuyauMicro->m_dbEmissionForAllMacroTypes;
            sVarSimT.dbMeanNbVehiclesForTravelTimeUpdatePeriod = pTuyauMicro->m_dbMeanNbVehiclesForTravelTimeUpdatePeriod;
            if (pTuyauMicro->m_pEdieSensor)
            {
                sVarSimT.sensorSnapshot = pTuyauMicro->m_pEdieSensor->TakeSnapshot();
            }
#endif

		    for( int nV = 0; nV < pTuyauMicro->getNbVoiesDis(); nV++)
		    {
			    stVarSimVoie sVarSimV;
			    sVarSimV.dbNextInstSortie = ((VoieMicro*)(pTuyauMicro->GetLstLanes()[nV]))->GetNextInstSortie();

			    m_mapVoie.insert( make_pair((VoieMicro*)(pTuyauMicro->GetLstLanes()[nV]), sVarSimV));
		    }
        }

        // rmq. : comme CTuyauMeso hérite de TuyauMicro, les variables de simulation de la classe TuyauMicro
        // sont déjà sauvegardées par le bloc ci-dessus.
        CTuyauMeso * pTuyauMeso = dynamic_cast<CTuyauMeso*>(*itT);
        if(pTuyauMeso)
        {
            stVarSimTuyMeso sVarSimT;
            for(size_t i = 0; i < pTuyauMeso->getCurrentVehicules().size(); i++)
            {
                sVarSimT.currentVehicules.push_back(pTuyauMeso->getCurrentVehicules()[i]->GetID());
            }
            sVarSimT.dCurrentMeanW = pTuyauMeso->getCurrentMeanW();
            sVarSimT.dCurrentMeanK = pTuyauMeso->getCurrentMeanK();
            std::list<std::pair<double, Vehicule*> >::const_iterator iterArrival;
            for(iterArrival = pTuyauMeso->getArrivals().begin(); iterArrival != pTuyauMeso->getArrivals().end(); ++iterArrival)
            {
                sVarSimT.arrivals.push_back(std::pair<double, int>(iterArrival->first,iterArrival->second->GetID()));
            }
            sVarSimT.nextArrival = std::pair<double, int>(pTuyauMeso->GetNextArrival().first, pTuyauMeso->GetNextArrival().second == NULL ? -1 : pTuyauMeso->GetNextArrival().second->GetID());
            sVarSimT.upstreamPassingTimes = pTuyauMeso->GetUpstreamPassingTimes();
            sVarSimT.downstreamPassingTimes = pTuyauMeso->getDownstreamPassingTimes();
            sVarSimT.dSupplyTime = pTuyauMeso->GetNextSupplyTime();
			sVarSimT.bIsForbidden = pTuyauMeso->GetIsForbidden();

            this->m_mapTuyMeso.insert( make_pair( pTuyauMeso, sVarSimT) );
        }

	}

    // Les zones
    for (size_t iZone = 0; iZone < pReseau->Liste_zones.size(); iZone++)
    {
        ZoneDeTerminaison * pZone = pReseau->Liste_zones[iZone];
        stVarSimZone & sVarSimZ = m_mapZones[pZone];

#ifdef USE_SYMUCORE
        sVarSimZ.mapMeanSpeed = pZone->m_dbMeanSpeed;
        sVarSimZ.dbConcentration = pZone->m_dbConcentration;
        if (pZone->m_pMFDSensor)
        {
            sVarSimZ.sensorSnapshot = pZone->m_pMFDSensor->TakeSnapshot();
        }
        sVarSimZ.zoneData.mapDepartingVeh = pZone->m_AreaHelper.GetMapDepartingVehicles();
        sVarSimZ.zoneData.mapArrivingVeh = pZone->m_AreaHelper.GetMapArrivingVehicles();
        sVarSimZ.zoneData.lstDepartingTravelTimes = pZone->m_AreaHelper.GetMapDepartingTT();
        sVarSimZ.zoneData.lstDepartingMarginals = pZone->m_AreaHelper.GetMapDepartingMarginals();
        sVarSimZ.zoneData.lstArrivingTravelTimes = pZone->m_AreaHelper.GetMapArrivingTT();
        sVarSimZ.zoneData.lstArrivingMarginals = pZone->m_AreaHelper.GetMapArrivingMarginals();
        sVarSimZ.zoneData.lstArrivingMeanNbVehicles = pZone->m_AreaHelper.GetMapArrivingMeanNbVehicles();
        sVarSimZ.zoneData.lstDepartingMeanNbVehicles = pZone->m_AreaHelper.GetMapDepartingMeanNbVehicles();
        sVarSimZ.zoneData.lstArrivingTTForAllMacroTypes = pZone->m_AreaHelper.GetMapArrivingTTForAllMacroTypes();
        sVarSimZ.zoneData.lstDepartingTTForAllMacroTypes = pZone->m_AreaHelper.GetMapDepartingTTForAllMacroTypes();
		sVarSimZ.zoneData.lstArrivingEmissionForAllMacroTypes = pZone->m_AreaHelper.GetMapArrivingEmissionForAllMacroTypes();
		sVarSimZ.zoneData.lstDepartingEmissionForAllMacroTypes = pZone->m_AreaHelper.GetMapDepartingEmissionForAllMacroTypes();

        for (size_t iPlaque = 0; iPlaque < pZone->GetLstPlaques().size(); iPlaque++)
        {
            CPlaque * pPlaque = pZone->GetLstPlaques()[iPlaque];

            stVarSimPlaque & sVarSimPlaque = sVarSimZ.mapPlaques[pPlaque];
            sVarSimPlaque.mapDepartingVeh = pPlaque->m_AreaHelper.GetMapDepartingVehicles();
            sVarSimPlaque.mapArrivingVeh = pPlaque->m_AreaHelper.GetMapArrivingVehicles();
            sVarSimPlaque.lstDepartingTravelTimes = pPlaque->m_AreaHelper.GetMapDepartingTT();
            sVarSimPlaque.lstDepartingMarginals = pPlaque->m_AreaHelper.GetMapDepartingMarginals();
            sVarSimPlaque.lstArrivingTravelTimes = pPlaque->m_AreaHelper.GetMapArrivingTT();
            sVarSimPlaque.lstArrivingMarginals = pPlaque->m_AreaHelper.GetMapArrivingMarginals();
            sVarSimPlaque.lstArrivingMeanNbVehicles = pPlaque->m_AreaHelper.GetMapArrivingMeanNbVehicles();
            sVarSimPlaque.lstDepartingMeanNbVehicles = pPlaque->m_AreaHelper.GetMapDepartingMeanNbVehicles();
            sVarSimPlaque.lstArrivingTTForAllMacroTypes = pPlaque->m_AreaHelper.GetMapArrivingTTForAllMacroTypes();
            sVarSimPlaque.lstDepartingTTForAllMacroTypes = pPlaque->m_AreaHelper.GetMapDepartingTTForAllMacroTypes();
			sVarSimPlaque.lstArrivingEmissionForAllMacroTypes = pPlaque->m_AreaHelper.GetMapArrivingEmissionForAllMacroTypes();
			sVarSimPlaque.lstDepartingEmissionForAllMacroTypes = pPlaque->m_AreaHelper.GetMapDepartingEmissionForAllMacroTypes();
        }
#endif
    }

	// Les entrées	
	std::deque<SymuViaTripNode*>::iterator itE;

	for( itE = pReseau->Liste_origines.begin(); itE != pReseau->Liste_origines.end(); itE++)
	{
		stVarSimEntree sVarSimE;

		sVarSimE.dbCritCreationVeh = (*itE)->m_dbCritCreationVeh;
        sVarSimE.lstVehiculesACreer = (*itE)->GetLstCreationsVehicule();
        sVarSimE.lstInstantsSortieStationnement = (*itE)->GetLstSortiesStationnement();
		sVarSimE.mapDrivenPaths = (*itE)->GetMapDrivenPaths();
		sVarSimE.mapVehPret = (*itE)->m_mapVehPret;		

		// Les véhicules en attente aux entrées...
        std::map<Tuyau*, std::map<int, std::deque<boost::shared_ptr<Vehicule>>>>::const_iterator iterVehAttente;
        for (iterVehAttente = (*itE)->m_mapVehEnAttente.begin(); iterVehAttente != (*itE)->m_mapVehEnAttente.end(); ++iterVehAttente)
        {
            std::map<int, std::deque<boost::shared_ptr<Vehicule>> > ::const_iterator iterMapVehAttente;
            for (iterMapVehAttente = iterVehAttente->second.begin(); iterMapVehAttente != iterVehAttente->second.end(); ++iterMapVehAttente)
            {
                sVarSimE.mapVehEnAttente[iterVehAttente->first].insert(pair<int, std::deque<boost::shared_ptr<Vehicule>>>(iterMapVehAttente->first, std::deque<boost::shared_ptr<Vehicule>>(0)));

                for (itVehDeq = iterMapVehAttente->second.begin(); itVehDeq != iterMapVehAttente->second.end(); itVehDeq++)
                {
                    boost::shared_ptr<Vehicule> pV(new Vehicule());
                    (*itVehDeq)->CopyTo(pV);
                    sVarSimE.mapVehEnAttente[iterVehAttente->first][iterMapVehAttente->first].push_back(pV);
                }
            }          
        }
		m_mapOrigine.insert( make_pair((*itE), sVarSimE));	
	}

	// Gestion capteur
	if (pReseau->GetGestionsCapteur() != NULL)
	{
        // sauvegarde des données capteurs
        const std::vector<AbstractSensor*> & sensors = pReseau->GetGestionsCapteur()->GetAllSensors();
        for(size_t iSensor = 0; iSensor < sensors.size(); iSensor++)
        {
            AbstractSensor * pSensor = sensors[iSensor];
            AbstractSensorSnapshot * pSnapshot = pSensor->TakeSnapshot();
            this->m_mapCapteur.insert( make_pair( pSensor, pSnapshot) );
        }

        // sauvegarde des numéros de périodes
        m_nGestionCapteurPeriodes = pReseau->GetGestionsCapteur()->GetPeriodes();
	}

    // sauvegarde de l'état des calculs de temps de parcours pour la sortie JSON
    if (m_pTravelTimesOutputSnapshot)
    {
        delete m_pTravelTimesOutputSnapshot;
        m_pTravelTimesOutputSnapshot = NULL;
    }
    if (pReseau->GetTravelTimesOutputManager())
    {
        m_pTravelTimesOutputSnapshot = pReseau->GetTravelTimesOutputManager()->TakeSnapshot();
    }

    // Convergents
    std::map<std::string, Convergent*>::const_iterator itC;
    for(itC = pReseau->Liste_convergents.begin(); itC != pReseau->Liste_convergents.end(); itC++)
    {
        stVarSimConvergent stVarSimCvgt;
        for(size_t iPtCvg = 0; iPtCvg < itC->second->m_LstPtCvg.size(); iPtCvg++)
        {
            stVarSimCvgt.dbInstLastPassage.push_back(itC->second->m_LstPtCvg[iPtCvg]->m_dbInstLastPassage);
        }
        m_mapConvergents.insert( make_pair( itC->second, stVarSimCvgt));
    }

    // Crossroads
    std::deque<CarrefourAFeuxEx*>::const_iterator itCr;
    for(itCr = pReseau->Liste_carrefoursAFeux.begin(); itCr != pReseau->Liste_carrefoursAFeux.end(); itCr++)
    {
        stVarSimCrossroads stVarSimCrds;
        for(size_t iMvt = 0; iMvt < (*itCr)->m_LstMouvements.size(); iMvt++)
        {
            for(size_t iPts = 0; iPts < (*itCr)->m_LstMouvements[iMvt]->lstGrpPtsConflitTraversee.size(); iPts++)
                stVarSimCrds.dbInstLastCrossing.push_back((*itCr)->m_LstMouvements[iMvt]->lstGrpPtsConflitTraversee[iPts]->dbInstLastTraversee);
        }
        m_mapCrossroads.insert( make_pair( (*itCr), stVarSimCrds));
    }

    // Briques de régulation
    std::vector<RegulationBrique*> & lstBriques = pReseau->GetModuleRegulation()->GetLstBriquesRegulation();
    for(size_t iBrique = 0; iBrique < lstBriques.size(); iBrique++)
    {
        stVarSimRegulationBrique stVarSimRegul;
        for(size_t iCapteur = 0; iCapteur < lstBriques[iBrique]->GetLstCapteurs().size(); iCapteur++)
        {
            stVarSimRegul.lstContextes.push_back(lstBriques[iBrique]->GetLstCapteurs()[iCapteur]->GetContext().copy());
        }
        for(size_t iCondition = 0; iCondition < lstBriques[iBrique]->GetLstConditions().size(); iCondition++)
        {
            stVarSimRegul.lstContextes.push_back(lstBriques[iBrique]->GetLstConditions()[iCondition]->GetContext().copy());
        }
        for(size_t iAction = 0; iAction < lstBriques[iBrique]->GetLstActions().size(); iAction++)
        {
            stVarSimRegul.lstContextes.push_back(lstBriques[iBrique]->GetLstActions()[iAction]->GetContext().copy());
        }
        m_mapRegulations.insert(make_pair(lstBriques[iBrique], stVarSimRegul));
    }

    // Briques de connexions
	for(size_t i = 0; i < pReseau->Liste_carrefoursAFeux.size(); i++)
	{
		stVarSimBrique sVarSimB;

		sVarSimB.mapCoutsMesures = pReseau->Liste_carrefoursAFeux[i]->m_LstCoutsMesures;
        sVarSimB.mapVeh = pReseau->Liste_carrefoursAFeux[i]->m_mapVeh;

#ifdef USE_SYMUCORE
        sVarSimB.mapTTByMacroType = pReseau->Liste_carrefoursAFeux[i]->m_mapTTByMacroType;
        sVarSimB.mapMarginalByMacroType = pReseau->Liste_carrefoursAFeux[i]->m_mapMarginalByMacroType;
        sVarSimB.dbMeanNbVehiclesForTravelTimeUpdatePeriod = pReseau->Liste_carrefoursAFeux[i]->m_dbMeanNbVehiclesForTravelTimeUpdatePeriod;
        sVarSimB.dbTTForAllMacroTypes = pReseau->Liste_carrefoursAFeux[i]->m_dbTTForAllMacroTypes;
		sVarSimB.dbEmissionsForAllMacroTypes = pReseau->Liste_carrefoursAFeux[i]->m_dbEmissionsForAllMacroTypes;
#endif // USE_SYMUCORE
        
		this->m_mapBrique.insert( make_pair( pReseau->Liste_carrefoursAFeux[i], sVarSimB) );
	}
    for(size_t i = 0; i < pReseau->Liste_giratoires.size(); i++)
	{
		stVarSimBrique sVarSimB;

		sVarSimB.mapCoutsMesures = pReseau->Liste_giratoires[i]->m_LstCoutsMesures;
        sVarSimB.mapVeh = pReseau->Liste_giratoires[i]->m_mapVeh;

#ifdef USE_SYMUCORE
        sVarSimB.mapTTByMacroType = pReseau->Liste_giratoires[i]->m_mapTTByMacroType;
        sVarSimB.mapMarginalByMacroType = pReseau->Liste_giratoires[i]->m_mapMarginalByMacroType;
        sVarSimB.dbMeanNbVehiclesForTravelTimeUpdatePeriod = pReseau->Liste_giratoires[i]->m_dbMeanNbVehiclesForTravelTimeUpdatePeriod;
        sVarSimB.dbTTForAllMacroTypes = pReseau->Liste_giratoires[i]->m_dbTTForAllMacroTypes;
		sVarSimB.dbEmissionsForAllMacroTypes = pReseau->Liste_giratoires[i]->m_dbEmissionsForAllMacroTypes;
#endif // USE_SYMUCORE

		this->m_mapBrique.insert( make_pair( pReseau->Liste_giratoires[i], sVarSimB) );
	}

    // Noeuds Meso
    for(size_t i = 0; i < pReseau->m_LstUserCnxs.size(); i++)
    {
        CMesoNode * pMesoNode = pReseau->m_LstUserCnxs[i];

        stVarSimMesoNode sVarSimMN;

        std::map<Tuyau*,std::list< std::pair<double, Vehicule* > > >::const_iterator iterArrivals;
        for(iterArrivals = pMesoNode->getArrivals().begin(); iterArrivals != pMesoNode->getArrivals().end(); ++iterArrivals)
        {
            std::list< std::pair<double, int > > listArrivals;
            std::list< std::pair<double, Vehicule* > >::const_iterator iterListArrivals;
            for(iterListArrivals = iterArrivals->second.begin(); iterListArrivals != iterArrivals->second.end(); ++iterListArrivals)
            {
                listArrivals.push_back(make_pair(iterListArrivals->first, iterListArrivals->second->GetID()));
            }
            sVarSimMN.arrivals[iterArrivals->first] = listArrivals;
        }

        sVarSimMN.upstreamPassingTimes = pMesoNode->getUpstreamPassingTimes();
        sVarSimMN.downstreamPassingTimes = pMesoNode->getDownstreamPassingTimes();

        std::map<Tuyau*, std::pair<double, Vehicule* > >::const_iterator iterNextArrival;
        for(iterNextArrival = pMesoNode->getNextArrivals().begin(); iterNextArrival != pMesoNode->getNextArrivals().end(); ++iterNextArrival)
        {
            sVarSimMN.nextArrivals[iterNextArrival->first] = make_pair(iterNextArrival->second.first, iterNextArrival->second.second != NULL ? iterNextArrival->second.second->GetID() : -1);
        }

        sVarSimMN.mapRanksIncomming = pMesoNode->getMapRanksIncomming();
        sVarSimMN.mapRanksOutGoing = pMesoNode->getMapRanksOutGoing();
        sVarSimMN.pNextEventTuyauEntry = pMesoNode->GetNextEventIncommingLink();
        sVarSimMN.dNextEventTime = pMesoNode->GetNextEventTime();
        sVarSimMN.dNextSupplyTime = pMesoNode->getNextSupplyTime();

        this->m_mapMesoNode.insert( make_pair( pMesoNode, sVarSimMN) );
    }

#ifdef USE_SYMUCOM
    // Sauvegarde des ITS
    if (m_pITSSnapshot)
    {
        delete m_pITSSnapshot;
    }
	if (pReseau->m_pSymucomSimulator)
		m_pITSSnapshot = pReseau->m_pSymucomSimulator->TakeSimulatorSnapshot();
#endif // USE_SYMUCOM
}

void SimulationSnapshot::Restore(Reseau *pReseau)
{
	pReseau->m_dbInstSimu = m_dbInstSvg;
	pReseau->m_nInstSim = m_nInstSvg;

	pReseau->m_nLastVehicleID = m_nLastIdVehSvg;

    pReseau->RestoreSeed(m_nRandCountSvg);
    pReseau->m_nNbVehCum = m_nNbVehCumSvg;

	// Suppression des noeuds véhicules du fichier de sortie
    pReseau->log() << std::endl << "Vehicles removal since ID " << m_nLastIdVehSvg << std::endl;
    std::deque< TimeVariation<TraceDocTrafic> >::iterator it;
    for( it = pReseau->m_xmlDocTrafics.begin(); it!= pReseau->m_xmlDocTrafics.end(); it++ )
    {
        it->m_pData->RemoveVehicules( m_nLastIdVehSvg );
     }

	// Restitution des véhicules à partir de la liste sauvegardée
	pReseau->m_LstVehicles.clear();

	std::deque <boost::shared_ptr<Vehicule>>::iterator itVehSvg;    
	for(itVehSvg = this->m_lstVehSvg.begin(); itVehSvg != m_lstVehSvg.end(); itVehSvg++)
	{
		boost::shared_ptr<Vehicule> pV(new Vehicule());
        (*itVehSvg)->CopyTo( pV );
		pReseau->m_LstVehicles.push_back( pV );
	}

    // Restitution des leaders des véhicules présents sur le réseau (les pointeurs ici présents ne sont plus valables car
    // les véhicules ont été réinstanciés)
    std::vector<boost::shared_ptr<Vehicule>>::iterator itVeh;    
	for(itVeh = pReseau->m_LstVehicles.begin(); itVeh != pReseau->m_LstVehicles.end(); ++itVeh)
	{
        if((*itVeh)->GetCarFollowing()->GetCurrentContext())
        {
            const std::vector<boost::shared_ptr<Vehicule> > leaders = (*itVeh)->GetCarFollowing()->GetCurrentContext()->GetLeaders();
            std::vector<boost::shared_ptr<Vehicule> > newLeaders(leaders.size());
            for(size_t iLeader = 0; iLeader < leaders.size(); iLeader++)
            {
                newLeaders[iLeader] = pReseau->GetVehiculeFromID(leaders[iLeader]->GetID());
            }
            (*itVeh)->GetCarFollowing()->GetCurrentContext()->SetLeaders(newLeaders);
        }
    }

    // Restitution de l'état des contrôleurs de feux
	std::deque<ControleurDeFeux*>::iterator itCDF;
    for( itCDF = pReseau->Liste_ControleursDeFeux.begin(); itCDF != pReseau->Liste_ControleursDeFeux.end(); itCDF++)
	{
		stVarSimCDF sVarSim = m_mapCDF.find( (*itCDF) )->second;

        (*itCDF)->SetPDFActif(sVarSim.pPDFActif);
        (*itCDF)->SetModeFonctionnement(sVarSim.cModeFct);

        (*itCDF)->GetLstVGPs().clear();
        std::deque<std::pair<int, LGP*> >::iterator itLGP;
        for(itLGP = sVarSim.dqBus.begin(); itLGP != sVarSim.dqBus.end(); ++itLGP)
        {
            boost::shared_ptr<Vehicule> pV = pReseau->GetVehiculeFromID(itLGP->first);
            if(pV.get() != NULL)
                (*itCDF)->GetLstVGPs().push_back(std::make_pair(pV, itLGP->second));
        }

        (*itCDF)->SetPDFEx(sVarSim.pPDFEx);
        (*itCDF)->SetDebutPDFEx(sVarSim.dbDebPDFEx);
        (*itCDF)->SetFinPDFEx(sVarSim.dbFinPDFEx);
        (*itCDF)->SetSeqPrePriorite(sVarSim.pSeqPrePriorite);
        (*itCDF)->SetSeqRetour1(sVarSim.pSeqRetour1);
        (*itCDF)->SetSeqRetour2(sVarSim.pSeqRetour2);
        (*itCDF)->SetTypeGestionPrePriorite(sVarSim.nTypeGestionPrePriorite);
	}

	// Restitution des données de simulation d'un tronçon et de ses voies
	std::deque<Tuyau*>::iterator itT;
	for( itT = pReseau->m_LstTuyaux.begin(); itT != pReseau->m_LstTuyaux.end(); itT++)
	{
        TuyauMicro * pTuyauMicro = dynamic_cast<TuyauMicro*>(*itT);
        if(pTuyauMicro)
        {
		    const stVarSimTuy & sVarSimT = this->m_mapTuy.find(pTuyauMicro)->second;

		    pTuyauMicro->m_mapDernierVehSortant = sVarSimT.mapDernierVehSortant;
		    pTuyauMicro->m_mapNbVehEntre = sVarSimT.mapNbVehEntre;
		    pTuyauMicro->m_mapNbVehEntrePrev = sVarSimT.mapNbVehEntrePrev;
		    pTuyauMicro->m_mapNbVehSorti = sVarSimT.mapNbVehSorti;
		    pTuyauMicro->m_mapNbVehSortiPrev = sVarSimT.mapNbVehSortiPrev;
		    pTuyauMicro->m_mapVeh = sVarSimT.mapVeh;
			pTuyauMicro->SetForbidden(sVarSimT.bIsForbidden);

#ifdef USE_SYMUCORE
            pTuyauMicro->m_mapMarginalByMacroType = sVarSimT.mapMarginalByMacroType;
            pTuyauMicro->m_mapTTByMacroType = sVarSimT.mapTTByMacroType;
            pTuyauMicro->m_dbTTForAllMacroTypes = sVarSimT.dbTTForAllMacroTypes;
			pTuyauMicro->m_dbEmissionForAllMacroTypes = sVarSimT.dbEmissionForAllMacroTypes;
            pTuyauMicro->m_dbMeanNbVehiclesForTravelTimeUpdatePeriod = sVarSimT.dbMeanNbVehiclesForTravelTimeUpdatePeriod;
            if (pTuyauMicro->m_pEdieSensor)
            {
                pTuyauMicro->m_pEdieSensor->Restore(pReseau, sVarSimT.sensorSnapshot);
            }
#endif

		    // Voie
		    for( int nV = 0; nV < pTuyauMicro->getNbVoiesDis(); nV++)
		    {
			    stVarSimVoie sVarSimV = this->m_mapVoie.find( (VoieMicro*)(pTuyauMicro->GetLstLanes()[nV]) )->second;
			    ((VoieMicro*)(pTuyauMicro->GetLstLanes()[nV]))->GetNextInstSortie() = sVarSimV.dbNextInstSortie;
		    }

            // On restitue la résolution du tuyau et on place le tuyau dans la bonne liste le cas échéant
            if(pTuyauMicro->GetResolution() != sVarSimT.nResolution)
            {
                pTuyauMicro->SetResolution(sVarSimT.nResolution);
                if(sVarSimT.nResolution == RESOTUYAU_MICRO)
                {
                    pReseau->m_LstTuyauxMeso.erase(std::find(pReseau->m_LstTuyauxMeso.begin(), pReseau->m_LstTuyauxMeso.end(), pTuyauMicro));
                    pReseau->m_LstTuyauxMicro.push_back(pTuyauMicro);
                }
                else
                {
                    CTuyauMeso * pTuyMeso = dynamic_cast<CTuyauMeso*>(pTuyauMicro);
                    assert(pTuyMeso);
                    pReseau->m_LstTuyauxMicro.erase(std::find(pReseau->m_LstTuyauxMicro.begin(), pReseau->m_LstTuyauxMicro.end(), pTuyauMicro));
                    pReseau->m_LstTuyauxMeso.push_back(pTuyMeso);
                }
            }
        }

        CTuyauMeso * pTuyauMeso = dynamic_cast<CTuyauMeso*>(*itT);
        if(pTuyauMeso)
        {
		    stVarSimTuyMeso sVarSimT = this->m_mapTuyMeso.find(pTuyauMeso)->second;
            pTuyauMeso->getCurrentVehicules().clear();
            for(size_t i = 0; i < sVarSimT.currentVehicules.size(); i++)
            {
                pTuyauMeso->getCurrentVehicules().push_back(pReseau->GetVehiculeFromID(sVarSimT.currentVehicules[i]).get());
            }
            pTuyauMeso->setCurrentMeanW(sVarSimT.dCurrentMeanW);
            pTuyauMeso->setCurrentMeanK(sVarSimT.dCurrentMeanK);

            pTuyauMeso->getArrivals().clear();
            std::list<std::pair<double, int> >::const_iterator iterArrival;
            for(iterArrival = sVarSimT.arrivals.begin(); iterArrival != sVarSimT.arrivals.end(); ++iterArrival)
            {
                pTuyauMeso->getArrivals().push_back(std::pair<double, Vehicule*>(iterArrival->first, pReseau->GetVehiculeFromID(iterArrival->second).get()));
            }
            pTuyauMeso->SetNextArrival(std::pair<double, Vehicule*>(sVarSimT.nextArrival.first, sVarSimT.nextArrival.second != -1 ? pReseau->GetVehiculeFromID(sVarSimT.nextArrival.second).get() : NULL));
            pTuyauMeso->setUpstreamPassingTimes(sVarSimT.upstreamPassingTimes);
            pTuyauMeso->getDownstreamPassingTimes() = sVarSimT.downstreamPassingTimes;
            pTuyauMeso->SetNextSupplyTime(sVarSimT.dSupplyTime);
			pTuyauMeso->SetForbidden(sVarSimT.bIsForbidden);
        }
	}

    // Restitution de l'état des zones
    for (size_t iZone = 0; iZone < pReseau->Liste_zones.size(); iZone++)
    {
        ZoneDeTerminaison * pZone = pReseau->Liste_zones[iZone];

        const stVarSimZone & sVarSimZ = this->m_mapZones.at(pZone);

#ifdef USE_SYMUCORE
        pZone->m_dbMeanSpeed = sVarSimZ.mapMeanSpeed;
        pZone->m_dbConcentration = sVarSimZ.dbConcentration;
        if (pZone->m_pMFDSensor)
        {
            pZone->m_pMFDSensor->Restore(pReseau, sVarSimZ.sensorSnapshot);
        }

        pZone->m_AreaHelper.GetMapDepartingVehicles() = sVarSimZ.zoneData.mapDepartingVeh;
        pZone->m_AreaHelper.GetMapArrivingVehicles() = sVarSimZ.zoneData.mapArrivingVeh;
        pZone->m_AreaHelper.GetMapDepartingTT() = sVarSimZ.zoneData.lstDepartingTravelTimes;
        pZone->m_AreaHelper.GetMapDepartingMarginals() = sVarSimZ.zoneData.lstDepartingMarginals;
        pZone->m_AreaHelper.GetMapArrivingTT() = sVarSimZ.zoneData.lstArrivingTravelTimes;
        pZone->m_AreaHelper.GetMapArrivingMarginals() = sVarSimZ.zoneData.lstArrivingMarginals;
        pZone->m_AreaHelper.GetMapArrivingMeanNbVehicles() = sVarSimZ.zoneData.lstArrivingMeanNbVehicles;
        pZone->m_AreaHelper.GetMapDepartingMeanNbVehicles() = sVarSimZ.zoneData.lstDepartingMeanNbVehicles;
        pZone->m_AreaHelper.GetMapArrivingTTForAllMacroTypes() = sVarSimZ.zoneData.lstArrivingTTForAllMacroTypes;
        pZone->m_AreaHelper.GetMapDepartingTTForAllMacroTypes() = sVarSimZ.zoneData.lstDepartingTTForAllMacroTypes;
		pZone->m_AreaHelper.GetMapArrivingEmissionForAllMacroTypes() = sVarSimZ.zoneData.lstArrivingEmissionForAllMacroTypes;
		pZone->m_AreaHelper.GetMapDepartingEmissionForAllMacroTypes() = sVarSimZ.zoneData.lstDepartingEmissionForAllMacroTypes;

        for (size_t iPlaque = 0; iPlaque < pZone->GetLstPlaques().size(); iPlaque++)
        {
            CPlaque * pPlaque = pZone->GetLstPlaques()[iPlaque];

            const stVarSimPlaque & sVarSimPlaque = sVarSimZ.mapPlaques.at(pPlaque);
            pPlaque->m_AreaHelper.GetMapDepartingVehicles() = sVarSimPlaque.mapDepartingVeh;
            pPlaque->m_AreaHelper.GetMapArrivingVehicles() = sVarSimPlaque.mapArrivingVeh;
            pPlaque->m_AreaHelper.GetMapDepartingTT() = sVarSimPlaque.lstDepartingTravelTimes;
            pPlaque->m_AreaHelper.GetMapDepartingMarginals() = sVarSimPlaque.lstDepartingMarginals;
            pPlaque->m_AreaHelper.GetMapArrivingTT() = sVarSimPlaque.lstArrivingTravelTimes;
            pPlaque->m_AreaHelper.GetMapArrivingMarginals() = sVarSimPlaque.lstArrivingMarginals;
            pPlaque->m_AreaHelper.GetMapArrivingMeanNbVehicles() = sVarSimPlaque.lstArrivingMeanNbVehicles;
            pPlaque->m_AreaHelper.GetMapDepartingMeanNbVehicles() = sVarSimPlaque.lstDepartingMeanNbVehicles;
            pPlaque->m_AreaHelper.GetMapArrivingTTForAllMacroTypes() = sVarSimPlaque.lstArrivingTTForAllMacroTypes;
            pPlaque->m_AreaHelper.GetMapDepartingTTForAllMacroTypes() = sVarSimPlaque.lstDepartingTTForAllMacroTypes;
			pPlaque->m_AreaHelper.GetMapArrivingEmissionForAllMacroTypes() = sVarSimPlaque.lstArrivingEmissionForAllMacroTypes;
			pPlaque->m_AreaHelper.GetMapDepartingEmissionForAllMacroTypes() = sVarSimPlaque.lstDepartingEmissionForAllMacroTypes;
        }
#endif
    }

	// Restitution de l'état des entrées
	std::deque<SymuViaTripNode*>::iterator itE;
	for( itE = pReseau->Liste_origines.begin(); itE != pReseau->Liste_origines.end(); itE++)
	{
		stVarSimEntree sVarSimE = m_mapOrigine.find( (*itE) )->second;

		(*itE)->GetLstCreationsVehicule() = sVarSimE.lstVehiculesACreer;
        (*itE)->m_dbCritCreationVeh = sVarSimE.dbCritCreationVeh;
        (*itE)->GetLstSortiesStationnement() = sVarSimE.lstInstantsSortieStationnement;
		(*itE)->GetMapDrivenPaths() = sVarSimE.mapDrivenPaths;
		(*itE)->m_mapVehPret = sVarSimE.mapVehPret;

        // Ménage
        (*itE)->m_mapVehEnAttente.clear();
        std::map<Tuyau*, std::map<int, std::deque<boost::shared_ptr<Vehicule>> > >::const_iterator iterVehAttente;
        for (iterVehAttente = sVarSimE.mapVehEnAttente.begin(); iterVehAttente != sVarSimE.mapVehEnAttente.end(); ++iterVehAttente)
        {
            std::map<int, std::deque<boost::shared_ptr<Vehicule>> >::const_iterator iterMapVehAttente;
            for (iterMapVehAttente = iterVehAttente->second.begin(); iterMapVehAttente != iterVehAttente->second.end(); ++iterMapVehAttente)
            {
                std::deque <boost::shared_ptr<Vehicule> >::const_iterator itVehSvg;
                std::deque <boost::shared_ptr<Vehicule> > & dequeVehAttente = (*itE)->m_mapVehEnAttente[iterVehAttente->first][iterMapVehAttente->first];
                for (itVehSvg = iterMapVehAttente->second.begin(); itVehSvg != iterMapVehAttente->second.end(); itVehSvg++)
                {
                    boost::shared_ptr<Vehicule> pV(new Vehicule());
                    (*itVehSvg)->CopyTo(pV);
                    dequeVehAttente.push_back(pV);
                }
            }
        }
    }

    // Restitution de l'état des flottes de véhicules (après la restitution des véhicules en attente)
    for (size_t iFleet = 0; iFleet < pReseau->m_LstFleets.size(); iFleet++)
    {
        AbstractFleet * pFleet = pReseau->m_LstFleets[iFleet];
        AbstractFleetSnapshot * pSnapshot = this->m_mapFleet[pFleet];
        pFleet->Restore(pReseau, pSnapshot);
    }

	// Gestion capteur
	if (pReseau->GetGestionsCapteur() != NULL)
	{
		pReseau->GetGestionsCapteur()->Init();	

        // restitution des données capteurs
        const std::vector<AbstractSensor*> & sensors = pReseau->GetGestionsCapteur()->GetAllSensors();
        for(size_t iSensor = 0; iSensor < sensors.size(); iSensor++)
        {
            AbstractSensor * pSensor = sensors[iSensor];
            AbstractSensorSnapshot * pSnapshot = this->m_mapCapteur[pSensor];
            pSensor->Restore(pReseau, pSnapshot);
        }

        pReseau->GetGestionsCapteur()->SetPeriodes(m_nGestionCapteurPeriodes);
	}

    // Restitution de l'état du gestionnaire de sortie des temps de parcours
    if (pReseau->GetTravelTimesOutputManager())
    {
        pReseau->GetTravelTimesOutputManager()->Restore(m_pTravelTimesOutputSnapshot);
    }

    // Convergents
    std::map<std::string, Convergent*>::const_iterator itC;
    for(itC = pReseau->Liste_convergents.begin(); itC != pReseau->Liste_convergents.end(); itC++)
    {
        stVarSimConvergent stVarSimCvgt = m_mapConvergents.find(itC->second)->second;
        for(size_t iPtCvg = 0; iPtCvg < itC->second->m_LstPtCvg.size(); iPtCvg++)
        {
            itC->second->m_LstPtCvg[iPtCvg]->m_dbInstLastPassage = stVarSimCvgt.dbInstLastPassage[iPtCvg];
        }
    }

    // Crossroads
    std::deque<CarrefourAFeuxEx*>::const_iterator itCr;
    for(itCr = pReseau->Liste_carrefoursAFeux.begin(); itCr != pReseau->Liste_carrefoursAFeux.end(); itCr++)
    {
        stVarSimCrossroads stVarSimCrds = m_mapCrossroads.find((*itCr))->second;
        size_t nit = 0; 
        for(size_t iMvt = 0; iMvt < (*itCr)->m_LstMouvements.size(); iMvt++)
        {
            for(size_t iPts = 0; iPts < (*itCr)->m_LstMouvements[iMvt]->lstGrpPtsConflitTraversee.size(); iPts++)
            {
                (*itCr)->m_LstMouvements[iMvt]->lstGrpPtsConflitTraversee[iPts]->dbInstLastTraversee = stVarSimCrds.dbInstLastCrossing[nit];
                nit++;
            }
        }
    }

    // Briques de régulation
    std::vector<RegulationBrique*> & lstBriques = pReseau->GetModuleRegulation()->GetLstBriquesRegulation();
    for(size_t iBrique = 0; iBrique < lstBriques.size(); iBrique++)
    {
        stVarSimRegulationBrique & stVarSimRegul = m_mapRegulations.find(lstBriques[iBrique])->second;
        size_t iContext = 0;
        for(size_t iCapteur = 0; iCapteur < lstBriques[iBrique]->GetLstCapteurs().size(); iCapteur++)
        {
            lstBriques[iBrique]->GetLstCapteurs()[iCapteur]->SetContext(stVarSimRegul.lstContextes[iContext++]);
        }
        for(size_t iCondition = 0; iCondition < lstBriques[iBrique]->GetLstConditions().size(); iCondition++)
        {
            lstBriques[iBrique]->GetLstConditions()[iCondition]->SetContext(stVarSimRegul.lstContextes[iContext++]);
        }
        for(size_t iAction = 0; iAction < lstBriques[iBrique]->GetLstActions().size(); iAction++)
        {
            lstBriques[iBrique]->GetLstActions()[iAction]->SetContext(stVarSimRegul.lstContextes[iContext++]);
        }
    }

    // Briques de connexions
	for(size_t i = 0; i < pReseau->Liste_carrefoursAFeux.size(); i++)
	{
        pReseau->Liste_carrefoursAFeux[i]->m_LstCoutsMesures = this->m_mapBrique[pReseau->Liste_carrefoursAFeux[i]].mapCoutsMesures;
        pReseau->Liste_carrefoursAFeux[i]->m_mapVeh = this->m_mapBrique[pReseau->Liste_carrefoursAFeux[i]].mapVeh;
#ifdef USE_SYMUCORE
        pReseau->Liste_carrefoursAFeux[i]->m_mapTTByMacroType = this->m_mapBrique[pReseau->Liste_carrefoursAFeux[i]].mapTTByMacroType;
        pReseau->Liste_carrefoursAFeux[i]->m_mapMarginalByMacroType = this->m_mapBrique[pReseau->Liste_carrefoursAFeux[i]].mapMarginalByMacroType;
        pReseau->Liste_carrefoursAFeux[i]->m_dbMeanNbVehiclesForTravelTimeUpdatePeriod = this->m_mapBrique[pReseau->Liste_carrefoursAFeux[i]].dbMeanNbVehiclesForTravelTimeUpdatePeriod;
        pReseau->Liste_carrefoursAFeux[i]->m_dbTTForAllMacroTypes = this->m_mapBrique[pReseau->Liste_carrefoursAFeux[i]].dbTTForAllMacroTypes;
		pReseau->Liste_carrefoursAFeux[i]->m_dbEmissionsForAllMacroTypes = this->m_mapBrique[pReseau->Liste_carrefoursAFeux[i]].dbEmissionsForAllMacroTypes;
#endif // USE_SYMUCORE
	}
    for(size_t i = 0; i < pReseau->Liste_giratoires.size(); i++)
	{
		pReseau->Liste_giratoires[i]->m_LstCoutsMesures = this->m_mapBrique[pReseau->Liste_giratoires[i]].mapCoutsMesures;
        pReseau->Liste_giratoires[i]->m_mapVeh = this->m_mapBrique[pReseau->Liste_giratoires[i]].mapVeh;
#ifdef USE_SYMUCORE
        pReseau->Liste_giratoires[i]->m_mapTTByMacroType = this->m_mapBrique[pReseau->Liste_giratoires[i]].mapTTByMacroType;
        pReseau->Liste_giratoires[i]->m_mapMarginalByMacroType = this->m_mapBrique[pReseau->Liste_giratoires[i]].mapMarginalByMacroType;
        pReseau->Liste_giratoires[i]->m_dbMeanNbVehiclesForTravelTimeUpdatePeriod = this->m_mapBrique[pReseau->Liste_giratoires[i]].dbMeanNbVehiclesForTravelTimeUpdatePeriod;
        pReseau->Liste_giratoires[i]->m_dbTTForAllMacroTypes = this->m_mapBrique[pReseau->Liste_giratoires[i]].dbTTForAllMacroTypes;
		pReseau->Liste_giratoires[i]->m_dbEmissionsForAllMacroTypes = this->m_mapBrique[pReseau->Liste_giratoires[i]].dbEmissionsForAllMacroTypes;
#endif // USE_SYMUCORE
	}

    // Noeuds Meso
    for(size_t i = 0; i < pReseau->m_LstUserCnxs.size(); i++)
    {
        CMesoNode * pMesoNode = pReseau->m_LstUserCnxs[i];

        const stVarSimMesoNode & sVarSimMN = m_mapMesoNode[pMesoNode];

        pMesoNode->getArrivals().clear();
        std::map<Tuyau*,std::list< std::pair<double, int> > >::const_iterator iterArrivals;
        for(iterArrivals = sVarSimMN.arrivals.begin(); iterArrivals != sVarSimMN.arrivals.end(); ++iterArrivals)
        {
            
            std::list< std::pair<double, Vehicule* > > listArrivals;
            std::list< std::pair<double, int> >::const_iterator iterListArrivals;
            for(iterListArrivals = iterArrivals->second.begin(); iterListArrivals != iterArrivals->second.end(); ++iterListArrivals)
            {
                listArrivals.push_back(make_pair(iterListArrivals->first, pReseau->GetVehiculeFromID(iterListArrivals->second).get()));
            }
            pMesoNode->getArrivals()[iterArrivals->first] = listArrivals;
        }

        pMesoNode->getUpstreamPassingTimes() = sVarSimMN.upstreamPassingTimes;
        pMesoNode->getDownstreamPassingTimes() = sVarSimMN.downstreamPassingTimes;

        std::map<Tuyau*, std::pair<double, int> >::const_iterator iterNextArrival;
        for(iterNextArrival = sVarSimMN.nextArrivals.begin(); iterNextArrival != sVarSimMN.nextArrivals.end(); ++iterNextArrival)
        {
            pMesoNode->getNextArrivals()[iterNextArrival->first] = make_pair(iterNextArrival->second.first, pReseau->GetVehiculeFromID(iterNextArrival->second.second).get());
        }

        pMesoNode->getMapRanksIncomming() = sVarSimMN.mapRanksIncomming;
        pMesoNode->getMapRanksOutGoing() = sVarSimMN.mapRanksOutGoing;
        pMesoNode->SetGlobalNextEvent(sVarSimMN.dNextEventTime, sVarSimMN.pNextEventTuyauEntry);
        pMesoNode->getNextSupplyTime() = sVarSimMN.dNextSupplyTime;
    }

#ifdef USE_SYMUCOM
	if (pReseau->m_pSymucomSimulator)
		pReseau->m_pSymucomSimulator->Restore(m_pITSSnapshot);
#endif // USE_SYMUCOM
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void stVarSimEntree::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimEntree::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimEntree::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(mapVehEnAttente);
    ar & BOOST_SERIALIZATION_NVP(mapVehPret);
    ar & BOOST_SERIALIZATION_NVP(dbCritCreationVeh);
    ar & BOOST_SERIALIZATION_NVP(lstVehiculesACreer);
    ar & BOOST_SERIALIZATION_NVP(lstInstantsSortieStationnement);
	ar & BOOST_SERIALIZATION_NVP(mapDrivenPaths);
}

template void stVarSimCDF::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimCDF::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimCDF::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(pPDFActif);
    ar & BOOST_SERIALIZATION_NVP(cModeFct);
    ar & BOOST_SERIALIZATION_NVP(dqBus);
    ar & BOOST_SERIALIZATION_NVP(pPDFEx);
    ar & BOOST_SERIALIZATION_NVP(dbDebPDFEx);
    ar & BOOST_SERIALIZATION_NVP(dbFinPDFEx);
    ar & BOOST_SERIALIZATION_NVP(pSeqPrePriorite);
    ar & BOOST_SERIALIZATION_NVP(pSeqRetour1);
    ar & BOOST_SERIALIZATION_NVP(pSeqRetour2);
    ar & BOOST_SERIALIZATION_NVP(nTypeGestionPrePriorite);
}

template void stVarSimTuy::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimTuy::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimTuy::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(nResolution);
    ar & BOOST_SERIALIZATION_NVP(mapVeh);
    ar & BOOST_SERIALIZATION_NVP(mapDernierVehSortant);
    ar & BOOST_SERIALIZATION_NVP(mapNbVehEntre);
    ar & BOOST_SERIALIZATION_NVP(mapNbVehSorti);
    ar & BOOST_SERIALIZATION_NVP(mapNbVehSortiPrev);
    ar & BOOST_SERIALIZATION_NVP(mapNbVehEntrePrev);
	ar & BOOST_SERIALIZATION_NVP(bIsForbidden);
}

template void stVarSimZone::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimZone::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimZone::serialize(Archive & ar, const unsigned int version)
{
    // rmq. : pas la peine de sérialiser pour l'instant, car utilisé uniquement pour SymuMaster
}

template void stVarSimTuyMeso::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimTuyMeso::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimTuyMeso::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(currentVehicules);
    ar & BOOST_SERIALIZATION_NVP(dCurrentMeanW);
    ar & BOOST_SERIALIZATION_NVP(dCurrentMeanK);
    ar & BOOST_SERIALIZATION_NVP(arrivals);
    ar & BOOST_SERIALIZATION_NVP(nextArrival);
    ar & BOOST_SERIALIZATION_NVP(upstreamPassingTimes);
    ar & BOOST_SERIALIZATION_NVP(downstreamPassingTimes);
    ar & BOOST_SERIALIZATION_NVP(dSupplyTime);
	ar & BOOST_SERIALIZATION_NVP(bIsForbidden);
}

template void stVarSimMesoNode::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimMesoNode::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimMesoNode::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(arrivals);
    ar & BOOST_SERIALIZATION_NVP(upstreamPassingTimes);
    ar & BOOST_SERIALIZATION_NVP(downstreamPassingTimes);
    ar & BOOST_SERIALIZATION_NVP(nextArrivals);
    ar & BOOST_SERIALIZATION_NVP(mapRanksIncomming);
    ar & BOOST_SERIALIZATION_NVP(mapRanksOutGoing);
    ar & BOOST_SERIALIZATION_NVP(pNextEventTuyauEntry);
    ar & BOOST_SERIALIZATION_NVP(dNextEventTime);
    ar & BOOST_SERIALIZATION_NVP(dNextSupplyTime);
}

template void stVarSimBrique::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimBrique::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimBrique::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(mapCoutsMesures);
    ar & BOOST_SERIALIZATION_NVP(mapVeh);
}

template void stVarSimVoie::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimVoie::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimVoie::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(dbNextInstSortie);
}

template void stVarSimConvergent::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimConvergent::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimConvergent::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(dbInstLastPassage);
}

template void stVarSimCrossroads::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimCrossroads::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimCrossroads::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(dbInstLastCrossing);
}

template void stVarSimRegulationBrique::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void stVarSimRegulationBrique::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void stVarSimRegulationBrique::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(lstContextes);
}

template void SimulationSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void SimulationSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void SimulationSnapshot::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_XmlDocTraficTmp);
    ar & BOOST_SERIALIZATION_NVP(m_dbInstSvg);
    ar & BOOST_SERIALIZATION_NVP(m_nInstSvg);
	ar & BOOST_SERIALIZATION_NVP(m_nNbVehCumSvg);
    ar & BOOST_SERIALIZATION_NVP(m_nRandCountSvg);

    ar & BOOST_SERIALIZATION_NVP(m_lstVehSvg);
    ar & BOOST_SERIALIZATION_NVP(m_nLastIdVehSvg);

    ar & BOOST_SERIALIZATION_NVP(m_mapFleet);
    ar & BOOST_SERIALIZATION_NVP(m_mapCDF);
    ar & BOOST_SERIALIZATION_NVP(m_mapTuy);
    ar & BOOST_SERIALIZATION_NVP(m_mapTuyMeso);
    ar & BOOST_SERIALIZATION_NVP(m_mapVoie);
    ar & BOOST_SERIALIZATION_NVP(m_mapOrigine);
    ar & BOOST_SERIALIZATION_NVP(m_mapCapteur);
    ar & BOOST_SERIALIZATION_NVP(m_mapConvergents);
    ar & BOOST_SERIALIZATION_NVP(m_mapCrossroads);
    ar & BOOST_SERIALIZATION_NVP(m_mapRegulations);
    ar & BOOST_SERIALIZATION_NVP(m_mapBrique);
    ar & BOOST_SERIALIZATION_NVP(m_mapMesoNode);
    ar & BOOST_SERIALIZATION_NVP(m_mapZones);

    ar & BOOST_SERIALIZATION_NVP(m_nGestionCapteurPeriodes);

#ifdef USE_SYMUCOM
    ar & BOOST_SERIALIZATION_NVP(m_pITSSnapshot);
#endif // USE_SYMUCOM
}
