#include "stdafx.h"
#include "BlueToothSensor.h"

#include "PonctualSensor.h"
#include "../Connection.h"
#include "../tuyau.h"
#include "../SystemUtil.h"
#include "../TraceDocTrafic.h"
#include "../reseau.h"
#include "../vehicule.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

BlueToothSensorSnapshot::~BlueToothSensorSnapshot()
{
    for(size_t i = 0; i < lstCapteursAmontSnapshots.size(); i++)
    {
        delete lstCapteursAmontSnapshots[i];
    }
    for(size_t i = 0; i < lstCapteursAvalSnapshots.size(); i++)
    {
        delete lstCapteursAvalSnapshots[i];
    }
}

BlueToothSensor::BlueToothSensor() : AbstractSensor()
{
    m_pNode = NULL;
}

BlueToothSensor::BlueToothSensor(SensorsManager * pParentSensorsManager, const std::string & strNom, Connexion *pNode, const std::map<Tuyau*, double> &mapPositions)
    : AbstractSensor(strNom)
{
    m_pNode = pNode;

    // Initialisation du capteur ...
    // création d'un capteur ponctuel pour chaque tronçon amont
    for(size_t i = 0; i < pNode->m_LstTuyAm.size(); i++)
    {
        double dbPos;
        Tuyau * pTuy = pNode->m_LstTuyAm[i];
        if(mapPositions.find(pTuy) != mapPositions.end())
        {
            dbPos = mapPositions.at(pTuy);
        }
        else
        {
            dbPos = pTuy->GetLength()-0.0001;
        }
        lstCapteursAmont.push_back(new PonctualSensor(pTuy, dbPos));
        pTuy->getLstRelatedSensorsBySensorsManagers()[pParentSensorsManager].push_back(this);
    }

    // création d'un capteur ponctuel pour chaque tronçon aval
    for(size_t i = 0; i < pNode->m_LstTuyAv.size(); i++)
    {
        Tuyau * pTuy = pNode->m_LstTuyAv[i];
        double dbPos;        
        if(mapPositions.find(pTuy) != mapPositions.end())
        {
            dbPos = mapPositions.at(pTuy);
        }
        else
        {
            dbPos =  0.0001;
        }
        lstCapteursAval.push_back(new PonctualSensor(pTuy, dbPos));
        pTuy->getLstRelatedSensorsBySensorsManagers()[pParentSensorsManager].push_back(this);
    }
}

BlueToothSensor::~BlueToothSensor()
{
    for(size_t i = 0; i < lstCapteursAmont.size(); i++)
    {
        delete lstCapteursAmont[i];
    }
    for(size_t i = 0; i < lstCapteursAval.size(); i++)
    {
        delete lstCapteursAval[i];
    }
}

std::string BlueToothSensor::GetSensorPeriodXMLNodeName() const
{
    return "CAPTEURS_BLUETOOTH";
}

BlueToothVehSensorData::BlueToothVehSensorData()
{
    nbVehs = 0;
}

