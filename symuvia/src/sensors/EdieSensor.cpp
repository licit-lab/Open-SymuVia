#include "stdafx.h"
#include "EdieSensor.h"

#include "../SystemUtil.h"
#include "../TraceDocTrafic.h"
#include "../vehicule.h"
#include "../reseau.h"
#include "../voie.h"
#include "../tuyau.h"
#include "MFDSensor.h"

#include "../usage/Trip.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>

#include <iostream>

EdieSensor::EdieSensor() : LongitudinalSensor()
{
    eTuyauType = (int) TT_Link;
    m_pParentMFD = NULL;
}

EdieSensor::EdieSensor(const std::string & strNom, Tuyau * pTuyau, double startPos, double endPos, int tuyauType, MFDSensor * pParent)
    : LongitudinalSensor(strNom, pTuyau, startPos, endPos)
{
    eTuyauType = tuyauType;
    m_pParentMFD = pParent;

    // Initialisation d'un r�sultat par voie (+ 1 pour l'ensemble des voies)
	int countNbVoies = pTuyau->getNbVoiesDis();
    

    data.workingData.dbTotalTravelledTime.resize(countNbVoies + 1);
	data.workingData.dbTotalTravelledDistance.resize(countNbVoies + 1);
    TypeVehicule * pTypeVeh;
    for(int i = 0; i < countNbVoies+1; i++)
    {
        for (size_t iTypeVeh = 0; iTypeVeh < pTuyau->GetReseau()->m_LstTypesVehicule.size(); iTypeVeh++)
        {
            pTypeVeh = pTuyau->GetReseau()->m_LstTypesVehicule[iTypeVeh];
			data.workingData.dbTotalTravelledTime[i][pTypeVeh] = 0.0;
			data.workingData.dbTotalTravelledDistance[i][pTypeVeh] = 0.0;
        }
    }

	for (size_t iTypeVeh = 0; iTypeVeh < pTuyau->GetReseau()->m_LstTypesVehicule.size(); iTypeVeh++)
		data.workingData.nNumberOfRemovalVehicles[pTypeVeh] = 0;
}

EdieSensor::~EdieSensor()
{
}

MFDSensor * EdieSensor::GetParentMFD() const
{
	return m_pParentMFD;
}

