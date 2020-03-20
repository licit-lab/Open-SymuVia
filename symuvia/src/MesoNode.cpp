#include "stdafx.h"
#include "tools.h"
#include "MesoNode.h"
#include "TuyauMeso.h"
#include "usage/SymuViaTripNode.h"
#include "reseau.h"
#include "ConnectionPonctuel.h"
#include "entree.h"

#include "TuyauMeso.h"

#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/deque.hpp>


CMesoNode::CMesoNode(void)
{
    m_pNextEventTuyauEntry = NULL;
    m_dNextEventTime= DBL_MAX;
}


CMesoNode::~CMesoNode(void)
{
}

void CMesoNode::AddArrival(Tuyau * pTuyau, double dArrivalTime, Vehicule *pVehicule, bool bEnsureIsLastArrival)
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO )
    {
        ((CTuyauMeso*)pTuyau)->AddArrival(dArrivalTime, pVehicule, bEnsureIsLastArrival);
    }
    else
    {
        // supprime si le vehicule est déja dans la liste des véhicules arrivant
        if( m_arrivals.find(pTuyau) != m_arrivals.end())
        {
           
            std::list<std::pair<double, Vehicule*> > arrivals =  m_arrivals.find(pTuyau)->second;
            std::list<std::pair<double, Vehicule*> >::iterator it;
            for( it = arrivals.begin() ; it!= arrivals.end() ; it++)
            {
                if(it->second ==  pVehicule)
                {
                    m_arrivals[pTuyau].remove(*it);
                }
            }
        }

        double dRealArrivalTime = dArrivalTime;
        if (bEnsureIsLastArrival)
        {
            double dMaxArrivalTime = GetMaxArrivalTime(pTuyau);
            dRealArrivalTime = std::max<double>(dMaxArrivalTime, dRealArrivalTime);
            assert(dMaxArrivalTime != DBL_MAX);
        }

        m_arrivals[pTuyau].push_back(std::pair<double, Vehicule*>(dRealArrivalTime, pVehicule));
		m_arrivals[pTuyau].sort(LessPairPtrIDRef<std::pair<double, Vehicule*> >());
    }
}
bool CMesoNode::AddEnteringVehiculeFromOutside(Tuyau * pTuyauFrom, CTuyauMeso * pTuyau, double dArrivalTime, Vehicule *pVehicule )
{
    SymuViaTripNode *pRealOrigin = pTuyau->GetReseau()->GetOriginSymuViaTripNode(pTuyau->GetCnxAssAm());
                  
    int nVehWainting = 0;
    if( pRealOrigin)
    { 
        std::map<int, std::deque<boost::shared_ptr<Vehicule>> >::iterator itAttBegin = pRealOrigin->m_mapVehEnAttente[pTuyau].begin();
        std::map<int, std::deque<boost::shared_ptr<Vehicule>> >::iterator itAtt;
        std::map<int, std::deque<boost::shared_ptr<Vehicule>> >::iterator itAttEnd = pRealOrigin->m_mapVehEnAttente[pTuyau].end();
        for( itAtt = itAttBegin; itAtt != itAttEnd; itAtt++)
        {
            nVehWainting += (int)itAtt->second.size();
        }
                     
    }
    if( GetNextEventTime()>dArrivalTime &&
        nVehWainting == 0 &&
        pTuyau->GetNextSupplyTime() != DBL_MAX && 
        pTuyau->IsCongested() == false)
    {
        pTuyau->GetCnxAssAm()->GetReseau()->ChangeMesoNodeEventTime(this, dArrivalTime, pTuyauFrom);
        SetNextArrival(pTuyauFrom, std::pair<double, Vehicule*>(dArrivalTime, pVehicule) );

        AddArrival(pTuyauFrom, dArrivalTime, pVehicule, false);
        return true; 
    }
                
    
    return false;
                     
}





