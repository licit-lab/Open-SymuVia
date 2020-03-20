#include "stdafx.h"
#include "PonctualSensor.h"

#include "../tuyau.h"
#include "../vehicule.h"
#include "../voie.h"
#include "../reseau.h"
#include "../SystemUtil.h"
#include "../CSVOutputWriter.h"
#include "../TraceDocTrafic.h"

#include "../usage/Trip.h"

#include <boost/serialization/deque.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

using namespace std;


PonctualSensor::PonctualSensor() : AbstractSensor()
{
    m_pTuyau = NULL;
    m_pTuyauAmont = NULL;

    m_pNextPeriodSensor = NULL;
}

PonctualSensor::PonctualSensor(Tuyau * pTuyau, double dbPosition) : AbstractSensor()
{
    m_pTuyau = pTuyau;
    m_dbPosition = dbPosition;
    m_pTuyauAmont = NULL;

    m_pNextPeriodSensor = NULL;
}

PonctualSensor::PonctualSensor(const std::string & strNom, Tuyau * pTuyau, double dbPosition) : AbstractSensor(strNom)
{
    m_pTuyau = pTuyau;
    m_dbPosition = dbPosition;
    m_pTuyauAmont = NULL;

    m_pNextPeriodSensor = NULL;
}

PonctualSensor::~PonctualSensor()
{
    if(m_pNextPeriodSensor)
    {
        delete m_pNextPeriodSensor;
    }
}

std::string PonctualSensor::GetSensorPeriodXMLNodeName() const
{
    return "CAPTEURS_PONCTUELS";
}

Tuyau * PonctualSensor::GetTuyau()
{
    return m_pTuyau;
}

double PonctualSensor::GetPosition()
{
    return m_dbPosition;
}

PonctualSensorSnapshot & PonctualSensor::GetData()
{
    return m_Data;
}

std::deque<int> & PonctualSensor::GetNIDVehFranchissant()
{
    return m_Data.nIDVehFranchissant;
}

std::deque<double> & PonctualSensor::GetInstVehFranchissant()
{
    return m_Data.dbInstVehFranchissant;
}

std::deque<std::string> & PonctualSensor::GetTypeVehFranchissant()
{
    return m_Data.cTypeVehFranchissant;
}

std::deque<int> & PonctualSensor::GetVoieVehFranchissant()
{
    return m_Data.dbVoieVehFranchissant;
}

std::map<TypeVehicule*, std::map<Voie*, std::vector<double>, LessPtr<Voie>>, LessPtr<TypeVehicule>> & PonctualSensor::GetNbTypeVehInst()
{
    return m_Data.pNbTypeVehInst;
}

int PonctualSensor::GetLastInd()
{
    return nLastInd;
}

void PonctualSensor::SetLastInd(int lastInd)
{
    nLastInd = lastInd;
}

void PonctualSensor::SetTuyauAmont(Tuyau * pTuyAm)
{
    m_pTuyauAmont = pTuyAm;
}