void BlueToothSensor::CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh)
{
    PonctualSensor * pCptAmont;
    PonctualSensor * pCptAval;
    bool bMoveFound, bFranchissement, bAmont;
    double dbDst;
    int nVoie;

    for(size_t iCptAmont = 0; iCptAmont < lstCapteursAmont.size(); iCptAmont++)
    {
        pCptAmont = lstCapteursAmont[iCptAmont];
        pCptAmont->CalculFranchissement(pVeh.get(), bFranchissement, dbDst, nVoie, bAmont);
            
        if(bFranchissement)
        {
            pCptAmont->GetNIDVehFranchissant().push_back(pVeh->GetID());
            // Calcul de l'instant de franchissement du capteur au prorata de la distance
            // entre la position du véhicule et la position du capteur
            double dbInstFr;
            if( bAmont )
				if( pVeh->GetExitInstant() > 0)
					dbInstFr = pNetwork->GetTimeStep() * dbDst / pVeh->GetDstParcourueEx() + (pVeh->GetExitInstant() - pNetwork->GetTimeStep());
				else
					dbInstFr = pNetwork->GetTimeStep() * dbDst / pVeh->GetDstParcourueEx() + (dbInstant - pNetwork->GetTimeStep());
            else
				if( pVeh->GetHeureEntree() > dbInstant - pNetwork->GetTimeStep() )
					dbInstFr = pVeh->GetHeureEntree()+((dbInstant - pVeh->GetHeureEntree()) * (1- dbDst / pVeh->GetDstParcourueEx())) ;
				else
					dbInstFr = (pNetwork->GetTimeStep() * (1- dbDst / pVeh->GetDstParcourueEx())) + (dbInstant - pNetwork->GetTimeStep());
            pCptAmont->GetInstVehFranchissant().push_back(dbInstFr);
        }
    }

    for(size_t iCptAval = 0; iCptAval < lstCapteursAval.size(); iCptAval++)
    {
        pCptAval = lstCapteursAval[iCptAval];
        pCptAval->CalculFranchissement(pVeh.get(), bFranchissement, dbDst, nVoie, bAmont);

        if(bFranchissement)
        {
            bMoveFound = false;

            // recherche dans la liste des capteurs amont l'ID du véhicule détecté
            for(size_t iCptAmont = 0; !bMoveFound && iCptAmont < lstCapteursAmont.size(); iCptAmont++)
            {
                pCptAmont = lstCapteursAmont[iCptAmont];
                        
                for(size_t iVehID = 0; !bMoveFound && iVehID < pCptAmont->GetNIDVehFranchissant().size(); iVehID++)
                {
                    if(pCptAmont->GetNIDVehFranchissant()[iVehID] == pVeh->GetID())
                    {
                        // on a trouvé le capteur amont qui a vu passer le véhicule, on ne continue pas la recherche
                        bMoveFound = true;
                        // suppression du véhicule de la liste du capteur amont (pour ne pas se trainer des listes immenses pour rien)
                        pCptAmont->GetNIDVehFranchissant().erase(pCptAmont->GetNIDVehFranchissant().begin()+iVehID);
                        double dbInstantEntree = pCptAmont->GetInstVehFranchissant()[iVehID];
                        pCptAmont->GetInstVehFranchissant().erase(pCptAmont->GetInstVehFranchissant().begin()+iVehID);
                        // une fois le tuyau amont identifié, MAJ des résultats du capteur
                        BlueToothVehSensorData & dataBT = data.mapCoeffsDir[pCptAmont->GetTuyau()->GetLabel()][pCptAval->GetTuyau()->GetLabel()];
                        dataBT.nbVehs++;
                        dataBT.nIDs.push_back(pVeh->GetID());
                        dataBT.dbInstEntrees.push_back(dbInstantEntree);

                        // Calcul de l'instant de franchissement du capteur au prorata de la distance
                        // entre la position du véhicule et la position du capteur
                        double dbInstFr;
                        if( bAmont )
					        if( pVeh->GetExitInstant() > 0)
						        dbInstFr = pNetwork->GetTimeStep() * dbDst / pVeh->GetDstParcourueEx() + (pVeh->GetExitInstant() - pNetwork->GetTimeStep());
					        else
						        dbInstFr = pNetwork->GetTimeStep() * dbDst / pVeh->GetDstParcourueEx() + (dbInstant - pNetwork->GetTimeStep());
                        else
					        if( pVeh->GetHeureEntree() > dbInstant - pNetwork->GetTimeStep() )
						        dbInstFr = pVeh->GetHeureEntree()+((dbInstant - pVeh->GetHeureEntree()) * (1- dbDst / pVeh->GetDstParcourueEx())) ;
					        else
						        dbInstFr = (pNetwork->GetTimeStep() * (1- dbDst / pVeh->GetDstParcourueEx())) + (dbInstant - pNetwork->GetTimeStep());

                        dataBT.dbInstSorties.push_back(dbInstFr);
                    }
                }
            }
        }
    }
}

void BlueToothSensor::AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink)
{
    PonctualSensor * pCptAmont;
    PonctualSensor * pCptAval;

    bool bFranchissement;
    double dbInstFr, dbInvVit;

    // Capteurs amont
    for(size_t iCptAmont = 0; iCptAmont < lstCapteursAmont.size(); iCptAmont++)
    {
        pCptAmont = lstCapteursAmont[iCptAmont];

        pCptAmont->CalculFranchissementMeso(dbInstant, pVeh, pLink, dbLengthInLink, bFranchissement, dbInstFr, dbInvVit);

        if(bFranchissement)
        {
            pCptAmont->GetNIDVehFranchissant().push_back(pVeh->GetID());
            pCptAmont->GetInstVehFranchissant().push_back(dbInstFr);
        }
    }

    // Capteurs aval
    for(size_t iCptAval = 0; iCptAval < lstCapteursAval.size(); iCptAval++)
    {
        pCptAval = lstCapteursAval[iCptAval];

        pCptAval->CalculFranchissementMeso(dbInstant, pVeh, pLink, dbLengthInLink, bFranchissement, dbInstFr, dbInvVit);

        if(bFranchissement)
        {
            bool bMoveFound = false;

            // recherche dans la liste des capteurs amont l'ID du véhicule détecté
            for(size_t iCptAmont = 0; !bMoveFound && iCptAmont < lstCapteursAmont.size(); iCptAmont++)
            {
                pCptAmont = lstCapteursAmont[iCptAmont];
                        
                for(size_t iVehID = 0; !bMoveFound && iVehID < pCptAmont->GetNIDVehFranchissant().size(); iVehID++)
                {
                    if(pCptAmont->GetNIDVehFranchissant()[iVehID] == pVeh->GetID())
                    {
                        // on a trouvé le capteur amont qui a vu passer le véhicule, on ne continue pas la recherche
                        bMoveFound = true;
                        // suppression du véhicule de la liste du capteur amont (pour ne pas se trainer des listes immenses pour rien)
                        pCptAmont->GetNIDVehFranchissant().erase(pCptAmont->GetNIDVehFranchissant().begin()+iVehID);
                        double dbInstantEntree = pCptAmont->GetInstVehFranchissant()[iVehID];
                        pCptAmont->GetInstVehFranchissant().erase(pCptAmont->GetInstVehFranchissant().begin()+iVehID);
                        // une fois le tuyau amont identifié, MAJ des résultats du capteur
                        BlueToothVehSensorData & dataBT = data.mapCoeffsDir[pCptAmont->GetTuyau()->GetLabel()][pCptAval->GetTuyau()->GetLabel()];
                        dataBT.nbVehs++;
                        dataBT.nIDs.push_back(pVeh->GetID());
                        dataBT.dbInstEntrees.push_back(dbInstantEntree);
                        dataBT.dbInstSorties.push_back(dbInstFr);
                    }
                }
            }
        }
    }
}

