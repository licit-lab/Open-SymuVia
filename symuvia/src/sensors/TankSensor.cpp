#include "stdafx.h"
#include "TankSensor.h"

#include "../TraceDocTrafic.h"
#include "../SystemUtil.h"
#include "../tuyau.h"
#include "../vehicule.h"
#include "../voie.h"
#include "../reseau.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>

TankSensor::TankSensor() : AbstractSensor()
{
}

TankSensor::TankSensor(Reseau * pNetwork,
                     SensorsManager * pParentSensorsManager,
                     const char* strNom,
                     const std::vector<Tuyau*> & Tuyaux)
                     : AbstractSensor(strNom),
                     m_Tuyaux(Tuyaux.begin(), Tuyaux.end())
{
    // a priori, un vehicule qui ne touche aucune capteur non interne de la zone pendant un pas de temps ne peut pas en sortir ni y rentrer
    // au cours de ce pas de temps.
    for (size_t i = 0; i < Tuyaux.size(); i++)
    {
        Tuyaux[i]->getLstRelatedSensorsBySensorsManagers()[pParentSensorsManager].push_back(this);
    }
}

TankSensor::~TankSensor()
{
}

std::string TankSensor::GetSensorPeriodXMLNodeName() const
{
    return "CAPTEURS_RESERVOIRS";
}