void PonctualSensor::InitData(Reseau * pNetwork, double dbPeriod)
{
	PonctualSensorData dt;

	// Information globale
    for(int k=0; k<m_pTuyau->getNbVoiesDis(); k++)
    {				
        dt.nNbVehFranchissant.push_back(0);
		dt.dbCumulInvVit.push_back(0.0);		
		dt.nCumNbVehFranchissant.push_back(0);        
    }	

	// Itération sur tous les types de vehicule
	std::deque<TypeVehicule*>::iterator beginTVeh, endTVeh, itTVeh;
	beginTVeh = pNetwork->m_LstTypesVehicule.begin();
	endTVeh = pNetwork->m_LstTypesVehicule.end();
	int countCompteur = (int)(dbPeriod / pNetwork->GetTimeStep());
	//int countNbVoies = pTuyau->getNb_voies();
	int countNbVoies = m_pTuyau->getNbVoiesDis();
	for (itTVeh = beginTVeh; itTVeh != endTVeh; itTVeh++)
	{
		// Ajout d'un type de véhicule
		TypeVehicule * tveh = *itTVeh;
		std::map<Voie*, std::vector<double>, LessPtr<Voie>> mapVoie;
		m_Data.pNbTypeVehInst[tveh] = mapVoie;
		// Itération sur toutes les voies
		for (int i=0; i<countNbVoies; i++)
		{
			Voie * pVoie = m_pTuyau->GetLstLanes()[i];
			std::vector<double> lst;
			lst.resize(countCompteur, 0);
			m_Data.pNbTypeVehInst[tveh].insert(std::pair<Voie*, std::vector<double>>(pVoie, lst));
		}
	}

	m_Data.data.insert( std::pair<TypeVehicule*, PonctualSensorData>(nullptr, dt));

	// Information par type de véhicule
	std::deque<TypeVehicule*>::iterator itTV;
	for(itTV = pNetwork->m_LstTypesVehicule.begin(); itTV != pNetwork->m_LstTypesVehicule.end(); itTV++)
	{
		m_Data.data.insert( std::pair<TypeVehicule*, PonctualSensorData>((*itTV), dt));
	}
}

void PonctualSensor::ResetData()
{
    // Itération sur tous les types de véhicule
	std::map<TypeVehicule*, std::map<Voie*, std::vector<double>, LessPtr<Voie>>, LessPtr<TypeVehicule>>::iterator beginTVeh, endTVeh, itTVeh;
	beginTVeh = m_Data.pNbTypeVehInst.begin();
	endTVeh = m_Data.pNbTypeVehInst.end();
	for(itTVeh = beginTVeh; itTVeh != endTVeh; itTVeh++)
	{				
		// Itération sur toutes les voies
		std::map<Voie*, std::vector<double>, LessPtr<Voie>>::iterator beginVoie, endVoie, itVoie;
		beginVoie = (*itTVeh).second.begin();
		endVoie = (*itTVeh).second.end();
		for (itVoie = beginVoie; itVoie != endVoie; itVoie++)
		{
			// Itération sur tous les compteurs
			std::vector<double>::iterator beginNb, endNb, itNb;
			beginNb = (*itVoie).second.begin();
			endNb = (*itVoie).second.end();
			for (itNb = beginNb; itNb != endNb; itNb++)
			{
				(*itNb) = 0;
			}
		}
	}

	// Information par type de véhicule
	std::map<TypeVehicule*, PonctualSensorData>::iterator itdt;
	for(itdt = m_Data.data.begin(); itdt != m_Data.data.end(); itdt++)
	{
		for(int i=0; i<m_pTuyau->getNbVoiesDis(); i++)
		{
			(*itdt).second.dbCumulInvVit[i] = 0;	
			(*itdt).second.nNbVehFranchissant[i] = 0;
			(*itdt).second.nCumNbVehFranchissant[i] = 0;	
		}
	}

	m_Data.nIDVehFranchissant.clear();
	m_Data.cTypeVehFranchissant.clear();
	m_Data.dbInstVehFranchissant.clear();
    m_Data.dbVoieVehFranchissant.clear();
}


//================================================================
    void PonctualSensor::CalculFranchissement