void CMesoNode::SetNextSupplyTime(Tuyau * pTuyau, double dSupplyTime )
{
    if(pTuyau && pTuyau->GetType() == Tuyau::TT_MESO )
    {
        ((CTuyauMeso *)pTuyau)->SetNextSupplyTime(dSupplyTime);
    }
    else
    {
        m_dNextSupplyTime[pTuyau] = dSupplyTime;
    }
}


double CMesoNode::GetNextSupplyTime( Tuyau * pTuyau) const
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO)
    {
        return ((CTuyauMeso *)pTuyau)->GetNextSupplyTime();
    }
    else
    {
        if( m_dNextSupplyTime.find(pTuyau) != m_dNextSupplyTime.end())
        {
            return m_dNextSupplyTime.find(pTuyau)->second;
        }
        else
        {
            return -DBL_MAX;
        }

    }
}




std::list<std::pair<double,Vehicule *> > CMesoNode::GetArrivalTimes(Tuyau *pTuyau) const
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO)
    {
        return ((CTuyauMeso *)pTuyau)->GetArrivalTimes();
    }
    else
    {
        if( m_arrivals.find(pTuyau) != m_arrivals.end() )
        {
            return m_arrivals.find(pTuyau)->second;
        }
        else
        {
            return std::list<std::pair<double,Vehicule *> >();
        }
    }
}

std::pair<double,Vehicule* > CMesoNode::GetMinArrival( Tuyau* pTuyau) const
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO)
    {
        return ((CTuyauMeso *)pTuyau)->GetMinArrival();
    }
    else
    {
        if( m_arrivals.find(pTuyau)->second.size() >0)
        {
            return m_arrivals.find(pTuyau)->second.front();
        }

    }
    return std::pair<double,Vehicule* > (DBL_MAX, nullptr);
}

std::pair<Tuyau*, std::pair<double, Vehicule*> > CMesoNode::GetNextIncomingLink(Tuyau * pLink) const
{
    std::pair<Tuyau*, std::pair<double, Vehicule*> > result;
    result.first = nullptr;
    result.second.first = DBL_MAX;
    result.second.second = nullptr;

    std::map<Tuyau*, std::pair<double, Vehicule* > >::const_iterator iter;
    for(iter = m_nextArrival.begin(); iter != m_nextArrival.end(); ++iter)
    {
        if(iter->first)
        {
            if(iter->second.first < result.second.first)
            {
                result.first = iter->first;
                result.second = iter->second;
            }
        }
    }

    // On regarde aussi dans le nextArrival des tronçons amont du noeud
    if (pLink)
    {
        Connexion * pConnexion = NULL;
        if (pLink->GetCnxAssAm() == this)
        {
            pConnexion = pLink->GetCnxAssAm();
        }
        else if (pLink->GetCnxAssAv() == this)
        {
            pConnexion = pLink->GetCnxAssAv();
        }
        for (size_t i = 0; i < pConnexion->m_LstTuyAssAm.size(); i++)
        {
            CTuyauMeso * pTuyauMeso = dynamic_cast<CTuyauMeso*>(pConnexion->m_LstTuyAssAm[i]);
            if (pTuyauMeso)
            {
                const std::pair<double, Vehicule* > & nextArrivalTuyau = pTuyauMeso->GetNextArrival();
                if (nextArrivalTuyau.first < result.second.first)
                {
                    result.first = pTuyauMeso;
                    result.second = nextArrivalTuyau;
                }
            }
        }
    }

    return result;
}


void CMesoNode::SetGlobalNextEvent(double dNextEventTime, Tuyau * pEventTuyauEntry)
{
    m_pNextEventTuyauEntry =pEventTuyauEntry;
    m_dNextEventTime  =dNextEventTime;
}



void CMesoNode::SetRankNextIncomingVeh(Tuyau * pEventTuyau, int iRank)
{
    m_mapRanksIncomming[pEventTuyau] = iRank;
}