void TankSensor::CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh)
{
    if (!pVeh->IsMeso())
    {
        double dbPasTemps = pNetwork->GetTimeStep();				// Calcul du pas de temps 'actif' (inférieur au pas de temps défini si le véhicule a été cré au cours du pas de temps)
        if (pVeh->GetInstantEntree() > 0)
        {
            dbPasTemps = dbInstant - pVeh->GetInstantEntree();
        }

        if (pVeh->GetExitInstant() > -1)
            dbPasTemps = pVeh->GetExitInstant() - (dbInstant - pNetwork->GetTimeStep());

        // REMARQUE : on ne vas pas traiter comme celà correctement le cas où un véhicule rentre, sort et rerentre dans une zone au cours du même pas de temps (il faudrait des tronçons très courts !)

        // On regarde si le véhicule fait déjà partie de véhicules déjà détectés dans la zone
        std::map<Vehicule*, std::pair<std::pair<double, double>, bool> >::const_iterator iterVeh = m_vehiclesInsideZone.find(pVeh.get());
        bool bIsInZone = iterVeh != m_vehiclesInsideZone.end();

        // Constitution de la liste ordonnée des tuyaux parcourus par le véhicule au cours du pas de temps
        std::vector<Tuyau*> lstLinks;
        if (pVeh->GetLink(1)) {
            lstLinks.push_back(pVeh->GetLink(1));
        }
        for (unsigned int t = 0; t < pVeh->m_LstUsedLanes.size(); t++)
        {
            lstLinks.push_back((Tuyau*)pVeh->m_LstUsedLanes[t]->GetParent());
        }
        if (pVeh->GetLink(0) && (lstLinks.empty() || pVeh->GetLink(0) != lstLinks.back())) {
            lstLinks.push_back(pVeh->GetLink(0));
        }

        double dbDistanceTotaleParcourue = pVeh->GetDstParcourueEx();

        double dbDistanceParcourueDebutTroncon = 0, dbDistanceParcourueDebutTronconInterne;

        size_t iLink = 0;
        size_t iFirstInternalLink = -1;
        for (size_t iLink = 0; iLink < lstLinks.size(); iLink++)
        {
            Tuyau * pLink = lstLinks.at(iLink);

            if (!bIsInZone)
            {
                // Si on n'est pas dans la zone avant ce lien, on regarde si ce lien l'y fait rentrer :
                if (m_Tuyaux.find(pLink) != m_Tuyaux.end())
                {
                    double dbEntryTime = dbInstant;
                    if (dbDistanceTotaleParcourue > 0)
                    {
                        dbEntryTime -= dbPasTemps * (dbDistanceTotaleParcourue - dbDistanceParcourueDebutTroncon) / dbDistanceTotaleParcourue;
                    }
                    double dbDistanceTravelled = pVeh->GetDstCumulee() - (dbDistanceTotaleParcourue - dbDistanceParcourueDebutTroncon);
                    m_vehiclesInsideZone[pVeh.get()] = std::make_pair(std::make_pair(dbEntryTime, dbDistanceTravelled), !pVeh->GetTuyauxParcourus().empty() && pVeh->GetTuyauxParcourus().front() == pLink);
                    pVeh->getReservoirs().insert(this);
                    bIsInZone = true;
                }
            }
            // Si le véhicule est dans la zone, on regarde s'il n'y est plus à partir de ce lien
            else if (bIsInZone)
            {
                if (pLink->GetBriqueParente())
                {
                    // Cas d'un lien interne : on ne sait pour l'instant pas si on est sorti ou non de la zone.
                    if (iFirstInternalLink == -1)
                    {
                        iFirstInternalLink = iLink;
                        dbDistanceParcourueDebutTronconInterne = dbDistanceParcourueDebutTroncon;
                    }
                }
                else
                {
                    if (m_Tuyaux.find(pLink) == m_Tuyaux.end())
                    {
                        TankSensorData resultLine;
                        resultLine.idVeh = pVeh->GetID();
                        const std::pair<std::pair<double, double>, bool > & entryData = m_vehiclesInsideZone.at(pVeh.get());
                        resultLine.dbEntryTime = entryData.first.first;
                        resultLine.bCreatedInZone = entryData.second;
                        resultLine.bDestroyedInZone = false;

                        double dbExitTime = dbInstant;

                        if (iFirstInternalLink != -1)
                        {
                            iFirstInternalLink = -1;

                            if (dbDistanceTotaleParcourue > 0)
                            {
                                dbExitTime -= dbPasTemps * (dbDistanceTotaleParcourue - dbDistanceParcourueDebutTronconInterne) / dbDistanceTotaleParcourue;
                            }
                            resultLine.dbTraveleldDistance = (pVeh->GetDstCumulee() - (dbDistanceTotaleParcourue - dbDistanceParcourueDebutTronconInterne)) - entryData.first.second;
                        }
                        else
                        {
                            if (dbDistanceTotaleParcourue > 0)
                            {
                                dbExitTime -= dbPasTemps * (dbDistanceTotaleParcourue - dbDistanceParcourueDebutTroncon) / dbDistanceTotaleParcourue;
                            }
                            resultLine.dbTraveleldDistance = (pVeh->GetDstCumulee() - (dbDistanceTotaleParcourue - dbDistanceParcourueDebutTroncon)) - entryData.first.second;
                        }

                        resultLine.dbExitTime = dbExitTime;
                        
                        m_results.push_back(resultLine);
                        // Le véhicule sort de la zone
                        bIsInZone = false;
                        m_vehiclesInsideZone.erase(pVeh.get());
                        pVeh->getReservoirs().erase(this);
                    }
                    else
                    {
                        // le passage dans les tronçons internes ne correspond pas à une sortie de zone : reset.
                        iFirstInternalLink = -1;
                    }
                }
            }

            if (iLink == 0)
            {
                if (!pVeh->GetLink(1))
                {
                    dbDistanceParcourueDebutTroncon = pLink->GetLength();
                }
                else
                {
                    dbDistanceParcourueDebutTroncon = pLink->GetLength() - pVeh->GetPos(1);
                }
            }
            else
            {
                dbDistanceParcourueDebutTroncon += pLink->GetLength();
            }
        }

        if (iFirstInternalLink != -1)
        {
            // le véhicule termine le pas de temps sur un tronçon interne. On regarde du coup
            // le prochain tuyaux non interne de l'itinéraire pour savoir si on et sorti ou non de la zone !
            Tuyau * pNextNonInternalLink = pVeh->CalculNextTuyau(lstLinks.back(), dbInstant);
            if (m_Tuyaux.find(pNextNonInternalLink) == m_Tuyaux.end())
            {
                // le véhicule était donc sorti de la zone au tronçon interne n°iFirstInternalLink !
                TankSensorData resultLine;
                resultLine.idVeh = pVeh->GetID();
                const std::pair<std::pair<double, double>, bool > & entryData = m_vehiclesInsideZone.at(pVeh.get());
                resultLine.dbEntryTime = entryData.first.first;
                resultLine.bCreatedInZone = entryData.second;
                resultLine.bDestroyedInZone = false;

                double dbExitTime = dbInstant;
                if (dbDistanceTotaleParcourue > 0)
                {
                    dbExitTime -= dbPasTemps * (dbDistanceTotaleParcourue - dbDistanceParcourueDebutTronconInterne) / dbDistanceTotaleParcourue;
                }
                resultLine.dbTraveleldDistance = (pVeh->GetDstCumulee() - (dbDistanceTotaleParcourue - dbDistanceParcourueDebutTronconInterne)) - entryData.first.second;
                resultLine.dbExitTime = dbExitTime;

                m_results.push_back(resultLine);

                m_vehiclesInsideZone.erase(pVeh.get());
                pVeh->getReservoirs().erase(this);
            }
        }
    }
}