//----------------------------------------------------------------
// Fonction  : Détection du franchissement d'un capteur ponctuel
//             par un véhicule
// Version du: 08/01/2013
// Historique: 08/01/2013 (C.Bécarie - Tinea)
//             Création
//================================================================        
(
    Vehicule * pVeh,
    bool & bFranchissement,
    double & dbDst,
    int & nVoie,
    bool & bAmont
)
{
    bFranchissement = false;

	if( pVeh->GetPos(0) > UNDEF_POSITION || pVeh->GetExitInstant()>0 )		// Sinon la nouvelle position du véhicule n'a pas été calculée (pas encore rentré sur le réseau)
	{
		// Le capteur est-il placé sur le même tuyau que le véhicule au début du pas de temps ?
		if( m_pTuyau == pVeh->GetLink(1) )
		{
			// Le capteur est positionné en aval du véhicule au début du pas de temps ?
			if( pVeh->GetPos(1) < m_dbPosition)
			{
				// Le véhicule est-il positionné après le capteur à la fin du pas de temps
				// ou a t'il changé de tuyau ?
				if( pVeh->GetPos(0) >= m_dbPosition || pVeh->GetLink(1) != pVeh->GetLink(0) )
				{
					bFranchissement = true;
                    if(pVeh->GetVoie(1))
                    {
					    nVoie = pVeh->GetVoie(1)->GetNum();     // Voie du véhicule
                    }
                    else
                    {
                        // Cas d'un véhicule meso : on ne connait pas la voie, on prend la voie de doite par défaut, on n'aura donc pas d'infos utiles
                        // par voie
                        nVoie = 0;
                    }
					// Calcul de la distance entre la position initiale du véhicule et le capteur
					dbDst = m_dbPosition - pVeh->GetPos(1);
					bAmont = true; // La distance calculée est celle entre le véhicule à l'état initial et le capteur
					return;
				}
			}
		}
		else
		{
			// On regarde si le capteur considéré est sur le tuyau sur lequel est placé le véhicule à la
			// fin du pas de temps. Si le véhicule ne change pas de tuyau, pas la peine d'aller
			// plus loin puisqu'il n'y a pas de capteur sur le tuyau initial
			if( pVeh->GetLink(1) != pVeh->GetLink(0) )
			{
				// Le capteur est-il sur le tuyau final ?
				if( m_pTuyau == pVeh->GetLink(0) )
				{
					// Le véhicule a t-il passé le capteur ?
					if( pVeh->GetPos(0) >= m_dbPosition )
					{
						bFranchissement = true;
						nVoie = pVeh->GetVoie(0)->GetNum();     // Voie du véhicule
						// Calcul de la distance entre le capteur et la position finale du véhicule
						dbDst = pVeh->GetPos(0) - m_dbPosition;
						bAmont = false; // La distance calculée est celle entre le capteur et le véhicule à l'état final
						return;
					}
				}
				else
				{
					// Le capteur est-il situé sur un des tronçons traversés ?
					for(unsigned int t=0; t< pVeh->m_LstUsedLanes.size(); t++)
					{
						if( m_pTuyau == pVeh->m_LstUsedLanes[t]->GetParent() )
						{
							bFranchissement = true;
							nVoie = pVeh->m_LstUsedLanes[t]->GetNum();
							// Calcul de la distance entre la position initiale du véhicule et le capteur

							dbDst = m_dbPosition + (pVeh->GetVoie(1)->GetLength() - pVeh->GetPos(1));

							for(unsigned int  u=0; u< t; u++)
								dbDst += pVeh->m_LstUsedLanes[u]->GetLength();
	                                     
							bAmont = true; // La distance calculée est celle entre le véhicule à l'état initial et le capteur
							return;
						}
					}
				}
			}
		}
		if (pVeh->GetLink(0) == NULL && pVeh->GetExitInstant()>0)
		{
			if (pVeh->GetTrip()->GetFinalDestination()->GetInputPosition().GetLink() == m_pTuyau)
			{
				bFranchissement = true;
				nVoie = 0;
				dbDst = 0;
				bAmont = true;
			}
		}
	}
}