void CMesoNode::RemoveFirstDownStreamPassingTime(Tuyau * pTuyau)
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO)
    {
        ((CTuyauMeso*)pTuyau)->RemoveFirstDownStreamPassingTime();
    }
    else
    {
        if( m_downstreamPassingTimes[pTuyau].size()>0)
        {
            m_downstreamPassingTimes[pTuyau].pop_front( );
        }

    }
}

void CMesoNode::RemoveFirstUpstreamPassingTime(Tuyau * pTuyau )
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO)
    {
        ((CTuyauMeso*)pTuyau)->RemoveFirstUpstreamPassingTime();
    }
    else
    {
        if( m_upstreamPassingTimes[pTuyau].size()>0)
        {
            m_upstreamPassingTimes[pTuyau].pop_front( );
        }
    }
}


void CMesoNode::AddUpstreamPassingTime(Tuyau * pTuyau, double dInstant  )
{
    if( pTuyau &&   pTuyau->GetType() == Tuyau::TT_MESO)
    {
        
        ((CTuyauMeso*)pTuyau)->AddUpstreamPassingTime( dInstant);
        

    }
    else
    {
    
        m_upstreamPassingTimes[pTuyau].push_back( dInstant );

    }
}


void CMesoNode::AddDownstreamPassingTime(Tuyau* pTuyau/*, int iRank,*/, double dInstant  )
{
   if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO)
    {
        ((CTuyauMeso*)pTuyau)->AddDownstreamPassingTime( dInstant);
    }
    else
    {
        m_downstreamPassingTimes[pTuyau].push_back( dInstant );
    }
   
}

void CMesoNode::IncreaseRankNextIncommingVeh(Tuyau *pTuyau)
{
    std::map< Tuyau*, int>::iterator it =m_mapRanksIncomming.find(pTuyau) ;
    if( it != m_mapRanksIncomming.end())
    {
        m_mapRanksIncomming[pTuyau ] = it->second +1;
    }
    else
    {
        m_mapRanksIncomming[pTuyau] = 1;
    }

}





void CMesoNode::IncreaseRankNextOutgoingVeh(Tuyau *pTuyau)
{
    std::map< Tuyau*, int>::iterator it =m_mapRanksOutGoing.find(pTuyau) ;
    if( it != m_mapRanksOutGoing.end())
    {
        m_mapRanksOutGoing[pTuyau ] = it->second + 1;
    }
    else
    {
        m_mapRanksOutGoing[pTuyau] = 1;
    }

}
void CMesoNode::DecreaseRankNextIncommingVeh(Tuyau *pTuyau)
{
    std::map< Tuyau*, int>::iterator it =m_mapRanksIncomming.find(pTuyau) ;
    if( it != m_mapRanksIncomming.end())
    {
        m_mapRanksIncomming[pTuyau ] = std::max<int>(it->second - 1,0);
  
    }
    else
    {
        m_mapRanksIncomming[pTuyau] = 0;
    }

}
void CMesoNode::DecreaseRankNextOutgoingVeh(Tuyau *pTuyau)
{
    std::map< Tuyau*, int>::iterator it =m_mapRanksOutGoing.find(pTuyau) ;
    if( it != m_mapRanksOutGoing.end())
    {
        m_mapRanksOutGoing[pTuyau ] = std::max<int>(it->second - 1, 0);
    }
    else
    {
        m_mapRanksOutGoing[pTuyau] = 0;
    }

}

int CMesoNode::GetRankNextOutgoingVehicule(Tuyau * pTuyau) const
{
     std::map< Tuyau*, int>::const_iterator it = m_mapRanksOutGoing.find(pTuyau) ;
    if( it != m_mapRanksOutGoing.end())
    {
        return it->second ;
    }
    else
    {
        return 0;
    }
}

