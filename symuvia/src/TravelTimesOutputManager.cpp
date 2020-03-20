#include "stdafx.h"
#include "TravelTimesOutputManager.h"

#include "reseau.h"
#include "tuyau.h"
#include "sensors/SensorsManager.h"
#include "sensors/EdieSensor.h"
#include "sensors/MFDSensor.h"
#include "SimulationSnapshot.h"

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <boost/math/special_functions/round.hpp>


TravelTimesOutputManager::TravelTimesOutputManager() :
m_TravelTimes(rapidjson::kArrayType)
{
    m_pNetwork = NULL;
    m_pSensorsManager = NULL;
    m_nPeriode = 0;

    m_pCurrentSimulationSnapshot = NULL;
}

TravelTimesOutputManager::TravelTimesOutputManager(Reseau * pNetwork) :
m_TravelTimes(rapidjson::kArrayType)
{
    m_pNetwork = pNetwork;
    m_nPeriode = 0;

    // construction des capteurs : un par tronçon non interne
    m_pSensorsManager = new SensorsManager(pNetwork, pNetwork->GetTravelTimesOutputPeriod(), 0);
    for (size_t iLink = 0; iLink < m_pNetwork->m_LstTuyaux.size(); iLink++)
    {
        Tuyau * pLink = m_pNetwork->m_LstTuyaux[iLink];
        if (!pLink->GetBriqueParente())
        {
            m_pSensorsManager->AddCapteurMFD(const_cast<char*>(pLink->GetLabel().c_str()),
                std::vector<Tuyau*>(1, pLink), std::vector<double>(1, 0.0), std::vector<double>(1, pLink->GetLength()),
                std::vector<int>(1, EdieSensor::TT_Link), false);
        }
    }

    m_pCurrentSimulationSnapshot = NULL;
}

TravelTimesOutputManager::~TravelTimesOutputManager()
{
    // Pour la dernière periode tronquée le cas échéant
    CalculInfoCapteurs(m_pNetwork->GetInstSim() * m_pNetwork->GetTimeStep(), true);

    converge(m_pCurrentSimulationSnapshot);

    std::ofstream ofs(m_pNetwork->GetPrefixOutputFiles() + "_traveltimes" + m_pNetwork->GetSuffixOutputFiles() +  ".json");
    rapidjson::OStreamWrapper osw(ofs);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
    m_TravelTimes.Accept(writer);

    // En principe inutile mais bug de fichier tronqué remonté dans certains cas...
    osw.Flush();
    ofs.close();

    if (m_pSensorsManager)
    {
        delete m_pSensorsManager;
    }
}


void TravelTimesOutputManager::Restore(TravelTimesOutputManagerSnapshot* pSnapshot)
{
    m_nPeriode = pSnapshot->nPeriode;
    m_mapTravelTimes = pSnapshot->mapTravelTimes;
    for (size_t iSensor = 0; iSensor < m_pSensorsManager->m_LstCapteurs.size(); iSensor++)
    {
        m_pSensorsManager->m_LstCapteurs[iSensor]->Restore(m_pNetwork, pSnapshot->sensorsSnapshots[iSensor]);
    }
}

TravelTimesOutputManagerSnapshot * TravelTimesOutputManager::TakeSnapshot()
{
    TravelTimesOutputManagerSnapshot * pSnapshot = new TravelTimesOutputManagerSnapshot();
    pSnapshot->nPeriode = m_nPeriode;
    pSnapshot->mapTravelTimes = m_mapTravelTimes;
    for (size_t iSensor = 0; iSensor < m_pSensorsManager->m_LstCapteurs.size(); iSensor++)
    {
        pSnapshot->sensorsSnapshots.push_back(m_pSensorsManager->m_LstCapteurs[iSensor]->TakeSnapshot());
    }
    return pSnapshot;
}

void TravelTimesOutputManager::AddMeasuredTravelTime(Tuyau * pLink, double dbMeasuredTravelTime)
{
    std::pair<int, double> & linkData = m_mapTravelTimes[pLink];
    linkData.first += 1;
    linkData.second += dbMeasuredTravelTime;
}