void PonctualSensor::CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh)
{
    if (bNewPeriod && m_pNextPeriodSensor == NULL)
    {
        m_pNextPeriodSensor = new PonctualSensor();
        PonctualSensorData dt;

		// Tout type de véhicule
        for(int k=0; k<m_pTuyau->getNbVoiesDis(); k++)
        {
            dt.nNbVehFranchissant.push_back(0);
            dt.dbCumulInvVit.push_back(0);
			dt.nCumNbVehFranchissant = m_Data.data.find(nullptr)->second.nCumNbVehFranchissant;
        }
		m_pNextPeriodSensor->m_Data.data.insert( std::pair<TypeVehicule*, PonctualSensorData>(nullptr, dt) );

		// Par type de véhicule
		std::deque<TypeVehicule*>::iterator itTV;
		for(itTV = pNetwork->m_LstTypesVehicule.begin(); itTV != pNetwork->m_LstTypesVehicule.end(); itTV++)
		{				
			dt.nCumNbVehFranchissant = m_Data.data.find((*itTV))->second.nCumNbVehFranchissant;
			m_pNextPeriodSensor->m_Data.data.insert( std::pair<TypeVehicule*, PonctualSensorData>((*itTV), dt) );
		}
    }

    PonctualSensor * pCpt = this;
    bool bFranchissement, bAmont;
    double dbDst, dbInstFr, dbInvVit;
    int nVoie;

	if (!pVeh->IsMeso())
	{
		CalculFranchissement(pVeh.get(), bFranchissement, dbDst, nVoie, bAmont);

		if (bFranchissement)
		{
            if (!CheckTuyauAmont(pVeh.get()))
			{
				return;
			}

			// Calcul de l'instant de franchissement du capteur au prorata de la distance
			// entre la position du véhicule et la position du capteur
			if (bAmont)
			if (pVeh->GetExitInstant() > 0)
				dbInstFr = pNetwork->GetTimeStep() * dbDst / pVeh->GetDstParcourueEx() + (pVeh->GetExitInstant() - pNetwork->GetTimeStep());
			else
				dbInstFr = pNetwork->GetTimeStep() * dbDst / pVeh->GetDstParcourueEx() + (dbInstant - pNetwork->GetTimeStep());
			else
			if (pVeh->GetHeureEntree() > dbInstant - pNetwork->GetTimeStep())
				dbInstFr = pVeh->GetHeureEntree() + ((dbInstant - pVeh->GetHeureEntree()) * (1 - dbDst / pVeh->GetDstParcourueEx()));
			else
				dbInstFr = (pNetwork->GetTimeStep() * (1 - dbDst / pVeh->GetDstParcourueEx())) + (dbInstant - pNetwork->GetTimeStep());

			if (bNewPeriod)
			{
				if (dbInstFr > dbInstNewPeriode)
					pCpt = m_pNextPeriodSensor;
				else
				{
					m_pNextPeriodSensor->m_Data.data.find(nullptr)->second.nCumNbVehFranchissant[nVoie]++;  // MAJ du nombre cumulé global de la nouvelle période
					m_pNextPeriodSensor->m_Data.data.find(pVeh->GetType())->second.nCumNbVehFranchissant[nVoie]++;  // MAJ du nombre cumulé pour le type de véhicule de la nouvelle période
				}
			}

			// Calcul de l'inverse de la vitesse de franchissement et ajout à la variable
			// de cumul
			dbInvVit = (pVeh->GetVit(0) + ((dbInstant - dbInstFr) / pNetwork->GetTimeStep())*(pVeh->GetVit(1) - pVeh->GetVit(0)));
			dbInvVit = 1 / dbInvVit;

            AddVehicleInfos(pCpt, pVeh.get(), nVoie, dbInstFr, dbInvVit);
		}
	}
}

void PonctualSensor::AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink)
{
    PonctualSensor * pCpt = this;

    bool bFranchissement;
    double dbInstFr, dbInvVit;
    int nVoie;

    CalculFranchissementMeso(dbInstant, pVeh, pLink, dbLengthInLink, bFranchissement, dbInstFr, dbInvVit);

    if(bFranchissement)
    {
        // rmq. idem que partout ailleurs: pas de voie en mode meso donc on utilise la voie de droite par défaut
        nVoie = 0;

        AddVehicleInfos(this, pVeh, nVoie, dbInstFr, dbInvVit);
    }
}

