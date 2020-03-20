#include "stdafx.h"
#include "LongitudinalSensor.h"

#include "../tuyau.h"
#include "../voie.h"
#include "../SystemUtil.h"
#include "../TraceDocTrafic.h"

#include <boost/serialization/map.hpp>

LongitudinalSensor::LongitudinalSensor() : AbstractSensor()
{
    m_pTuyau = NULL;
}

LongitudinalSensor::LongitudinalSensor(const std::string & strName, Tuyau * pTuyau, double dbStartPos, double dbEndPos)
    : AbstractSensor(strName)
{
    m_pTuyau = pTuyau;
    dbPositionDebut = dbStartPos;
    dbPositionFin = dbEndPos;
}

LongitudinalSensor::~LongitudinalSensor()
{
}

std::string LongitudinalSensor::GetSensorPeriodXMLNodeName() const
{
    return "CAPTEURS_LONGITUDINAUX";
}

void LongitudinalSensor::InitData()
{
    // Initialisation d'un résultat par couple de voies adjacentes
	int countNbVoies = m_pTuyau->getNbVoiesDis();
    
    for(int i = 0; i < countNbVoies-1; i++)
    {
        int iDVoie1 = m_pTuyau->GetLstLanes()[i]->GetNum();
        int iDVoie2 = m_pTuyau->GetLstLanes()[i+1]->GetNum();
        data.mapNbChgtVoie[std::make_pair(iDVoie1, iDVoie2)] = 0;
        data.mapNbChgtVoie[std::make_pair(iDVoie2, iDVoie1)] = 0;
    }
}


void LongitudinalSensor::AddChgtVoie(Tuyau * pTuyau, double dbPosition, int nVoieOrigine, int nVoieDestination)
{
    if(pTuyau == m_pTuyau)
    {
        if(dbPositionDebut <= dbPosition && dbPositionFin >= dbPosition)
        {
            // incrémentation du compteur
            data.mapNbChgtVoie[std::make_pair(nVoieOrigine, nVoieDestination)]++;
        }
    }
}

void LongitudinalSensor::CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh)
{
    // rien ici (pour de meilleurs perfs, on maj les données dans la méthode de changement de voie).
}


void LongitudinalSensor::WriteDef(DocTrafic* pDocTrafic)
{
    Point ptCoord, ptCoordFin;
    CalcCoordFromPos(GetTuyau(), GetStartPosition(), ptCoord);
    CalcCoordFromPos(GetTuyau(), GetEndPosition(), ptCoordFin);
    pDocTrafic->AddDefCapteurLongitudinal( GetUnifiedID(), GetTuyau()->GetLabel(), GetStartPosition(), ptCoord, GetEndPosition(), ptCoordFin);
}

void LongitudinalSensor::Write(double dbInstant, Reseau * pReseau, double dbPeriodAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutputWriter)
{
    if(!docTrafics.empty())
    {
        // Ecriture des données capteur pour chaque couple de voie
        std::map<std::pair<int, int>, int>::iterator pIter;
        for(pIter = data.mapNbChgtVoie.begin(); pIter != data.mapNbChgtVoie.end(); pIter++)
        {
            std::deque<TraceDocTrafic* >::const_iterator itDocTraf;
            for( itDocTraf = docTrafics.begin(); itDocTraf != docTrafics.end(); itDocTraf++ )
            {
                // écriture
                (*itDocTraf)->AddInfoCapteurLongitudinal(m_strNom,   // identifiant capteur
                    SystemUtil::ToString(pIter->first.first+1),                   // voie d'origine
                    SystemUtil::ToString(pIter->first.second+1),                  // voie de destination
                    SystemUtil::ToString(pIter->second));                       // compteur sur la période
            }
        }
    }
}

void LongitudinalSensor::PrepareNextPeriod()
{
    std::map<std::pair<int, int>, int>::iterator pIter;
    for(pIter = data.mapNbChgtVoie.begin(); pIter != data.mapNbChgtVoie.end(); pIter++)
    {
        pIter->second = 0;
    }
}


double LongitudinalSensor::GetSensorLength()
{
    return dbPositionFin - dbPositionDebut;
}

Tuyau * LongitudinalSensor::GetTuyau()
{
    return m_pTuyau;
}
double LongitudinalSensor::GetStartPosition()
{
    return dbPositionDebut;
}
double LongitudinalSensor::GetEndPosition()
{
    return dbPositionFin;
}

AbstractSensorSnapshot * LongitudinalSensor::TakeSnapshot()
{
    LongitudinalSensorData * pResult = new LongitudinalSensorData();
    pResult->mapNbChgtVoie = data.mapNbChgtVoie;
    return pResult;
}

void LongitudinalSensor::Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup)
{
    LongitudinalSensorData * pSnapshot = (LongitudinalSensorData*)backup;
    data.mapNbChgtVoie = pSnapshot->mapNbChgtVoie;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template void LongitudinalSensorData::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void LongitudinalSensorData::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void LongitudinalSensorData::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensorSnapshot);
	ar & BOOST_SERIALIZATION_NVP(mapNbChgtVoie);
}

template void LongitudinalSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void LongitudinalSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void LongitudinalSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensor);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyau);
	ar & BOOST_SERIALIZATION_NVP(dbPositionDebut);
	ar & BOOST_SERIALIZATION_NVP(dbPositionFin);
    ar & BOOST_SERIALIZATION_NVP(data);
}