void TankSensor::AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink)
{
    // NON TESTE

    // a priori, on ne tient pas compte du cas où on passe ici car un tronçon meso devient micro (on ne recherche que des évènements d'entrée ou sortie
    // sur les tronçons entiers.
    if (pDownstreamLink)
    {
        std::map<Vehicule*, std::pair<std::pair<double, double>, bool> >::const_iterator iterVeh = m_vehiclesInsideZone.find(pVeh);
        bool bIsInZone = iterVeh != m_vehiclesInsideZone.end();
        if (bIsInZone && m_Tuyaux.find(pDownstreamLink) == m_Tuyaux.end())
        {
            // Cas de la sortie de zone :
            TankSensorData resultLine;
            resultLine.idVeh = pVeh->GetID();
            const std::pair<std::pair<double, double>, bool> & entryData = m_vehiclesInsideZone.at(pVeh);
            resultLine.dbEntryTime = entryData.first.first;
            resultLine.dbExitTime = dbInstant;
            resultLine.dbTraveleldDistance = pVeh->GetDstCumulee() - entryData.first.second;
            resultLine.bCreatedInZone = entryData.second;
            resultLine.bDestroyedInZone = false;

            m_results.push_back(resultLine);

            m_vehiclesInsideZone.erase(pVeh);
            pVeh->getReservoirs().erase(this);
        }
        else if (!bIsInZone && m_Tuyaux.find(pDownstreamLink) != m_Tuyaux.end())
        {
            // Cas de l'entrée en zone :
            double dbEntryTime = dbInstant;
            double dbDistanceTravelled = pVeh->GetDstCumulee(); // Approx ?
            m_vehiclesInsideZone[pVeh] = std::make_pair(std::make_pair(dbEntryTime, dbDistanceTravelled), !pVeh->GetTuyauxParcourus().empty() && pVeh->GetTuyauxParcourus().front() == pLink);
            pVeh->getReservoirs().insert(this);
        }
    }
}

void TankSensor::WriteDef(DocTrafic* pDocTrafic)
{
}

void TankSensor::VehicleDestroyed(Vehicule * pVeh)
{
    std::map<Vehicule*, std::pair<std::pair<double, double>, bool> >::const_iterator iter = m_vehiclesInsideZone.find(pVeh);
    if (iter != m_vehiclesInsideZone.end())
    {
        TankSensorData resultLine;
        resultLine.idVeh = pVeh->GetID();
        const std::pair<std::pair<double, double>, bool > & entryData = m_vehiclesInsideZone.at(pVeh);
        resultLine.dbEntryTime = entryData.first.first;
        resultLine.dbExitTime = pVeh->GetExitInstant();
        resultLine.bCreatedInZone = entryData.second;
        resultLine.bDestroyedInZone = true;
        resultLine.dbTraveleldDistance = pVeh->GetDstCumulee() - entryData.first.second;

        m_results.push_back(resultLine);
        
        m_vehiclesInsideZone.erase(pVeh);
    }
}