void PonctualSensor::CalculFranchissementMeso(double dbInstant, Vehicule * pVeh, Tuyau * pLink, double dbLengthInLink, bool & bFranchissement, double & dbInstFr, double & dbInvVit)
{
    bFranchissement = false;

    if(pLink == m_pTuyau && dbLengthInLink > m_dbPosition)
    {
        // On ignore les véhicules qui ne viennent pas du bon tuyau amont
        if(!CheckTuyauAmont(pVeh))
        {
            return;
        }

        bFranchissement = true;

        // on se ramène à la longueur du tronçon si pas de longueur définie (DBL_MAX)
        dbLengthInLink = std::min<double>(dbLengthInLink, pLink->GetLength());

        double dbInputTime = pVeh->GetTimeCode(pLink, Vehicule::TC_INCOMMING);
        assert(dbInputTime != DBL_MAX); // en principe le temps d'entrée du véhicule sur le tronçon doit être défini !
        dbInvVit = (dbInstant-dbInputTime) / dbLengthInLink;
        dbInstFr = dbInputTime + m_dbPosition * dbInvVit;
    }
}

bool PonctualSensor::CheckTuyauAmont(Vehicule * pVeh)
{
    if(m_pTuyauAmont)
    {
        // Si le véhicule n'est pas passé en dernier par ce tronçon non interne, on ne le prend pas en compte
        const std::vector<Tuyau*> & tuyauxParcourus = pVeh->GetTuyauxParcourus();
        Tuyau * pUpstreamLink = NULL;
        // on parcours à rebours cette liste. Le premier tronçon non interne est le tronçon amont de la brique
        for(int iTuy = (int)tuyauxParcourus.size()-1; iTuy >= 0 && !pUpstreamLink; iTuy--)
        {
            //if(tuyauxParcourus[iTuy]->GetBriqueAval() == m_pTuyau->GetBriqueParente() )
			if (tuyauxParcourus[iTuy]->GetBriqueAval() == m_pTuyau->GetBriqueAmont())
            {
                pUpstreamLink = tuyauxParcourus[iTuy];
            }
        }
        if(pUpstreamLink != m_pTuyauAmont)
        {
            return false;
        }
    }
    return true;
}

void PonctualSensor::AddVehicleInfos(PonctualSensor * pCpt, Vehicule * pVeh, int nVoie, double dbInstFr, double dbInvVit)
{
    // rmq. voir si besoin de mieux gérer les capteurs ponctuels pour les véhicules méso...
    // Pour l'instant on dit que les véhicules méso sont tous sur la voie 0. Comme la structure des données
    // de ces capteurs ne comprend pas de champ pour l'ensemble des voies mais qu'on fait la somme pour les cumuls,
    // on est obligés de les mettre dans une voie (ou alors il faut prévoir un élément de plus dans les tableaux pour une voie indéterminée ?)
    // ou alors il faut rajouter le stockage de la donnée cumulée sur toutes les voies.
    pCpt->m_Data.data.find(nullptr)->second.nNbVehFranchissant[nVoie]++;
	pCpt->m_Data.data.find(pVeh->GetType())->second.nNbVehFranchissant[nVoie]++;

    pCpt->m_Data.data.find(nullptr)->second.nCumNbVehFranchissant[nVoie]++;
	pCpt->m_Data.data.find(pVeh->GetType())->second.nCumNbVehFranchissant[nVoie]++;

    pCpt->m_Data.nIDVehFranchissant.push_back( pVeh->GetID() );
    pCpt->m_Data.cTypeVehFranchissant.push_back( pVeh->GetType()->GetLabel() );

    pCpt->m_Data.dbInstVehFranchissant.push_back( dbInstFr );
    pCpt->m_Data.dbVoieVehFranchissant.push_back( nVoie+1 );

    pCpt->m_Data.data.find(nullptr)->second.dbCumulInvVit[nVoie] += dbInvVit;
	pCpt->m_Data.data.find(pVeh->GetType())->second.dbCumulInvVit[nVoie] += dbInvVit;
}