void EdieSensor::CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh)
{
    bool bFranchissement;
	if (!pVeh->IsMeso())
	{
		// on teste le franchissement potentiel du capteur (le v�hicule passe-t-il sur le tron�on du capteur pendant le PDT ? et si oui, entre quelles positions ?)
		bFranchissement = false;
		double dbPosDebut, dbPosFin;
		int numVoie = 0;
		if (m_pTuyau == pVeh->GetLink(1))
		{
			dbPosDebut = pVeh->GetPos(1);
			if (pVeh->GetVoie(1))
			{
				numVoie = pVeh->GetVoie(1)->GetNum();
			}
			else if (pVeh->GetVoie(0))
			{
				numVoie = std::min<int>(pVeh->GetVoie(0)->GetNum(), m_pTuyau->getNbVoiesDis() - 1);
			}
			if (pVeh->GetLink(0) != m_pTuyau)
			{
				dbPosFin = m_pTuyau->GetLength();
			}
			else
			{
				dbPosFin = pVeh->GetPos(0);
			}
			bFranchissement = true;
		}
		else if (m_pTuyau == pVeh->GetLink(0))
		{
			dbPosFin = pVeh->GetPos(0);
			if (pVeh->GetVoie(0))
			{
				numVoie = pVeh->GetVoie(0)->GetNum();
			}
			else if (pVeh->GetVoie(1))
			{
				numVoie = std::min<int>(pVeh->GetVoie(1)->GetNum(), m_pTuyau->getNbVoiesDis() - 1);
			}
			if (pVeh->GetLink(1) != m_pTuyau)
			{
				dbPosDebut = 0;
			}
			else
			{
				dbPosDebut = pVeh->GetPos(1);
			}
			bFranchissement = true;
		}
		else
		{
			// test des voies emprunt�es au cas o� le v�hicule "saute" le tron�on pendant le pas de temps
			for (unsigned int t = 0; t < pVeh->m_LstUsedLanes.size(); t++)
			{
				if (m_pTuyau == pVeh->m_LstUsedLanes[t]->GetParent())
				{
					numVoie = pVeh->m_LstUsedLanes[t]->GetNum();
					dbPosDebut = 0;
					dbPosFin = m_pTuyau->GetLength();
					bFranchissement = true;
					break;
				}
			}
		}

		if (bFranchissement)
		{
			double dbDistance = std::min<double>(dbPositionFin, dbPosFin) - std::max<double>(dbPositionDebut, dbPosDebut);

			if (dbDistance >= 0)
			{
				// remaque : dans le cas des capteurs MFD internes aux briques, on ignore les v�hicules qui ne proviennent pas de la zone MFD.
				bool bIgnoreVehicle = false;
				if (eTuyauType != TT_Link && m_pParentMFD)
				{
					// d�termination du tron�on amont du v�hicule � la brique
					const std::vector<Tuyau*> & tuyauxParcourus = pVeh->GetTuyauxParcourus();
					Tuyau * pUpstreamLink = NULL;
					// on parcours � rebours cette liste. Le premier tron�on non interne est le tron�on amont de la brique
					for (int iTuy = (int)tuyauxParcourus.size() - 1; iTuy >= 0 && !pUpstreamLink; iTuy--)
					{
						if (tuyauxParcourus[iTuy]->GetBriqueAval() == m_pTuyau->GetBriqueParente())
						{
							pUpstreamLink = tuyauxParcourus[iTuy];
						}
					}
					if (pUpstreamLink)
					{
						bool bTuyAmontIsInMFD = false;
						for (size_t iTuy = 0; iTuy < m_pParentMFD->GetLstSensors().size() && !bTuyAmontIsInMFD; iTuy++)
						{
							if (m_pParentMFD->GetLstSensors()[iTuy]->GetTuyau() == pUpstreamLink)
							{
								bTuyAmontIsInMFD = true;
							}
						}
						bIgnoreVehicle = !bTuyAmontIsInMFD;
					}
					else
					{
						///assert(false); // c'est possible, �a ?? un v�hicule cr�� dans un CAF ? A reprendre pourquoi on arrive l� ?
						bIgnoreVehicle = true;	
					}
				}
				// Fin de la v�rification du tron�on amont � la brique

				if (bIgnoreVehicle)
				{
					return;
				}

				// Distance parcourue pour la voie du v�hicule
				data.workingData.dbTotalTravelledDistance[numVoie][pVeh->GetType()] += dbDistance;
				// distance parcourue pour l' ensemble du tron�on
				data.workingData.dbTotalTravelledDistance[m_pTuyau->getNb_voies()][pVeh->GetType()] += dbDistance;

				// temps pass� pour la voie du v�hicule
				double dbPasTemps = pNetwork->GetTimeStep();				// Calcul du pas de temps 'actif' (inf�rieur au pas de temps d�fini si le v�hicule a �t� cr� au cours du pas de temps)
				if (pVeh->GetInstantEntree() > 0)
				{
					dbPasTemps = dbInstant - pVeh->GetInstantEntree(), pNetwork->GetTimeStep();
				}

				if (pVeh->GetExitInstant() > -1)
					dbPasTemps = pVeh->GetExitInstant() - (dbInstant - pNetwork->GetTimeStep());

				// Pour �viter la division par zero dans le cas rare ou le v�hicule est sorti pile au d�but du pas de temps
				if (dbPasTemps > 0)
				{
					double dbVitMoy = pVeh->GetDstParcourueEx() / dbPasTemps;		// Calcul de la vitesse moyenne

					double dbTemps;
					if (dbVitMoy != 0)
					{
						dbTemps = std::min<double>(dbDistance / dbVitMoy, pNetwork->GetTimeStep());		// Attention:  GetDstParcourueEx may be incorrect if this is a bus which departs (then min ...)
					}
					else
					{
						// tout le pas de temps si arr�t� dans le capteur
						if (dbPosFin <= dbPositionFin && dbPosFin >= dbPositionDebut)
						{
							dbTemps = pNetwork->GetTimeStep();
						}
						else
						{
							dbTemps = 0.0;
						}
					}
					data.workingData.dbTotalTravelledTime[numVoie][pVeh->GetType()] += dbTemps;
					// temps pass� sur toutes les voies :
					data.workingData.dbTotalTravelledTime[m_pTuyau->getNb_voies()][pVeh->GetType()] += dbTemps;

					if( std::find(data.workingData.vectofvehicleIDs.begin(), data.workingData.vectofvehicleIDs.end(), pVeh->GetID() )== data.workingData.vectofvehicleIDs.end())
						data.workingData.vectofvehicleIDs.push_back(pVeh->GetID());
				}
			}
		}
	}

	// CP des v�hicules qui disparaissent durant le pas de temps
	if (!pVeh->GetLink(0))
	{
		if (pVeh->GetTrip()->GetFinalDestination()->GetInputPosition().IsLink())
		{
			if ( pVeh->GetTrip()->GetFinalDestination()->GetInputPosition().GetLink() == m_pTuyau)
			{
				data.workingData.nNumberOfRemovalVehicles[pVeh->GetType()] += 1;
			}
		}
	}
}