void TravelTimesOutputManager::CalculInfoCapteurs(double dbInstant, bool bTerminate)
{
    bool bNewPeriode = false;
    double dbInstNewPeriode = 0;

    // Mise à jour de la période (sauf sur terminate si ca tombe pile sur un instant de fin de période)
    if ((bTerminate && (m_nPeriode > 0 && (((m_nPeriode - 1)*m_pNetwork->GetTravelTimesOutputPeriod()) != m_pNetwork->GetInstSim()*m_pNetwork->GetTimeStep()))) || // Condition pour considérer lors de la terminaison qu'une nouvelle periode partielle doit être écrite
        (!bTerminate && (dbInstant - (m_nPeriode * m_pNetwork->GetTravelTimesOutputPeriod()) >= 0))) // Condition pour passer à la période suivante en cours de simulation
    {
        // On attaque une nouvelle période
        bNewPeriode = true;

        // Début de la nouvelle période
        // (pour le pas de temps courant, il peut y avoir des franchissement de capteurs
        // à considérer dans la nouvelle période...)
        dbInstNewPeriode = m_nPeriode * m_pNetwork->GetTravelTimesOutputPeriod();
    }

    if (!bTerminate)
    {
        for (size_t iVeh = 0; iVeh < m_pNetwork->m_LstVehicles.size(); iVeh++)
        {
            boost::shared_ptr<Vehicule> pVeh = m_pNetwork->m_LstVehicles[iVeh];

            // Récupération des capteurs potentiellement impactés par le véhicule :
            std::set<AbstractSensor*> potentiallyImpactedSensors = pVeh->GetPotentiallyImpactedSensors(m_pSensorsManager);
            // Traitement des capteurs potentiellement impactés
            for (std::set<AbstractSensor*>::iterator iterSensor = potentiallyImpactedSensors.begin(); iterSensor != potentiallyImpactedSensors.end(); ++iterSensor)
            {
                (*iterSensor)->CalculInfoCapteur(m_pNetwork, dbInstant, bNewPeriode, dbInstNewPeriode, pVeh);
            }
        }
    }

    if (bNewPeriode)
    {
        // écriture des résultats
        if (m_nPeriode > 0)
        {
            double dbDeb = (m_nPeriode - 1)*m_pNetwork->GetTravelTimesOutputPeriod();

            rapidjson::Document * pJSONDoc = m_pCurrentSimulationSnapshot ? m_pCurrentSimulationSnapshot->m_TravelTimesTmp : &m_TravelTimes;
            rapidjson::Document::AllocatorType& allocator = pJSONDoc->GetAllocator();

            rapidjson::Value periodJson(rapidjson::kObjectType);

            rapidjson::Value strSymuStartValue(rapidjson::kStringType);
            STime symuStart = m_pNetwork->GetSimuStartTime() + STimeSpan((int)boost::math::round(dbDeb));
            std::string symuStartStr = symuStart.ToString();
            strSymuStartValue.SetString(symuStartStr.c_str(), (int)symuStartStr.length(), allocator);
            periodJson.AddMember("symuStart", strSymuStartValue, allocator);

            rapidjson::Value strSymuEndValue(rapidjson::kStringType);
            STime symuEnd = m_pNetwork->GetSimuStartTime() + STimeSpan((int)boost::math::round(dbInstant));
            std::string symuEndStr = symuEnd.ToString();
            strSymuEndValue.SetString(symuEndStr.c_str(), (int)symuEndStr.length(), allocator);
            periodJson.AddMember("symuEnd", strSymuEndValue, allocator);

            rapidjson::Value sectionsJson(rapidjson::kArrayType);

            for (size_t j = 0; j < m_pSensorsManager->m_LstCapteurs.size(); j++)
            {
                rapidjson::Value sectionJson(rapidjson::kObjectType);
                EdieSensor * pEdie = (EdieSensor*)((MFDSensor*)m_pSensorsManager->m_LstCapteurs[j])->GetLstSensors().front();

                rapidjson::Value strLinkValue(rapidjson::kStringType);
                strLinkValue.SetString(pEdie->GetTuyau()->GetLabel().c_str(), (int)pEdie->GetTuyau()->GetLabel().length(), allocator);
                sectionJson.AddMember("id", strLinkValue, allocator);

                double dbTD = pEdie->GetData().GetTotalTravelledDistance();
                double dbTT = pEdie->GetData().GetTotalTravelledTime();
                double dbSpeed;
                if (dbTT > 0)
                {
                    dbSpeed = dbTD / dbTT;
                }
                else
                {
                    double dbEmptytravelTime = ((TuyauMicro*)pEdie->GetTuyau())->GetCoutAVide(dbInstant, NULL, true);
                    if (dbEmptytravelTime > 0)
                    {
                        dbSpeed = pEdie->GetSensorLength() / dbEmptytravelTime;
                    }
                    else
                    {
                        dbSpeed = 0;
                    }
                }
                sectionJson.AddMember("speed", dbSpeed, allocator);
                

                double dbDuration = 0;
                std::map<Tuyau*, std::pair<int, double> >::const_iterator iter = m_mapTravelTimes.find(pEdie->GetTuyau());
                if (iter != m_mapTravelTimes.end())
                {
                    dbDuration = iter->second.second / iter->second.first;
                }
                else
                {
                    dbDuration = ((TuyauMicro*)pEdie->GetTuyau())->GetCoutAVide(dbInstant, NULL, true);
                }
                sectionJson.AddMember("duration", dbDuration, allocator);


                sectionsJson.PushBack(sectionJson, allocator);
            }

            periodJson.AddMember("sections", sectionsJson, allocator);

            pJSONDoc->PushBack(periodJson, allocator);
        }

        // Préparation de la période suivante
        if (!bTerminate)
        {
            for (size_t j = 0; j < m_pSensorsManager->m_LstCapteurs.size(); j++)
            {
                m_pSensorsManager->m_LstCapteurs[j]->PrepareNextPeriod();
            }
            m_mapTravelTimes.clear();
            m_nPeriode++;
        }
    }
}

// passage en mode d'écriture dans fichiers temporaires (affectation dynamique)
void TravelTimesOutputManager::doUseTempFiles(SimulationSnapshot * pSimulationSnapshot)
{
    m_pCurrentSimulationSnapshot = pSimulationSnapshot;

    assert(m_pCurrentSimulationSnapshot->m_TravelTimesTmp == NULL);

    m_pCurrentSimulationSnapshot->m_TravelTimesTmp = new rapidjson::Document(rapidjson::kArrayType);
}

// validation de la dernière période d'affectation et recopie des fichiers temporaires dans les fichiers finaux
void TravelTimesOutputManager::converge(SimulationSnapshot * pSimulationSnapshot)
{
    if (pSimulationSnapshot && pSimulationSnapshot->m_TravelTimesTmp)
    {
        // Ajout des periodes du document temporaire dans le document final :
        for (rapidjson::SizeType i = 0; i < pSimulationSnapshot->m_TravelTimesTmp->Size(); i++)
        {
            m_TravelTimes.PushBack(rapidjson::Value(pSimulationSnapshot->m_TravelTimesTmp->operator[](i), m_TravelTimes.GetAllocator()), m_TravelTimes.GetAllocator());
        }
    }
    m_pCurrentSimulationSnapshot = NULL;
}