void PonctualSensor::WriteDef(DocTrafic* pDocTrafic)
{
    Point ptCoord;
    CalcCoordFromPos(GetTuyau(), GetPosition(), ptCoord);
    pDocTrafic->AddDefCapteur( GetUnifiedID(), GetTuyau()->GetLabel(), GetPosition(), ptCoord);
}


void PonctualSensor::Write(double dbInstant, Reseau * pReseau, double dbPeriodAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutputWriter)
{
    // Tous types de véhicule confondus
    int nNbVehFranchissant = 0;
    int nCumNbVehFranchissant = 0;

    for(int k=0; k<m_pTuyau->getNb_voies(); k++)
    {
        nNbVehFranchissant += m_Data.data.find(nullptr)->second.nNbVehFranchissant[k];
        nCumNbVehFranchissant += m_Data.data.find(nullptr)->second.nCumNbVehFranchissant[k];
    }

    // Cumul du nombre de véhicule depuis le début de la simulation
    std::string sNbCum = SystemUtil::ToString(nCumNbVehFranchissant);            

    // Débits

    // Débit global
    std::string sFlow = "";
    if(dbFin > dbDebut)
    {
        sFlow = SystemUtil::ToString(2, nNbVehFranchissant / (dbFin - dbDebut));
    }

    // Débit voie par voie
    std::string sNbCumVoie = "";
    for(int k=0; k<m_pTuyau->getNb_voies() ; k++)
        sNbCumVoie += SystemUtil::ToString(m_Data.data.find(nullptr)->second.nCumNbVehFranchissant[k]) + " ";


    // Vitesses

    // Vitesse
    double dbInvVit = 0;
    double dbVit = 0;
    std::string sVit = "";
    if(nNbVehFranchissant>0)
    {
        for(int k=0; k<m_pTuyau->getNb_voies() ; k++)
            dbInvVit += m_Data.data.find(nullptr)->second.dbCumulInvVit[k];
        dbInvVit = dbInvVit / nNbVehFranchissant;
    }
    if(dbInvVit>0)
    {
        dbVit = 1 / dbInvVit;
        sVit = SystemUtil::ToString(2, dbVit);
    }
    else
    {
        sVit = "-";
    }

    // Vitesse voie par voie
    std::string sVitVoie = "";
    for(int k=0; k<m_pTuyau->getNb_voies() ; k++)
    {
        if( m_Data.data.find(nullptr)->second.nNbVehFranchissant[k] > 0 && m_Data.data.find(nullptr)->second.dbCumulInvVit[k]>0)
        {
            double dbVit = 1/(m_Data.data.find(NULL)->second.dbCumulInvVit[k]/m_Data.data.find(NULL)->second.nNbVehFranchissant[k]);
            sVitVoie += SystemUtil::ToString(2, dbVit) + " ";
        }
        else
            sVitVoie += "- ";
    }

    if(pCSVOutputWriter)
    {
        // écriture des infos capteur dans les fichiers CSV
        pCSVOutputWriter->writeSensorData(dbInstant, m_strNom, nNbVehFranchissant, dbVit, sVit);
    }
    if(docTrafics.size()>0)
    {
        std::deque<TraceDocTrafic* >::const_iterator itDocTraf;
        for( itDocTraf = docTrafics.begin(); itDocTraf != docTrafics.end(); itDocTraf++ )
        {
            DocTrafic::PCAPTEUR xmlNodeCpt = (*itDocTraf)->AddInfoCapteur(m_strNom, "", sVit, sNbCum, sVitVoie, sNbCumVoie, sFlow, pReseau->DoVehicleListSensorExtraction());
            if(xmlNodeCpt && pReseau->DoVehicleListSensorExtraction())
            {
                for(size_t i=0; i<m_Data.nIDVehFranchissant.size(); i++)
                {
                    std::string sId = SystemUtil::ToString(m_Data.nIDVehFranchissant[i]);
                    (*itDocTraf)->AddVehCpt( xmlNodeCpt, sId, m_Data.dbInstVehFranchissant[i], m_Data.dbVoieVehFranchissant[i]);
                }
            }
        }
        // Type par type
        for(unsigned int nType=0; nType<pReseau->GetLstTypesVehicule().size(); nType++)
        {
            nNbVehFranchissant = 0;
            nCumNbVehFranchissant = 0;

            TypeVehicule *pTV = pReseau->GetLstTypesVehicule()[nType];

            for(int k=0; k<m_pTuyau->getNb_voies(); k++)
            {
                nNbVehFranchissant += m_Data.data.find(pTV)->second.nNbVehFranchissant[k];
                nCumNbVehFranchissant += m_Data.data.find(pTV)->second.nCumNbVehFranchissant[k];
            }

            // Cumul du nombre de véhicule depuis le début de la simulation
            sNbCum = SystemUtil::ToString(nCumNbVehFranchissant);            

            // Débits

            // Débit voie par voie
            sNbCumVoie = "";
            for(int k=0; k<m_pTuyau->getNb_voies() ; k++)
                sNbCumVoie += SystemUtil::ToString(m_Data.data.find(pTV)->second.nCumNbVehFranchissant[k]) + " ";


            // Vitesses

            // Vitesse
            double dbInvVit = 0;
            sVit = "";
            if(nNbVehFranchissant>0)
            {
                for(int k=0; k<m_pTuyau->getNb_voies() ; k++)
                    dbInvVit += m_Data.data.find(pTV)->second.dbCumulInvVit[k];
                dbInvVit = dbInvVit / nNbVehFranchissant;
            }
            if(dbInvVit>0)
            {
                double dbVit = 1 / dbInvVit;
                sVit = SystemUtil::ToString(2, dbVit);
            }
            else
            {
                sVit = "-";
            }

            // Vitesse voie par voie
            sVitVoie = "";
            for(int k=0; k<m_pTuyau->getNb_voies() ; k++)
            {
                if( m_Data.data.find(pTV)->second.nNbVehFranchissant[k] > 0 && m_Data.data.find(pTV)->second.dbCumulInvVit[k]>0)
                {
                    double dbVit = 1/(m_Data.data.find(pTV)->second.dbCumulInvVit[k]/m_Data.data.find(pTV)->second.nNbVehFranchissant[k]);
                    sVitVoie += SystemUtil::ToString(2, dbVit) + " ";
                }
                else
                    sVitVoie += "- ";
            }

            for( itDocTraf = docTrafics.begin(); itDocTraf != docTrafics.end(); ++itDocTraf)
            {
                DocTrafic::PCAPTEUR xmlNodeCpt = (*itDocTraf)->AddInfoCapteur(m_strNom, pTV->GetLabel(), sVit, sNbCum, sVitVoie, sNbCumVoie, "", pReseau->DoVehicleListSensorExtraction());
            }
        }
    }
}