void EdieSensor::AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink)
{
    if(m_pTuyau == pLink)
    {
        double dbDistance = std::min<double>(dbPositionFin,dbLengthInLink)-std::max<double>(dbPositionDebut,0.0);

        if(dbDistance >= 0)
        {
            // Pas de voie en meso : on laisse tomber les r�sultats par voie ici
            //int numVoie = std::min<int>( std::max<int>( pVeh->GetNumVoieMeso(),0), pLink->getNb_voies()-1);

            // distance parcourue pour la voie du v�hicule
            //data.dbTotalTravelledDistance[numVoie] += dbDistance;
            // distance parcourue pour l' ensemble du tron�on
			data.workingData.dbTotalTravelledDistance[m_pTuyau->getNb_voies()][pVeh->GetType()] += dbDistance;

            double dbTemps = 0;

            if(eTuyauType == TT_Link)
            {
                // Cas du capteur Edie classique
                double dbInputTime = pVeh->GetTimeCode(pLink, Vehicule::TC_INCOMMING);
                assert(dbInputTime != DBL_MAX); // en principe le temps d'entr�e du v�hicule sur le tron�on doit �tre d�fini !
                dbTemps = dbInstant - dbInputTime;
            }
            else
            {
                // Cas de la prise en compte d'un mouvement interne pour un capteur MFD et un v�hicule m�so :
                // on fait au plus simple en utilisant le temps de parcours � la vitesse nominale sur le tuyau
                //dbTemps = m_pTuyau->GetLength() / std::min<double>( pVeh->GetVitMax(), m_pTuyau->GetVitReg() );
                // EN FAIT, suite aux derni�re smodifs pour prendre en compte le temps pass� dans les briques dans le mod�le meso,
                // il suffit ici d'ajouter la distance, mais pas le temps (il est inclus dans le tuyau amont !)
                dbTemps = 0;
            }

            //data.dbAcumulation[numVoie] += dbTemps;
            // temps pass� pour toutes les voies :
			data.workingData.dbTotalTravelledTime[m_pTuyau->getNb_voies()][pVeh->GetType()] += dbTemps;
        }

        // gestion ici du cas o� la connexion aval est une brique franchie par le v�hicule en mode meso,
        // pour laquelle il faut prendre en compte les capteurs sur les tuyaux internes emprunt�s
        if(pLink->GetBriqueAval() && dbLengthInLink == DBL_MAX && m_pParentMFD && pDownstreamLink)
        {
            // D�termination du chemin interne du v�hicule
            std::vector<Tuyau*> lstTuyauxInternes;
            if(pLink->GetBriqueAval()->GetTuyauxInternes(pLink, pDownstreamLink, lstTuyauxInternes))
            {
                const std::vector<EdieSensor*> & lstEdieSensor = m_pParentMFD->GetLstSensors();
                for(size_t iSensor = 0; iSensor < lstEdieSensor.size(); iSensor++)
                {
                    EdieSensor * pEdieSensor = lstEdieSensor[iSensor];
                    if(std::find(lstTuyauxInternes.begin(), lstTuyauxInternes.end(), pEdieSensor->GetTuyau()) != lstTuyauxInternes.end())
                    {
                        pEdieSensor->AddMesoVehicle(dbInstant, pVeh, pEdieSensor->GetTuyau(), NULL);
                    }
                }
            }
        }
    }
}