std::deque<double> CMesoNode::GetUpstreamPassingTimes(Tuyau * pUpstream ) const
{
    if( pUpstream && pUpstream->GetType() == Tuyau::TT_MESO )
    {
        return ((CTuyauMeso*)pUpstream)->GetUpstreamPassingTimes();
    }
    else
    {
        if( m_upstreamPassingTimes.find(pUpstream) != m_upstreamPassingTimes.end())
        {
            return m_upstreamPassingTimes.find(pUpstream)->second;
        }
        else
        {
            return std::deque<double>();
        }
    }
}
/*
std::vector<Vehicule *> CMesoNode::GetCreatedVehicules(CTuyauMeso *pTuyau) const
{
    if(m_mapCreatedVehicule.find(pTuyau) != m_mapCreatedVehicule.end())
    {
        return m_mapCreatedVehicule.find(pTuyau)->second;
      
    }
    else
        return std::vector<Vehicule *>();
}

void CMesoNode::RemoveCreatedVehicules(CTuyauMeso *pTuyau)
{
    m_mapCreatedVehicule[pTuyau].clear();

}

void CMesoNode::AddCreatedVehicule(CTuyauMeso *pTuyau, Vehicule*pVehicule)
{
    if(m_mapCreatedVehicule.find(pTuyau) != m_mapCreatedVehicule.end())
    {
         m_mapCreatedVehicule[pTuyau].push_back(pVehicule);
    }
    else
    {
        m_mapCreatedVehicule[pTuyau] = std::vector<Vehicule *>();
        m_mapCreatedVehicule[pTuyau].push_back(pVehicule);
    }
}*/






void CMesoNode::ClearArrivals(Tuyau * pTuyau )
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO)
    {
        ((CTuyauMeso*)pTuyau)->ClearArrivals();
    }
    else
    {
       m_arrivals[pTuyau].clear();
    }
}




void CMesoNode::SetNextArrival(Tuyau * pTuyau, std::pair<double, Vehicule*> arrival )
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO )
    {
        ((CTuyauMeso*)pTuyau)->SetNextArrival(arrival);
    }
    else
    {
        m_nextArrival[pTuyau] = arrival;
    }
}


void CMesoNode::ReplaceArrivalTimes(Tuyau * pTuyau,std::list< std::pair<double, Vehicule *>  >newList )
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO)
    {
        ((CTuyauMeso*)pTuyau)->ReplaceArrivalTimes( newList);

    }
    else
    {
        m_arrivals[pTuyau].clear();
        m_arrivals[pTuyau].assign(newList.begin(), newList.end()) ;
		m_arrivals[pTuyau].sort(LessPairPtrIDRef<std::pair<double, Vehicule*> >());

    }
}



std::pair<double,Vehicule* >  CMesoNode::GetNextArrival(Tuyau * pTuyau) const 
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO)
    {
        return ((CTuyauMeso*)pTuyau)->GetNextArrival();
    }
    else
    {
        if( m_nextArrival.find(pTuyau) != m_nextArrival.end())
        {
            return m_nextArrival.find(pTuyau)->second; 
        }
        else
        {
            return std::pair<double, Vehicule*>(DBL_MAX, nullptr);
        }
    }
}

double CMesoNode::GetMaxArrivalTime(Tuyau* pTuyau) const
{
    if( pTuyau && pTuyau->GetType() == Tuyau::TT_MESO)
    {
        return ((CTuyauMeso*) pTuyau)->GetMaxArrivalTime();
    }
    else
    {
        if( m_arrivals.find(pTuyau) != m_arrivals.end() &&  m_arrivals.find(pTuyau)->second.size()>0)
        {
            return m_arrivals.find(pTuyau)->second.back().first;
        }
        else
        {
            return -1;
        }
    }
}