void BlueToothSensor::WriteDef(DocTrafic* pDocTrafic)
{
}

void BlueToothSensor::Write(double dbInstant, Reseau * pReseau, double dbPeriodAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutputWriter)
{
    if(!docTrafics.empty())
    {
        // écriture
        std::deque<TraceDocTrafic* >::const_iterator itDocTraf;
        for( itDocTraf = docTrafics.begin(); itDocTraf != docTrafics.end(); ++itDocTraf)
        {
            (*itDocTraf)->AddInfoCapteurBlueTooth(m_strNom, data);
        }
    }
}

void BlueToothSensor::PrepareNextPeriod()
{
    data.mapCoeffsDir.clear();
}

AbstractSensorSnapshot * BlueToothSensor::TakeSnapshot()
{
    BlueToothSensorSnapshot * pResult = new BlueToothSensorSnapshot();
    pResult->data = data;
    for(size_t iCptInt = 0; iCptInt < lstCapteursAmont.size(); iCptInt++)
    {
        pResult->lstCapteursAmontSnapshots.push_back((PonctualSensorSnapshot*)lstCapteursAmont[iCptInt]->TakeSnapshot());
    }
    for(size_t iCptInt = 0; iCptInt < lstCapteursAval.size(); iCptInt++)
    {
        pResult->lstCapteursAvalSnapshots.push_back((PonctualSensorSnapshot*)lstCapteursAval[iCptInt]->TakeSnapshot());
    }
    return pResult;
}

void BlueToothSensor::Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup)
{
    BlueToothSensorSnapshot * pSnapshot = (BlueToothSensorSnapshot*)backup;
    data = pSnapshot->data;
    for(size_t iCptInt = 0; iCptInt < lstCapteursAmont.size(); iCptInt++)
    {
        lstCapteursAmont[iCptInt]->Restore(pNetwork, pSnapshot->lstCapteursAmontSnapshots[iCptInt]);
    }
    for(size_t iCptInt = 0; iCptInt < lstCapteursAval.size(); iCptInt++)
    {
        lstCapteursAval[iCptInt]->Restore(pNetwork, pSnapshot->lstCapteursAvalSnapshots[iCptInt]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template void BlueToothVehSensorData::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void BlueToothVehSensorData::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void BlueToothVehSensorData::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(nbVehs);
    ar & BOOST_SERIALIZATION_NVP(nIDs);
    ar & BOOST_SERIALIZATION_NVP(dbInstEntrees);
    ar & BOOST_SERIALIZATION_NVP(dbInstSorties);
}

template void BlueToothSensorData::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void BlueToothSensorData::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void BlueToothSensorData::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(mapCoeffsDir);
}

template void BlueToothSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void BlueToothSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void BlueToothSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensor);
    ar & BOOST_SERIALIZATION_NVP(m_pNode);
    ar & BOOST_SERIALIZATION_NVP(lstCapteursAmont);
    ar & BOOST_SERIALIZATION_NVP(lstCapteursAval);
    ar & BOOST_SERIALIZATION_NVP(data);
}

template void BlueToothSensorSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void BlueToothSensorSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void BlueToothSensorSnapshot::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensorSnapshot);
    ar & BOOST_SERIALIZATION_NVP(data);
    ar & BOOST_SERIALIZATION_NVP(lstCapteursAmontSnapshots);
    ar & BOOST_SERIALIZATION_NVP(lstCapteursAvalSnapshots);
}