void EdieSensor::WriteDef(DocTrafic* pDocTrafic)
{
    Point ptCoord, ptCoordFin;
    CalcCoordFromPos(GetTuyau(), GetStartPosition(), ptCoord);
    CalcCoordFromPos(GetTuyau(), GetEndPosition(), ptCoordFin);
    pDocTrafic->AddDefCapteurEdie( GetUnifiedID(), GetTuyau()->GetLabel(), GetStartPosition(), ptCoord, GetEndPosition(), ptCoordFin);
}

void EdieSensor::Write(double dbInstant, Reseau * pReseau, double dbPeriodeAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutput)
{
    if(!docTrafics.empty())
    {
        double dbLongueurCpt = GetSensorLength();
		double dbNbVoie = (double)data.workingData.dbTotalTravelledTime.size() - 1;

        std::string strConcentration = "";
        std::string strDebit = "";

		double nbVoie = (double)data.workingData.dbTotalTravelledTime.size() - 1;
		for (size_t k = 0; k < data.workingData.dbTotalTravelledTime.size() - 1; k++)
        {
			strConcentration = strConcentration + SystemUtil::ToString(3, data.workingData.GetTravelledTimeForLane(k) / (dbPeriodeAgregation*dbLongueurCpt)) + " ";
			strDebit = strDebit + SystemUtil::ToString(3, data.workingData.GetTravelledDistanceForLane(k) / (dbPeriodeAgregation*dbLongueurCpt)) + " ";
        }

        std::deque<TraceDocTrafic* >::const_iterator itDocTraf;
        for( itDocTraf = docTrafics.begin(); itDocTraf != docTrafics.end(); itDocTraf++ )
        {
            // �criture
            (*itDocTraf)->AddInfoCapteurEdie(m_strNom,   // identifiant capteur
				SystemUtil::ToString(3, data.workingData.GetTotalTravelledTime() / (dbPeriodeAgregation*dbLongueurCpt*nbVoie)),  // Concentration globale
				SystemUtil::ToString(3, data.workingData.GetTotalTravelledDistance() / (dbPeriodeAgregation*dbLongueurCpt*nbVoie)),// D�bit global
                strConcentration,                                                                                                                   // Concentration voie par voie
                strDebit);                                                                                                                          // D�bit voie par voie
        }
    }
}

void EdieSensor::PrepareNextPeriod()
{
	data.previousPeriodData = data.workingData;
    int countNbVoies = m_pTuyau->getNbVoiesDis();
    
    TypeVehicule * pTypeVeh;
    for (int i = 0; i < countNbVoies + 1; i++)
    {
        for (size_t iTypeVeh = 0; iTypeVeh < m_pTuyau->GetReseau()->m_LstTypesVehicule.size(); iTypeVeh++)
        {
            pTypeVeh = m_pTuyau->GetReseau()->m_LstTypesVehicule[iTypeVeh];
			data.workingData.dbTotalTravelledTime[i][pTypeVeh] = 0.0;
			data.workingData.dbTotalTravelledDistance[i][pTypeVeh] = 0.0;
        }
    }

	for (size_t iTypeVeh = 0; iTypeVeh < m_pTuyau->GetReseau()->m_LstTypesVehicule.size(); iTypeVeh++)
		data.workingData.nNumberOfRemovalVehicles[pTypeVeh] = 0;

	data.workingData.vectofvehicleIDs.clear();
}