void CMesoNode::ToMicro(Tuyau * pLink, Reseau * pNetwork)
{
    // Suppression des références au tronçon qui devient micro...

	m_arrivals.erase(pLink);
	m_nextArrival.erase(pLink);

	// nettoyage des arrivées prévues pour les véhicules qui n'ont pas eu le temps de s'insérer sur le tuyau meso 
	// alors que celui-ci redevient déjà meso :
	std::map<Tuyau*, std::list< std::pair<double, Vehicule* > > >::iterator iterArrivals;
	for (iterArrivals = m_arrivals.begin(); iterArrivals != m_arrivals.end(); ++iterArrivals)
	{
		// Si pas de tuyau d'entrée, pas la peine (et on ne peut pas faire de calculnexttuyau sur un tuyau nul de toute façon
		if (iterArrivals->first)
		{
			std::list< std::pair<double, Vehicule* > >::iterator iterListArrivals;
			for (iterListArrivals = iterArrivals->second.begin(); iterListArrivals != iterArrivals->second.end();)
			{
				if (iterListArrivals->second)
				{
					Tuyau * pNextTuyau = iterListArrivals->second->CalculNextTuyau(iterArrivals->first, iterListArrivals->second->GetReseau()->m_dbInstSimu);
					if (pNextTuyau == pLink)
					{
						// remplacement du nextarrival en conséquence si le next arrival est l'arrival qu'on supprime
						std::map<Tuyau*, std::pair<double, Vehicule* > >::iterator iterNextArrival = m_nextArrival.find(iterArrivals->first);
						if (iterNextArrival != m_nextArrival.end() && iterNextArrival->second == *iterListArrivals)
						{
							std::list< std::pair<double, Vehicule* > >::iterator iterListNextArrival = iterListArrivals;
							iterListNextArrival++;
							if (iterListNextArrival != iterArrivals->second.end())
							{
								m_nextArrival[iterArrivals->first] = *iterListNextArrival;
							}
							else
							{
								m_nextArrival[iterArrivals->first] = std::make_pair(DBL_MAX, nullptr);
							}
						}

						iterListArrivals = iterArrivals->second.erase(iterListArrivals);
					}
					else
					{
						iterListArrivals++;
					}
				}
				else
				{
					iterListArrivals++;
				}
			}
		}
	}

    m_upstreamPassingTimes.erase(pLink);
    m_downstreamPassingTimes.erase(pLink);
    m_mapRanksIncomming.erase(pLink);
    m_mapRanksOutGoing.erase(pLink);

    std::pair<Tuyau*, std::pair<double, Vehicule*> > nextArrival = GetNextIncomingLink(pLink);
    pNetwork->ChangeMesoNodeEventTime(this, nextArrival.second.first,  nextArrival.first);

    m_dNextSupplyTime.erase(pLink);
}