void TankSensor::Write(double dbInstant, Reseau * pReseau, double dbPeriodeAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutput)
{
    if(!docTrafics.empty())
    {
        // écriture
        for (std::deque<TraceDocTrafic* >::const_iterator itDocTraf = docTrafics.begin(); itDocTraf != docTrafics.end(); itDocTraf++)
        {
            DocTrafic::PCAPTEUR pSensorNode = (*itDocTraf)->AddInfoCapteurReservoir(m_strNom);

            for (size_t i = 0; i < m_results.size(); i++)
            {
                const TankSensorData & result = m_results.at(i);

                (*itDocTraf)->AddTraverseeCpt(pSensorNode,             
                    SystemUtil::ToString(result.idVeh),                     // Identifiant véhicule
                    SystemUtil::ToString(3, result.dbEntryTime),            // Instant d'entrée
                    SystemUtil::ToString(3, result.dbExitTime),             // Instant de sortie
                    SystemUtil::ToString(3, result.dbTraveleldDistance),    // Distance parcourue
                    SystemUtil::ToString(result.bCreatedInZone),            // vrai si le véhicule a été créé dans la zone
                    SystemUtil::ToString(result.bDestroyedInZone)           // vrai si le véhicule a été détruit dans la zone
                    );
            }  

            // Traiement des véhicules qui ne sont pas sortis de la zone !
            for (std::map<Vehicule*, std::pair<std::pair<double, double>, bool> >::const_iterator iter = m_vehiclesInsideZone.begin(); iter != m_vehiclesInsideZone.end(); ++iter)
            {
                double dbTravelledDistance = iter->first->GetDstCumulee() - iter->second.first.second;
                (*itDocTraf)->AddTraverseeCpt(pSensorNode,
                    SystemUtil::ToString(iter->first->GetID()),             // Identifiant véhicule
                    SystemUtil::ToString(3, iter->second.first.first),      // Instant d'entrée
                    SystemUtil::ToString(3, -1),                            // Instant de sortie
                    SystemUtil::ToString(3, dbTravelledDistance),           // Distance parcourue
                    SystemUtil::ToString(iter->second.second),              // vrai si le véhicule a été créé dans la zone
                    SystemUtil::ToString(false)                             // vrai si le véhicule a été détruit dans la zone (non puisque pas encore détruit)
                    );
            }
        }
    }
}

void TankSensor::PrepareNextPeriod()
{
}


AbstractSensorSnapshot * TankSensor::TakeSnapshot()
{
    TankSensorSnapshot * pResult = new TankSensorSnapshot();
    pResult->results = m_results;
    for (std::map<Vehicule*, std::pair<std::pair<double, double>, bool> >::iterator iter = m_vehiclesInsideZone.begin(); iter != m_vehiclesInsideZone.end(); ++iter)
    {
        pResult->vehiclesInsideZone[iter->first->GetID()] = iter->second;
    }
    return pResult;
}

void TankSensor::Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup)
{
    TankSensorSnapshot * pSnapshot = (TankSensorSnapshot*)backup;
    m_results = pSnapshot->results;
    m_vehiclesInsideZone.clear();
    for (std::map<int, std::pair<std::pair<double, double>, bool> >::iterator iter = pSnapshot->vehiclesInsideZone.begin(); iter != pSnapshot->vehiclesInsideZone.end(); ++iter)
    {
        m_vehiclesInsideZone[pNetwork->GetVehiculeFromID(iter->first).get()] = iter->second;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template void TankSensorData::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TankSensorData::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TankSensorData::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(idVeh);
    ar & BOOST_SERIALIZATION_NVP(dbEntryTime);
    ar & BOOST_SERIALIZATION_NVP(dbExitTime);
    ar & BOOST_SERIALIZATION_NVP(dbTraveleldDistance);
    ar & BOOST_SERIALIZATION_NVP(bCreatedInZone);
    ar & BOOST_SERIALIZATION_NVP(bDestroyedInZone);
}

template void TankSensorSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TankSensorSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TankSensorSnapshot::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensorSnapshot);
    
    ar & BOOST_SERIALIZATION_NVP(vehiclesInsideZone);
    ar & BOOST_SERIALIZATION_NVP(results);
}

template void TankSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void TankSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void TankSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensor);
    ar & BOOST_SERIALIZATION_NVP(m_Tuyaux);
   
    ar & BOOST_SERIALIZATION_NVP(m_vehiclesInsideZone);
    ar & BOOST_SERIALIZATION_NVP(m_results);
}