EdieSensorData & EdieSensor::GetData()
{
    return data.workingData;
}

const EdieSensorData & EdieSensor::GetPreviousData() const
{
	return data.previousPeriodData;
}

int EdieSensor::GetTuyauType()
{
    return eTuyauType;
}

AbstractSensorSnapshot * EdieSensor::TakeSnapshot()
{
	EdieSensorSnapshot * pResult = new EdieSensorSnapshot();
	*pResult = data;
    return pResult;
}

void EdieSensor::Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup)
{
	EdieSensorSnapshot * pSnapshot = (EdieSensorSnapshot*)backup;
	data = *pSnapshot;
}

double EdieSensorData::GetTotalTravelledDistance() const
{
    std::map<TypeVehicule*, double> allLanesDist = dbTotalTravelledDistance.back();
    double dbDist = 0;
    for (std::map<TypeVehicule*, double>::const_iterator iter = allLanesDist.begin(); iter != allLanesDist.end(); ++iter)
    {
        dbDist += iter->second;
    }
    return dbDist;
}

double EdieSensorData::GetTotalTravelledTime() const
{
    std::map<TypeVehicule*, double> allLanesTime = dbTotalTravelledTime.back();
    double dbTime = 0;
    for (std::map<TypeVehicule*, double>::const_iterator iter = allLanesTime.begin(); iter != allLanesTime.end(); ++iter)
    {
        dbTime += iter->second;
    }
    return dbTime;
}

double EdieSensorData::GetTravelledTimeForLane(size_t k) const
{
    std::map<TypeVehicule*, double> laneTime = dbTotalTravelledTime[k];
    double dbTime = 0;
    for (std::map<TypeVehicule*, double>::const_iterator iter = laneTime.begin(); iter != laneTime.end(); ++iter)
    {
        dbTime += iter->second;
    }
    return dbTime;
}

double EdieSensorData::GetTravelledDistanceForLane(size_t k) const
{
    std::map<TypeVehicule*, double> laneDist = dbTotalTravelledDistance[k];
    double dbDist = 0;
    for (std::map<TypeVehicule*, double>::const_iterator iter = laneDist.begin(); iter != laneDist.end(); ++iter)
    {
        dbDist += iter->second;
    }
    return dbDist;
}

int EdieSensorData::GetTotalNumberOfRemovalVehicles() const
{
	int nNb = 0;
	for (std::map<TypeVehicule*, int>::const_iterator iter = nNumberOfRemovalVehicles.begin(); iter != nNumberOfRemovalVehicles.end(); ++iter)
	{
		nNb += iter->second;
	}
	return nNb;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// S�rialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template void EdieSensorData::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void EdieSensorData::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void EdieSensorData::serialize(Archive& ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(dbTotalTravelledDistance);
    ar & BOOST_SERIALIZATION_NVP(dbTotalTravelledTime);
	ar & BOOST_SERIALIZATION_NVP(nNumberOfRemovalVehicles);
	ar & BOOST_SERIALIZATION_NVP(vectofvehicleIDs);
}

template void EdieSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void EdieSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void EdieSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(LongitudinalSensor);
    ar & BOOST_SERIALIZATION_NVP(data);
    ar & BOOST_SERIALIZATION_NVP(eTuyauType);
    ar & BOOST_SERIALIZATION_NVP(m_pParentMFD);
}


template void EdieSensorSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void EdieSensorSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void EdieSensorSnapshot::serialize(Archive& ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensorSnapshot);
	ar & BOOST_SERIALIZATION_NVP(workingData);
	ar & BOOST_SERIALIZATION_NVP(previousPeriodData);
}