void PonctualSensor::PrepareNextPeriod()
{
    // Remise à zéro des variables non cumulées sur les périodes
	std::map <TypeVehicule*, PonctualSensorData> :: iterator pIter;
	for ( pIter = m_Data.data.begin( ) ; pIter != m_Data.data.end( ) ; pIter++ )
	{
		pIter->second.nNbVehFranchissant.assign( m_pTuyau->getNbVoiesDis(),0);
		pIter->second.dbCumulInvVit.assign( m_pTuyau->getNbVoiesDis(),0.);    
	}				

    m_Data.dbInstVehFranchissant.clear();
    m_Data.nIDVehFranchissant.clear();
    m_Data.cTypeVehFranchissant.clear();    
    m_Data.dbVoieVehFranchissant.clear();

    if (m_pNextPeriodSensor)
    {
	    for ( pIter = m_Data.data.begin( ) ; pIter != m_Data.data.end( ) ; pIter++ )
	    {			
		    pIter->second.nNbVehFranchissant = m_pNextPeriodSensor->m_Data.data.find(pIter->first)->second.nNbVehFranchissant;
		    pIter->second.dbCumulInvVit = m_pNextPeriodSensor->m_Data.data.find(pIter->first)->second.dbCumulInvVit;
		    pIter->second.nCumNbVehFranchissant = m_pNextPeriodSensor->m_Data.data.find(pIter->first)->second.nCumNbVehFranchissant;   
	    }			

        m_Data.dbInstVehFranchissant = m_pNextPeriodSensor->m_Data.dbInstVehFranchissant;
        m_Data.nIDVehFranchissant = m_pNextPeriodSensor->m_Data.nIDVehFranchissant;
        m_Data.cTypeVehFranchissant = m_pNextPeriodSensor->m_Data.cTypeVehFranchissant;    
        m_Data.dbVoieVehFranchissant = m_pNextPeriodSensor->m_Data.dbVoieVehFranchissant;

        delete m_pNextPeriodSensor;
        m_pNextPeriodSensor = NULL;
    }
}


