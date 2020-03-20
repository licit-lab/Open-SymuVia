#include "stdafx.h"
#include "GlobalSensor.h"

#include "../vehicule.h"
#include "../SystemUtil.h"
#include "../reseau.h"
#include "../TraceDocTrafic.h"

#include <boost/serialization/map.hpp>
#include <boost/serialization/shared_ptr.hpp>

VehData::VehData()
{
    //dbInstCreation = -1;
	dbInstSuppres = -1;
	//dbDstParcourue = 0;
	dbDstCumParcPrev = 0;
}

GlobalSensor::GlobalSensor() : AbstractSensor()
{
}

GlobalSensor::~GlobalSensor()
{
}

std::string GlobalSensor::GetSensorPeriodXMLNodeName() const
{
    return "CAPTEUR_GLOBAL";
}

void GlobalSensor::CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pV)
{
    ProcessVehicle(pV);
}

void GlobalSensor::AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink)
{
    ProcessVehicle(pVeh->shared_from_this());
}

void GlobalSensor::ProcessVehicle(boost::shared_ptr<Vehicule> pV)
{
    if( data.mapVehs.find(pV)== data.mapVehs.end())
	{
		// Ce véhicule a été créé, il faut l'ajouter à la map
		VehData dv;
		data.mapVehs.insert( std::pair<boost::shared_ptr<Vehicule>,VehData>(pV, dv) );
	}

	if( !pV->GetLink(0) && pV->GetCurrentTuyauMeso() == NULL ) // Véhicule supprimé
	{
		std::map<boost::shared_ptr<Vehicule>, VehData, LessVehiculePtr<Vehicule> >::iterator it;
		it = data.mapVehs.find(pV);
		if( it != data.mapVehs.end() )
		{
			(*it).second.dbInstSuppres = pV->GetExitInstant();
		}
	}
}

void GlobalSensor::WriteDef(DocTrafic* pDocTrafic)
{
}

void GlobalSensor::Write(double dbInstant, Reseau * pReseau, double dbPeriodeAgregation, double dbDeb, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutputWriter)
{
    if(!docTrafics.empty())
    {
        std::deque<TraceDocTrafic* >::const_iterator itDocTraf;
        std::map<boost::shared_ptr<Vehicule>, VehData, LessVehiculePtr<Vehicule> >::iterator itV;
        double TpsEcoulementCum = 0;
        double DistanceParcourueCum = 0;
        int nNbVeh = 0;

        for(itV= data.mapVehs.begin(); itV!= data.mapVehs.end(); ++itV)
        {
            double TpsEcoulement;
            double DistanceParcourue;

            if( (*itV).first->GetHeureEntree() < dbFin )
            {
                if((*itV).second.dbDstCumParcPrev > -1)
                {
                    if( (*itV).second.dbInstSuppres >= 0 && (*itV).second.dbInstSuppres < dbFin )	// Le véhicule est sorti avant la fin du pas de temps
                    {
                        TpsEcoulement = (*itV).second.dbInstSuppres - std::max<double>((*itV).first->GetHeureEntree(), dbDeb);
                    }
                    else // Le véhicule est encore sur le réseau
                    {
                        TpsEcoulement = dbFin - std::max<double>((*itV).first->GetHeureEntree(), dbDeb);
                    }

                    DistanceParcourue = (*itV).first->GetDstCumulee() - (*itV).second.dbDstCumParcPrev;

                    std::string sIDVeh = SystemUtil::ToString((*itV).first->GetID());

                    for( itDocTraf = docTrafics.begin(); itDocTraf != docTrafics.end(); itDocTraf++)
                    {
                        (*itDocTraf)->AddInfoCapteurGlobal( sIDVeh, TpsEcoulement, DistanceParcourue);
                    }
                    TpsEcoulementCum += TpsEcoulement;
                    DistanceParcourueCum += DistanceParcourue;

                    nNbVeh++;
                }

            }

        }
        if( TpsEcoulementCum > 0)
        {
            for( itDocTraf = docTrafics.begin(); itDocTraf != docTrafics.end(); itDocTraf++)
            {
                (*itDocTraf)->AddIndicateursCapteurGlobal(DistanceParcourueCum, TpsEcoulementCum, DistanceParcourueCum/TpsEcoulementCum, nNbVeh );
            }
        }

        // MAJ de la map (suppression des véhicules sortis pendant la période d'agrégation et maj de la variable dbDstCumParcPrev)
        for(itV= data.mapVehs.begin(); itV!= data.mapVehs.end(); ++itV)
        {
            if( (*itV).second.dbInstSuppres >= 0  && (*itV).second.dbInstSuppres < dbFin )
                (*itV).second.dbDstCumParcPrev = -1;
            else
            {
                if( (*itV).first->GetHeureEntree() < dbFin )
                    (*itV).second.dbDstCumParcPrev = (*itV).first->GetDstCumulee();
            }
        }
    }
}

void GlobalSensor::PrepareNextPeriod()
{
}

AbstractSensorSnapshot * GlobalSensor::TakeSnapshot()
{
    GlobalSensorSnapshot * pResult = new GlobalSensorSnapshot();
    std::map<boost::shared_ptr<Vehicule>, VehData, LessVehiculePtr<Vehicule> >::iterator itCptVeh;
    for(itCptVeh = data.mapVehs.begin(); itCptVeh != data.mapVehs.end(); ++itCptVeh)
    {
        pResult->mapVehs[itCptVeh->first->GetID()] = itCptVeh->second;
    }
    return pResult;
}

void GlobalSensor::Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup)
{
    std::map<boost::shared_ptr<Vehicule>, VehData, LessVehiculePtr<Vehicule> > savedMap = data.mapVehs;
    data.mapVehs.clear();
    GlobalSensorSnapshot * pSnapshot = (GlobalSensorSnapshot*)backup;
    std::map<int, VehData>::iterator itCptVeh;
    for(itCptVeh = pSnapshot->mapVehs.begin(); itCptVeh != pSnapshot->mapVehs.end(); ++itCptVeh)
    {
        boost::shared_ptr<Vehicule> pVeh = pNetwork->GetVehiculeFromID(itCptVeh->first);
        if(!pVeh)
        {
            // le véhicule est sorti du réseau, on tente de le recupérer dans la map actuelle
            std::map<boost::shared_ptr<Vehicule>, VehData, LessVehiculePtr<Vehicule> >::iterator iter;
            for(iter = savedMap.begin(); iter != savedMap.end(); ++iter)
            {
                if(iter->first->GetID() == itCptVeh->first)
                {
                    pVeh = iter->first;
                    break;
                }
            }
        }
        assert(pVeh);
        data.mapVehs.insert(std::make_pair(pVeh, itCptVeh->second));
    }
}
			    

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template void VehData::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void VehData::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void VehData::serialize(Archive& ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(dbInstSuppres);
	ar & BOOST_SERIALIZATION_NVP(dbDstCumParcPrev);
}

template void GlobalSensorData::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void GlobalSensorData::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void GlobalSensorData::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(mapVehs);
}

template void GlobalSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void GlobalSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void GlobalSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensor);
    ar & BOOST_SERIALIZATION_NVP(data);
}

template void GlobalSensorSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void GlobalSensorSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void GlobalSensorSnapshot::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensorSnapshot);
    ar & BOOST_SERIALIZATION_NVP(mapVehs);
}