void CMesoNode::ToMeso(Tuyau * pLink)
{
    // Il faut transférer les données sur les tuyaux micro stockées au niveau du noeud vers les tuyaux meso
    // afin que les accesseurs qui se basent sur le type de tuyau fonctionnent bien.

    CTuyauMeso * pTuyauMeso = dynamic_cast<CTuyauMeso*>(pLink);

    std::map<Tuyau*,std::list< std::pair<double, Vehicule* > > >::iterator iterArrivals = m_arrivals.find(pLink);
    if(iterArrivals != m_arrivals.end())
    {
        if (pTuyauMeso)
        {
            pTuyauMeso->getArrivals() = iterArrivals->second;
        }
        m_arrivals.erase(iterArrivals);
    }

    std::map<Tuyau*, std::pair<double, Vehicule* > >::iterator iterNextArrivals = m_nextArrival.find(pLink);
    if(iterNextArrivals != m_nextArrival.end())
    {
        if (pTuyauMeso)
        {
            pTuyauMeso->SetNextArrival(iterNextArrivals->second);
        }
        m_nextArrival.erase(iterNextArrivals);
    }

    std::map<Tuyau*, std::deque<double > >::iterator iterUpstreamPassingTimes = m_upstreamPassingTimes.find(pLink);
    if(iterUpstreamPassingTimes != m_upstreamPassingTimes.end())
    {
        if (pTuyauMeso)
        {
            pTuyauMeso->setUpstreamPassingTimes(iterUpstreamPassingTimes->second);
        }
        m_upstreamPassingTimes.erase(iterUpstreamPassingTimes);
    }

    std::map<Tuyau*, std::deque<double > >::iterator iterDownstreamPassingTimes = m_downstreamPassingTimes.find(pLink);
    if(iterDownstreamPassingTimes != m_downstreamPassingTimes.end())
    {
        if (pTuyauMeso)
        {
            pTuyauMeso->getDownstreamPassingTimes() = iterDownstreamPassingTimes->second;
        }
        m_downstreamPassingTimes.erase(iterDownstreamPassingTimes);
    }

    std::map<Tuyau*,  double>::iterator iterNextSupplyTime = m_dNextSupplyTime.find(pLink);
    if(iterNextSupplyTime != m_dNextSupplyTime.end())
    {
        if (pTuyauMeso)
        {
            pTuyauMeso->SetNextSupplyTime(iterNextSupplyTime->second);
        }
        m_dNextSupplyTime.erase(iterNextSupplyTime);
    }

    // On nettoie ici pour remettre à zero le fonctionnment meso du tronçon correspondant (sinon blocage),
    // cf. nettoyage du même type dans CTuyauMeso::ToMeso.
    m_mapRanksIncomming.erase(pLink);
    m_mapRanksOutGoing.erase(pLink);

    // Cas de l'origine : il faut convertir les véhicules en attente en instants d'arrivée :
    if (this->IsAnOrigine() && pLink == NULL)
    {
        Entree * pEntree = (Entree*)this;
        // On se base sur les ID des véhicules pour avoir le bon ordre des arrivals
        std::map<int, Vehicule*> mapVehAttenteByID;
        for (std::map<Tuyau*, std::map<int, std::deque<boost::shared_ptr<Vehicule>>>>::const_iterator iterTuy = pEntree->m_mapVehEnAttente.begin();
            iterTuy != pEntree->m_mapVehEnAttente.end(); ++iterTuy)
        {
            for (std::map<int, std::deque<boost::shared_ptr<Vehicule>>>::const_iterator iterLane = iterTuy->second.begin();
                iterLane != iterTuy->second.end(); ++iterLane)
            {
                for (size_t iVeh = 0; iVeh < iterLane->second.size(); iVeh++)
                {
                    Vehicule * pVeh = iterLane->second[iVeh].get();
                    mapVehAttenteByID[pVeh->GetID()] = pVeh;
                }
            }
        }

        bool bFirstArrival = true;
        for (std::map<int, Vehicule*>::const_iterator iterArrival = mapVehAttenteByID.begin(); iterArrival != mapVehAttenteByID.end(); ++iterArrival)
        {
            pEntree->AddArrival(NULL, pEntree->GetReseau()->m_dbInstSimu, iterArrival->second, false);
            if (bFirstArrival)
            {
                pEntree->GetReseau()->ChangeMesoNodeEventTime(this, pEntree->GetReseau()->m_dbInstSimu, NULL);
                pEntree->SetNextArrival(NULL, std::pair<double, Vehicule*>(pEntree->GetReseau()->m_dbInstSimu, iterArrival->second));
                bFirstArrival = false;
            }
        }
    }
}


template void CMesoNode::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CMesoNode::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CMesoNode::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_arrivals);
    ar & BOOST_SERIALIZATION_NVP(m_upstreamPassingTimes);
    ar & BOOST_SERIALIZATION_NVP(m_downstreamPassingTimes);
    ar & BOOST_SERIALIZATION_NVP(m_nextArrival);
    ar & BOOST_SERIALIZATION_NVP(m_mapRanksIncomming);
    ar & BOOST_SERIALIZATION_NVP(m_mapRanksOutGoing);
    ar & BOOST_SERIALIZATION_NVP(m_pNextEventTuyauEntry);
    ar & BOOST_SERIALIZATION_NVP(m_dNextEventTime);
    ar & BOOST_SERIALIZATION_NVP(m_dNextSupplyTime);
}

 