AbstractSensorSnapshot * PonctualSensor::TakeSnapshot()
{
    PonctualSensorSnapshot * pResult = new PonctualSensorSnapshot();
    pResult->data = m_Data.data;
    pResult->cTypeVehFranchissant = m_Data.cTypeVehFranchissant;
    pResult->dbInstVehFranchissant = m_Data.dbInstVehFranchissant;
    pResult->dbVoieVehFranchissant = m_Data.dbVoieVehFranchissant;
    pResult->nIDVehFranchissant = m_Data.nIDVehFranchissant;
    pResult->pNbTypeVehInst = m_Data.pNbTypeVehInst;
    return pResult;
}

void PonctualSensor::Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup)
{
    PonctualSensorSnapshot * pSnapshot = (PonctualSensorSnapshot*)backup;
    m_Data.cTypeVehFranchissant = pSnapshot->cTypeVehFranchissant;
    m_Data.dbInstVehFranchissant = pSnapshot->dbInstVehFranchissant;
    m_Data.dbVoieVehFranchissant = pSnapshot->dbVoieVehFranchissant;
    m_Data.nIDVehFranchissant = pSnapshot->nIDVehFranchissant;
    m_Data.pNbTypeVehInst = pSnapshot->pNbTypeVehInst;
    m_Data.data = pSnapshot->data;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template void PonctualSensorData::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PonctualSensorData::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PonctualSensorData::serialize(Archive& ar, const unsigned int version)
{
	ar & BOOST_SERIALIZATION_NVP(nCumNbVehFranchissant);
	ar & BOOST_SERIALIZATION_NVP(nNbVehFranchissant);
	ar & BOOST_SERIALIZATION_NVP(dbCumulInvVit);
}

template void PonctualSensor::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PonctualSensor::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PonctualSensor::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensor);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyau);
	ar & BOOST_SERIALIZATION_NVP(m_dbPosition);
    ar & BOOST_SERIALIZATION_NVP(m_pTuyauAmont);
	ar & BOOST_SERIALIZATION_NVP(m_Data);
}

template void PonctualSensorSnapshot::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void PonctualSensorSnapshot::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void PonctualSensorSnapshot::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(AbstractSensorSnapshot);
    ar & BOOST_SERIALIZATION_NVP(data);
    ar & BOOST_SERIALIZATION_NVP(nIDVehFranchissant);
    ar & BOOST_SERIALIZATION_NVP(cTypeVehFranchissant);
    ar & BOOST_SERIALIZATION_NVP(dbInstVehFranchissant);
    ar & BOOST_SERIALIZATION_NVP(dbVoieVehFranchissant);
    ar & BOOST_SERIALIZATION_NVP(pNbTypeVehInst);
}